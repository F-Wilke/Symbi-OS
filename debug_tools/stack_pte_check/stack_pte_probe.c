/*
 * stack_pte_probe.c - Kernel module to check stack PTEs for corruption
 *
 * This module uses kprobes to hook into syscall handlers and checks the
 * PTEs associated with the current process's stack against a configurable
 * binary pattern.
 *
 * Uses manual page table walking since walk_page_range is not exported.
 */

#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/version.h>
#include <linux/pgtable.h>
#include <asm/pgtable.h>

/* Configuration parameters */
static unsigned long pte_mask = 0xFFF;  /* Mask to apply to PTE */
static unsigned long pte_pattern = 0x920;  /* Pattern to check for */
static int rate_limit = 1;  /* Use rate limiting for warnings */

module_param(pte_mask, ulong, 0644);
MODULE_PARM_DESC(pte_mask, "Mask to apply to PTE value");

module_param(pte_pattern, ulong, 0644);
MODULE_PARM_DESC(pte_pattern, "Pattern to check for in masked PTE");

module_param(rate_limit, int, 0644);
MODULE_PARM_DESC(rate_limit, "Use rate limiting for warnings (1=yes, 0=no)");

/* Statistics */
static atomic_t check_count = ATOMIC_INIT(0);
static atomic_t match_count = ATOMIC_INIT(0);

/*
 * Check a single PTE for the pattern
 */
static void check_pte(pte_t *pte, unsigned long addr)
{
	unsigned long val;
	unsigned long masked;

	if (!pte || !pte_present(*pte))
		return;

	val = pte_val(*pte);
	masked = val & pte_mask;

	if (masked == pte_pattern) {
		atomic_inc(&match_count);
		if (rate_limit) {
			pr_warn_ratelimited(
				"[stack_pte_probe] BAD STACK PTE: "
				"comm=%s pid=%d addr=0x%lx pte=0x%lx "
				"(masked=0x%lx pattern=0x%lx)\n",
				current->comm, current->pid, addr, val,
				masked, pte_pattern);
		} else {
			pr_warn(
				"[stack_pte_probe] BAD STACK PTE: "
				"comm=%s pid=%d addr=0x%lx pte=0x%lx "
				"(masked=0x%lx pattern=0x%lx)\n",
				current->comm, current->pid, addr, val,
				masked, pte_pattern);
		}
	}
}

/*
 * Manually walk page tables for a given address range
 * This is necessary because walk_page_range is not exported for modules
 */
static void manual_walk_page_range(struct mm_struct *mm, 
				   unsigned long start, unsigned long end)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *ptep;
	unsigned long addr;
	unsigned long next_pgd, next_p4d, next_pud, next_pmd;

	for (addr = start; addr < end; ) {
		/* PGD level */
		pgd = pgd_offset(mm, addr);
		if (pgd_none(*pgd) || pgd_bad(*pgd)) {
			next_pgd = (addr + PGDIR_SIZE) & PGDIR_MASK;
			addr = (next_pgd < end) ? next_pgd : end;
			continue;
		}

		/* P4D level */
		p4d = p4d_offset(pgd, addr);
		if (p4d_none(*p4d) || p4d_bad(*p4d)) {
			next_p4d = (addr + P4D_SIZE) & P4D_MASK;
			addr = (next_p4d < end) ? next_p4d : end;
			continue;
		}

		/* PUD level */
		pud = pud_offset(p4d, addr);
		if (pud_none(*pud) || pud_bad(*pud)) {
			next_pud = (addr + PUD_SIZE) & PUD_MASK;
			addr = (next_pud < end) ? next_pud : end;
			continue;
		}

		/* PMD level */
		pmd = pmd_offset(pud, addr);
		if (pmd_none(*pmd) || pmd_bad(*pmd)) {
			next_pmd = (addr + PMD_SIZE) & PMD_MASK;
			addr = (next_pmd < end) ? next_pmd : end;
			continue;
		}

		/* PTE level - use kernel direct mapping */
		ptep = pte_offset_kernel(pmd, addr);
		if (ptep) {
			check_pte(ptep, addr);
		}

		addr += PAGE_SIZE;
	}
}

/*
 * Check all PTEs in the current process's stack
 */
static void check_current_stack_ptes(void)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long start, end;

	if (!mm)
		return;  /* Kernel thread */

	/* Try to acquire the mmap lock */
	if (!mmap_read_trylock(mm)) {
		/* Lock is held - skip this check to avoid deadlock */
		return;
	}

	/* Find the stack VMA */
	vma = find_vma(mm, mm->start_stack);
	if (!vma || !(vma->vm_flags & VM_GROWSDOWN)) {
		mmap_read_unlock(mm);
		return;
	}

	start = vma->vm_start;
	end = vma->vm_end;

	/* Manually walk the page tables in the stack range */
	manual_walk_page_range(mm, start, end);

	mmap_read_unlock(mm);
	atomic_inc(&check_count);
}

/*
 * Kprobe pre-handler - called when the probed function is hit
 */
static int probe_pre_handler(struct kprobe *p, struct pt_regs *regs)
{
	/* Safety checks */
	if (!current->mm)
		return 0;  /* Kernel thread */

	if (in_atomic() || irqs_disabled())
		return 0;  /* Not safe to sleep/lock */

	if (current->flags & PF_KTHREAD)
		return 0;  /* Kernel thread */

	/* Perform the stack PTE check */
	check_current_stack_ptes();

	return 0;
}

/*
 * Kprobe definitions - multiple syscalls for coverage
 * Choose simple, safe syscalls that don't manipulate memory
 */
static struct kprobe kprobes[] = {
	{
		.symbol_name = "__x64_sys_getpid",
		.pre_handler = probe_pre_handler,
	},
	{
		.symbol_name = "__x64_sys_read",
		.pre_handler = probe_pre_handler,
	},
	{
		.symbol_name = "__x64_sys_write",
		.pre_handler = probe_pre_handler,
	},
	{
		.symbol_name = "symbi_elevate",
		.pre_handler = probe_pre_handler,
	},
	{
		.symbol_name = "symbi_lower",
		.pre_handler = probe_pre_handler,
	},
};

static int num_probes = ARRAY_SIZE(kprobes);
static int registered_probes = 0;  /* Track successfully registered probes */

/*
 * Module initialization
 */
static int __init stack_pte_probe_init(void)
{
	int i, ret;

	pr_info("[stack_pte_probe] Loading module\n");
	pr_info("[stack_pte_probe] PTE mask=0x%lx pattern=0x%lx rate_limit=%d\n",
		pte_mask, pte_pattern, rate_limit);

	/* Register all kprobes - continue even if some fail */
	for (i = 0; i < num_probes; i++) {
		ret = register_kprobe(&kprobes[i]);
		if (ret < 0) {
			pr_warn("[stack_pte_probe] Failed to register kprobe for %s: %d (symbol may not exist)\n",
			        kprobes[i].symbol_name, ret);
			/* Mark this probe as not registered */
			kprobes[i].addr = NULL;
		} else {
			pr_info("[stack_pte_probe] Registered kprobe at %s: %px\n",
				kprobes[i].symbol_name, kprobes[i].addr);
			registered_probes++;
		}
	}

	/* Fail only if no probes were registered at all */
	if (registered_probes == 0) {
		pr_err("[stack_pte_probe] Failed to register any kprobes - module cannot function\n");
		return -ENOENT;
	}

	pr_info("[stack_pte_probe] Successfully registered %d/%d kprobes\n",
		registered_probes, num_probes);
	return 0;
}

/*
 * Module cleanup
 */
static void __exit stack_pte_probe_exit(void)
{
	int i;

	/* Unregister only the kprobes that were successfully registered */
	for (i = 0; i < num_probes; i++) {
		if (kprobes[i].addr != NULL) {
			unregister_kprobe(&kprobes[i]);
			pr_info("[stack_pte_probe] Unregistered kprobe for %s\n",
				kprobes[i].symbol_name);
		}
	}

	pr_info("[stack_pte_probe] Statistics: checks=%d matches=%d\n",
		atomic_read(&check_count), atomic_read(&match_count));
	pr_info("[stack_pte_probe] Module unloaded (had %d/%d probes active)\n",
		registered_probes, num_probes);
}

module_init(stack_pte_probe_init);
module_exit(stack_pte_probe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Symbi-OS Debug Tools");
MODULE_DESCRIPTION("Stack PTE corruption checker using kprobes");
MODULE_VERSION("1.0");
