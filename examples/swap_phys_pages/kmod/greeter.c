#include <linux/module.h>
#include <linux/init.h>

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/uaccess.h>
#include <linux/signal.h>
#include <linux/pid.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/mm.h>
#include <asm-generic/io.h>


//taken from https://elixir.bootlin.com/linux/v6.16/source/tools/testing/selftests/kvm/include/x86/processor.h#L1167


//  Define the module metadata.
#define MODULE_NAME "greeter"
MODULE_AUTHOR("Dave Kerr");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple kernel module to greet a user");
MODULE_VERSION("0.1");



void swap_phys_pages(void* vaddr1, void* vaddr2) {


    pgd_t *pgd = pgd_offset(current->mm, (unsigned long)vaddr1);
    p4d_t* p4d = p4d_offset(pgd, (unsigned long)vaddr1);
    pud_t* pud = pud_offset(p4d, (unsigned long)vaddr1);
    pmd_t* pmd = pmd_offset(pud, (unsigned long)vaddr1);
    pte_t* pte1 = pte_offset_kernel(pmd, (unsigned long)vaddr1);
    
    pgd = pgd_offset(current->mm, (unsigned long)vaddr2);
    p4d = p4d_offset(pgd, (unsigned long)vaddr2);
    pud = pud_offset(p4d, (unsigned long)vaddr2);
    pmd = pmd_offset(pud, (unsigned long)vaddr2);
    pte_t* pte2 = pte_offset_kernel(pmd, (unsigned long)vaddr2);
    
    printk("swap_phys_pages: pte1=%p, pte2=%p\n", pte1, pte2);

    //print values before swap
    printk("swap_phys_pages: Before swap: pte1->pte=%lx, pte2->pte=%lx\n", pte1->pte, pte2->pte);
    
    swap(pte1->pte, pte2->pte);
    
    printk("swap_phys_pages: Swap complete\n");
    printk("swap_phys_pages: After swap: pte1->pte=%lx, pte2->pte=%lx\n", pte1->pte, pte2->pte);

    // Flush TLBs for both affected VMAs
    asm volatile("invlpg (%0)" ::"r"(vaddr1) : "memory");
    asm volatile("invlpg (%0)" ::"r"(vaddr2) : "memory");
    
    printk("swap_phys_pages: TLB flush complete\n");
}






//  Define the name parameter.
static char *name = "Bilbo";
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");


static int __init greeter_init(void)
{
    pr_info("%s: module loaded at 0x%p\n", MODULE_NAME, greeter_init);
    pr_info("%s: greetings %s\n", MODULE_NAME, name);

    return 0;
}

static void __exit greeter_exit(void)
{
    pr_info("%s: goodbye %s\n", MODULE_NAME, name);
    pr_info("%s: module unloaded from 0x%p\n", MODULE_NAME, greeter_exit);
}

module_init(greeter_init);
module_exit(greeter_exit);
