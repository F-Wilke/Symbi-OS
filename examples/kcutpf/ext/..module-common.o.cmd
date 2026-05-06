savedcmd_.module-common.o := gcc -Wp,-MMD,./..module-common.o.d -nostdinc -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated -I/home/user/Symbi-OS/linux-nodbg/include -I/home/user/Symbi-OS/linux-nodbg/include -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi -I/home/user/Symbi-OS/linux-nodbg/include/uapi -I/home/user/Symbi-OS/linux-nodbg/include/generated/uapi -include /home/user/Symbi-OS/linux-nodbg/include/linux/compiler-version.h -include /home/user/Symbi-OS/linux-nodbg/include/linux/kconfig.h -include /home/user/Symbi-OS/linux-nodbg/include/linux/compiler_types.h -D__KERNEL__ -std=gnu11 -fshort-wchar -funsigned-char -fno-common -fno-PIE -fno-strict-aliasing -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -fcf-protection=branch -fno-jump-tables -m64 -falign-jumps=1 -falign-loops=1 -mno-80387 -mno-fp-ret-in-387 -mpreferred-stack-boundary=3 -mskip-rax-setup -march=x86-64 -mtune=generic -mno-red-zone -mcmodel=kernel -mstack-protector-guard-reg=gs -mstack-protector-guard-symbol=__ref_stack_chk_guard -Wno-sign-compare -fno-asynchronous-unwind-tables -mindirect-branch=thunk-extern -mindirect-branch-register -mindirect-branch-cs-prefix -mfunction-return=thunk-extern -fno-jump-tables -mharden-sls=all -fpatchable-function-entry=16,16 -fno-delete-null-pointer-checks -O2 -fno-allow-store-data-races -fstack-protector-strong -ftrivial-auto-var-init=zero -fzero-init-padding-bits=all -fno-stack-clash-protection -pg -mrecord-mcount -mfentry -DCC_USING_FENTRY -fno-inline-functions-called-once -fmin-function-alignment=16 -fstrict-flex-arrays=3 -fno-strict-overflow -fno-stack-check -fconserve-stack -fno-builtin-wcslen -Wall -Wextra -Wundef -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Werror=strict-prototypes -Wno-format-security -Wno-trigraphs -Wno-frame-address -Wno-address-of-packed-member -Wmissing-declarations -Wmissing-prototypes -Wframe-larger-than=2048 -Wno-main -Wno-dangling-pointer -Wvla-larger-than=1 -Wno-pointer-sign -Wcast-function-type -Wno-unterminated-string-initialization -Wno-array-bounds -Wno-stringop-overflow -Wno-alloc-size-larger-than -Wimplicit-fallthrough=5 -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -Wenum-conversion -Wunused -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-packed-not-aligned -Wno-format-overflow -Wno-format-truncation -Wno-stringop-truncation -Wno-override-init -Wno-missing-field-initializers -Wno-type-limits -Wno-shift-negative-value -Wno-maybe-uninitialized -Wno-sign-compare -Wno-unused-parameter -g  -fsanitize=bounds-strict -fsanitize=shift    -DMODULE  -DKBUILD_BASENAME='".module_common"' -DKBUILD_MODNAME='".module_common.o"' -D__KBUILD_MODNAME=kmod_.module_common.o -c -o .module-common.o /home/user/Symbi-OS/linux-nodbg/scripts/module-common.c   ; /home/user/Symbi-OS/linux-nodbg/tools/objtool/objtool --hacks=jump_label --hacks=noinstr --hacks=skylake --ibt --orc --retpoline --rethunk --sls --static-call --uaccess --prefix=16  --link  --module .module-common.o

source_.module-common.o := /home/user/Symbi-OS/linux-nodbg/scripts/module-common.c

deps_.module-common.o := \
    $(wildcard include/config/UNWINDER_ORC) \
    $(wildcard include/config/MITIGATION_RETPOLINE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler-version.h \
    $(wildcard include/config/CC_VERSION_TEXT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kconfig.h \
    $(wildcard include/config/CPU_BIG_ENDIAN) \
    $(wildcard include/config/BOOGER) \
    $(wildcard include/config/FOO) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler_types.h \
    $(wildcard include/config/DEBUG_INFO_BTF) \
    $(wildcard include/config/PAHOLE_HAS_BTF_TAG) \
    $(wildcard include/config/FUNCTION_ALIGNMENT) \
    $(wildcard include/config/CC_HAS_SANE_FUNCTION_ALIGNMENT) \
    $(wildcard include/config/X86_64) \
    $(wildcard include/config/ARM64) \
    $(wildcard include/config/LD_DEAD_CODE_DATA_ELIMINATION) \
    $(wildcard include/config/LTO_CLANG) \
    $(wildcard include/config/HAVE_ARCH_COMPILER_H) \
    $(wildcard include/config/CC_HAS_COUNTED_BY) \
    $(wildcard include/config/CC_HAS_MULTIDIMENSIONAL_NONSTRING) \
    $(wildcard include/config/UBSAN_INTEGER_WRAP) \
    $(wildcard include/config/CC_HAS_ASM_INLINE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler_attributes.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler-gcc.h \
    $(wildcard include/config/ARCH_USE_BUILTIN_BSWAP) \
    $(wildcard include/config/SHADOW_CALL_STACK) \
    $(wildcard include/config/KCOV) \
    $(wildcard include/config/CC_HAS_TYPEOF_UNQUAL) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/module.h \
    $(wildcard include/config/MODULES) \
    $(wildcard include/config/SYSFS) \
    $(wildcard include/config/MODULES_TREE_LOOKUP) \
    $(wildcard include/config/LIVEPATCH) \
    $(wildcard include/config/STACKTRACE_BUILD_ID) \
    $(wildcard include/config/ARCH_USES_CFI_TRAPS) \
    $(wildcard include/config/MODULE_SIG) \
    $(wildcard include/config/GENERIC_BUG) \
    $(wildcard include/config/KALLSYMS) \
    $(wildcard include/config/SMP) \
    $(wildcard include/config/TRACEPOINTS) \
    $(wildcard include/config/TREE_SRCU) \
    $(wildcard include/config/BPF_EVENTS) \
    $(wildcard include/config/DEBUG_INFO_BTF_MODULES) \
    $(wildcard include/config/JUMP_LABEL) \
    $(wildcard include/config/TRACING) \
    $(wildcard include/config/EVENT_TRACING) \
    $(wildcard include/config/FTRACE_MCOUNT_RECORD) \
    $(wildcard include/config/KPROBES) \
    $(wildcard include/config/HAVE_STATIC_CALL_INLINE) \
    $(wildcard include/config/KUNIT) \
    $(wildcard include/config/PRINTK_INDEX) \
    $(wildcard include/config/MODULE_UNLOAD) \
    $(wildcard include/config/CONSTRUCTORS) \
    $(wildcard include/config/FUNCTION_ERROR_INJECTION) \
    $(wildcard include/config/DYNAMIC_DEBUG_CORE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/list.h \
    $(wildcard include/config/LIST_HARDENED) \
    $(wildcard include/config/DEBUG_LIST) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/container_of.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/build_bug.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler.h \
    $(wildcard include/config/TRACE_BRANCH_PROFILING) \
    $(wildcard include/config/PROFILE_ALL_BRANCHES) \
    $(wildcard include/config/OBJTOOL) \
    $(wildcard include/config/CFI_CLANG) \
    $(wildcard include/config/64BIT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/rwonce.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/rwonce.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kasan-checks.h \
    $(wildcard include/config/KASAN_GENERIC) \
    $(wildcard include/config/KASAN_SW_TAGS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/types.h \
    $(wildcard include/config/HAVE_UID16) \
    $(wildcard include/config/UID16) \
    $(wildcard include/config/ARCH_DMA_ADDR_T_64BIT) \
    $(wildcard include/config/PHYS_ADDR_T_64BIT) \
    $(wildcard include/config/ARCH_32BIT_USTAT_F_TINODE) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/types.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/types.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/int-ll64.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/int-ll64.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/bitsperlong.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitsperlong.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/bitsperlong.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/posix_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/stddef.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/stddef.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/posix_types.h \
    $(wildcard include/config/X86_32) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/posix_types_64.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/posix_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kcsan-checks.h \
    $(wildcard include/config/KCSAN) \
    $(wildcard include/config/KCSAN_WEAK_MEMORY) \
    $(wildcard include/config/KCSAN_IGNORE_ATOMICS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/poison.h \
    $(wildcard include/config/ILLEGAL_POINTER_VALUE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/const.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/const.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/const.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/barrier.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/alternative.h \
    $(wildcard include/config/CALL_THUNKS) \
    $(wildcard include/config/MITIGATION_ITS) \
    $(wildcard include/config/MITIGATION_RETHUNK) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/stringify.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/objtool.h \
    $(wildcard include/config/FRAME_POINTER) \
    $(wildcard include/config/NOINSTR_VALIDATION) \
    $(wildcard include/config/MITIGATION_UNRET_ENTRY) \
    $(wildcard include/config/MITIGATION_SRSO) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/objtool_types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/asm.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/extable_fixup_types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/bug.h \
    $(wildcard include/config/DEBUG_BUGVERBOSE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/instrumentation.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bug.h \
    $(wildcard include/config/BUG) \
    $(wildcard include/config/GENERIC_BUG_RELATIVE_POINTERS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/once_lite.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/panic.h \
    $(wildcard include/config/PANIC_TIMEOUT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/printk.h \
    $(wildcard include/config/MESSAGE_LOGLEVEL_DEFAULT) \
    $(wildcard include/config/CONSOLE_LOGLEVEL_DEFAULT) \
    $(wildcard include/config/CONSOLE_LOGLEVEL_QUIET) \
    $(wildcard include/config/EARLY_PRINTK) \
    $(wildcard include/config/PRINTK) \
    $(wildcard include/config/DYNAMIC_DEBUG) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/stdarg.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/init.h \
    $(wildcard include/config/MEMORY_HOTPLUG) \
    $(wildcard include/config/HAVE_ARCH_PREL32_RELOCATIONS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kern_levels.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/linkage.h \
    $(wildcard include/config/ARCH_USE_SYM_ANNOTATIONS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/export.h \
    $(wildcard include/config/MODVERSIONS) \
    $(wildcard include/config/GENDWARFKSYMS) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/linkage.h \
    $(wildcard include/config/CALL_PADDING) \
    $(wildcard include/config/MITIGATION_SLS) \
    $(wildcard include/config/FUNCTION_PADDING_BYTES) \
    $(wildcard include/config/UML) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/ibt.h \
    $(wildcard include/config/X86_KERNEL_IBT) \
    $(wildcard include/config/FINEIBT_BHI) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/ratelimit_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bits.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/bits.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/bits.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/overflow.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/limits.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/limits.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/limits.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/param.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/param.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/param.h \
    $(wildcard include/config/HZ) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/param.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/spinlock_types_raw.h \
    $(wildcard include/config/DEBUG_SPINLOCK) \
    $(wildcard include/config/DEBUG_LOCK_ALLOC) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/spinlock_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/qspinlock_types.h \
    $(wildcard include/config/NR_CPUS) \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/qrwlock_types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/byteorder.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/byteorder/little_endian.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/byteorder/little_endian.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/swab.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/swab.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/swab.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/byteorder/generic.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/lockdep_types.h \
    $(wildcard include/config/PROVE_RAW_LOCK_NESTING) \
    $(wildcard include/config/LOCKDEP) \
    $(wildcard include/config/LOCK_STAT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/dynamic_debug.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/jump_label.h \
    $(wildcard include/config/HAVE_ARCH_JUMP_LABEL_RELATIVE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/cleanup.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/jump_label.h \
    $(wildcard include/config/HAVE_JUMP_LABEL_HACK) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/nops.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/barrier.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/stat.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/stat.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/stat.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/time.h \
    $(wildcard include/config/POSIX_TIMERS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/cache.h \
    $(wildcard include/config/ARCH_HAS_CACHE_LINE_SIZE) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/kernel.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/sysinfo.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/cache.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cache.h \
    $(wildcard include/config/X86_L1_CACHE_SHIFT) \
    $(wildcard include/config/X86_INTERNODE_CACHE_SHIFT) \
    $(wildcard include/config/X86_VSMP) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/math64.h \
    $(wildcard include/config/ARCH_SUPPORTS_INT128) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/math.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/div64.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/div64.h \
    $(wildcard include/config/CC_OPTIMIZE_FOR_PERFORMANCE) \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/math64.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/time64.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/time64.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/time.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/time_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/time32.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/timex.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/timex.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/timex.h \
    $(wildcard include/config/X86_TSC) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/processor.h \
    $(wildcard include/config/X86_VMX_FEATURE_NAMES) \
    $(wildcard include/config/X86_IOPL_IOPERM) \
    $(wildcard include/config/VM86) \
    $(wildcard include/config/X86_USER_SHADOW_STACK) \
    $(wildcard include/config/X86_DEBUG_FPU) \
    $(wildcard include/config/USE_X86_SEG_SUPPORT) \
    $(wildcard include/config/PARAVIRT_XXL) \
    $(wildcard include/config/CPU_SUP_AMD) \
    $(wildcard include/config/XEN) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/processor-flags.h \
    $(wildcard include/config/MITIGATION_PAGE_TABLE_ISOLATION) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/processor-flags.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mem_encrypt.h \
    $(wildcard include/config/ARCH_HAS_MEM_ENCRYPT) \
    $(wildcard include/config/AMD_MEM_ENCRYPT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/mem_encrypt.h \
    $(wildcard include/config/X86_MEM_ENCRYPT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/cc_platform.h \
    $(wildcard include/config/ARCH_HAS_CC_PLATFORM) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/math_emu.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/ptrace.h \
    $(wildcard include/config/PARAVIRT) \
    $(wildcard include/config/IA32_EMULATION) \
    $(wildcard include/config/X86_DEBUGCTLMSR) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/segment.h \
    $(wildcard include/config/XEN_PV) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/page_types.h \
    $(wildcard include/config/PHYSICAL_START) \
    $(wildcard include/config/PHYSICAL_ALIGN) \
    $(wildcard include/config/DYNAMIC_PHYSICAL_MASK) \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/page.h \
    $(wildcard include/config/PAGE_SHIFT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/page_64_types.h \
    $(wildcard include/config/KASAN) \
    $(wildcard include/config/RANDOMIZE_BASE) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/kaslr.h \
    $(wildcard include/config/RANDOMIZE_MEMORY) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/ptrace.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/ptrace-abi.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/paravirt_types.h \
    $(wildcard include/config/ZERO_CALL_USED_REGS) \
    $(wildcard include/config/PARAVIRT_DEBUG) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/desc_defs.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/pgtable_types.h \
    $(wildcard include/config/X86_INTEL_MEMORY_PROTECTION_KEYS) \
    $(wildcard include/config/X86_PAE) \
    $(wildcard include/config/MEM_SOFT_DIRTY) \
    $(wildcard include/config/HAVE_ARCH_USERFAULTFD_WP) \
    $(wildcard include/config/PGTABLE_LEVELS) \
    $(wildcard include/config/PROC_FS) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/pgtable_64_types.h \
    $(wildcard include/config/KMSAN) \
    $(wildcard include/config/DEBUG_KMAP_LOCAL_FORCE_MAP) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/sparsemem.h \
    $(wildcard include/config/SPARSEMEM) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/nospec-branch.h \
    $(wildcard include/config/CALL_THUNKS_DEBUG) \
    $(wildcard include/config/MITIGATION_CALL_DEPTH_TRACKING) \
    $(wildcard include/config/MITIGATION_IBPB_ENTRY) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/static_key.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cpufeatures.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/msr-index.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/unwind_hints.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/orc_types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/percpu.h \
    $(wildcard include/config/CC_HAS_NAMED_AS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/args.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/percpu.h \
    $(wildcard include/config/DEBUG_PREEMPT) \
    $(wildcard include/config/HAVE_SETUP_PER_CPU_AREA) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/threads.h \
    $(wildcard include/config/BASE_SMALL) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/percpu-defs.h \
    $(wildcard include/config/DEBUG_FORCE_WEAK_PER_CPU) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/asm-offsets.h \
  /home/user/Symbi-OS/linux-nodbg/include/generated/asm-offsets.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/GEN-for-each-reg.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/proto.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/ldt.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/sigcontext.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/current.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cpuid/api.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cpuid/types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/string.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/string_64.h \
    $(wildcard include/config/ARCH_HAS_UACCESS_FLUSHCACHE) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/paravirt.h \
    $(wildcard include/config/PARAVIRT_SPINLOCKS) \
    $(wildcard include/config/DEBUG_ENTRY) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bug.h \
    $(wildcard include/config/BUG_ON_DATA_CORRUPTION) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/cpumask.h \
    $(wildcard include/config/FORCE_NR_CPUS) \
    $(wildcard include/config/HOTPLUG_CPU) \
    $(wildcard include/config/DEBUG_PER_CPU_MAPS) \
    $(wildcard include/config/CPUMASK_OFFSTACK) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kernel.h \
    $(wildcard include/config/PREEMPT_VOLUNTARY_BUILD) \
    $(wildcard include/config/PREEMPT_DYNAMIC) \
    $(wildcard include/config/HAVE_PREEMPT_DYNAMIC_CALL) \
    $(wildcard include/config/HAVE_PREEMPT_DYNAMIC_KEY) \
    $(wildcard include/config/PREEMPT_) \
    $(wildcard include/config/DEBUG_ATOMIC_SLEEP) \
    $(wildcard include/config/MMU) \
    $(wildcard include/config/PROVE_LOCKING) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/align.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/align.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/array_size.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bitops.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/typecheck.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/generic-non-atomic.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/bitops.h \
    $(wildcard include/config/X86_CMOV) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/rmwcc.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/sched.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/arch_hweight.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/const_hweight.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/instrumented-atomic.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/instrumented.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kmsan-checks.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/instrumented-non-atomic.h \
    $(wildcard include/config/KCSAN_ASSUME_PLAIN_WRITES_ATOMIC) \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/instrumented-lock.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/le.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bitops/ext2-atomic-setbit.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/hex.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kstrtox.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/log2.h \
    $(wildcard include/config/ARCH_HAS_ILOG2_U32) \
    $(wildcard include/config/ARCH_HAS_ILOG2_U64) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/minmax.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sprintf.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/static_call_types.h \
    $(wildcard include/config/HAVE_STATIC_CALL) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/instruction_pointer.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/util_macros.h \
    $(wildcard include/config/FOO_SUSPEND) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/wordpart.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bitmap.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/errno.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/errno.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/errno.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/errno.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/errno-base.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/find.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/string.h \
    $(wildcard include/config/BINARY_PRINTF) \
    $(wildcard include/config/FORTIFY_SOURCE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/err.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/string.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/fortify-string.h \
    $(wildcard include/config/CC_HAS_KASAN_MEMINTRINSIC_PREFIX) \
    $(wildcard include/config/GENERIC_ENTRY) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bitfield.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bitmap-str.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/cpumask_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/atomic.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/atomic.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cmpxchg.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cmpxchg_64.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/atomic64_64.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/atomic/atomic-arch-fallback.h \
    $(wildcard include/config/GENERIC_ATOMIC64) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/atomic/atomic-long.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/atomic/atomic-instrumented.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/gfp_types.h \
    $(wildcard include/config/KASAN_HW_TAGS) \
    $(wildcard include/config/SLAB_OBJ_EXT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/numa.h \
    $(wildcard include/config/NUMA_KEEP_MEMINFO) \
    $(wildcard include/config/NUMA) \
    $(wildcard include/config/HAVE_ARCH_NODE_DEV_GROUP) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/nodemask.h \
    $(wildcard include/config/HIGHMEM) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/nodemask_types.h \
    $(wildcard include/config/NODES_SHIFT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/random.h \
    $(wildcard include/config/VMGENID) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/random.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/ioctl.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/ioctl.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/ioctl.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/ioctl.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/irqnr.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/irqnr.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/frame.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/page.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/page_64.h \
    $(wildcard include/config/DEBUG_VIRTUAL) \
    $(wildcard include/config/X86_VSYSCALL_EMULATION) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/range.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/memory_model.h \
    $(wildcard include/config/FLATMEM) \
    $(wildcard include/config/SPARSEMEM_VMEMMAP) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/pfn.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/getorder.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/special_insns.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/irqflags.h \
    $(wildcard include/config/TRACE_IRQFLAGS) \
    $(wildcard include/config/PREEMPT_RT) \
    $(wildcard include/config/IRQSOFF_TRACER) \
    $(wildcard include/config/PREEMPT_TRACER) \
    $(wildcard include/config/DEBUG_IRQFLAGS) \
    $(wildcard include/config/TRACE_IRQFLAGS_SUPPORT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/irqflags_types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/irqflags.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/fpu/types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/vmxfeatures.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/vdso/processor.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/shstk.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/personality.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/personality.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/tsc.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cpufeature.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/cpufeaturemasks.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/msr.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cpumask.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/msr.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/shared/msr.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/percpu.h \
    $(wildcard include/config/RANDOM_KMALLOC_CACHES) \
    $(wildcard include/config/PAGE_SIZE_4KB) \
    $(wildcard include/config/NEED_PER_CPU_PAGE_FIRST_CHUNK) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/alloc_tag.h \
    $(wildcard include/config/MEM_ALLOC_PROFILING_DEBUG) \
    $(wildcard include/config/MEM_ALLOC_PROFILING) \
    $(wildcard include/config/MEM_ALLOC_PROFILING_ENABLED_BY_DEFAULT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/codetag.h \
    $(wildcard include/config/CODE_TAGGING) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/preempt.h \
    $(wildcard include/config/PREEMPT_COUNT) \
    $(wildcard include/config/TRACE_PREEMPT_TOGGLE) \
    $(wildcard include/config/PREEMPTION) \
    $(wildcard include/config/PREEMPT_NOTIFIERS) \
    $(wildcard include/config/PREEMPT_NONE) \
    $(wildcard include/config/PREEMPT_VOLUNTARY) \
    $(wildcard include/config/PREEMPT) \
    $(wildcard include/config/PREEMPT_LAZY) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/preempt.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/smp.h \
    $(wildcard include/config/UP_LATE_INIT) \
    $(wildcard include/config/CSD_LOCK_WAIT_DEBUG) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/smp_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/llist.h \
    $(wildcard include/config/ARCH_HAVE_NMI_SAFE_CMPXCHG) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/thread_info.h \
    $(wildcard include/config/THREAD_INFO_IN_TASK) \
    $(wildcard include/config/ARCH_HAS_PREEMPT_LAZY) \
    $(wildcard include/config/HAVE_ARCH_WITHIN_STACK_FRAMES) \
    $(wildcard include/config/SH) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/restart_block.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/thread_info.h \
    $(wildcard include/config/X86_FRED) \
    $(wildcard include/config/COMPAT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/smp.h \
    $(wildcard include/config/DEBUG_NMI_SELFTEST) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mmdebug.h \
    $(wildcard include/config/DEBUG_VM) \
    $(wildcard include/config/DEBUG_VM_IRQSOFF) \
    $(wildcard include/config/DEBUG_VM_PGFLAGS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched.h \
    $(wildcard include/config/VIRT_CPU_ACCOUNTING_NATIVE) \
    $(wildcard include/config/SCHED_INFO) \
    $(wildcard include/config/SCHEDSTATS) \
    $(wildcard include/config/SCHED_CORE) \
    $(wildcard include/config/FAIR_GROUP_SCHED) \
    $(wildcard include/config/RT_GROUP_SCHED) \
    $(wildcard include/config/RT_MUTEXES) \
    $(wildcard include/config/UCLAMP_TASK) \
    $(wildcard include/config/UCLAMP_BUCKETS_COUNT) \
    $(wildcard include/config/KMAP_LOCAL) \
    $(wildcard include/config/SCHED_CLASS_EXT) \
    $(wildcard include/config/CGROUP_SCHED) \
    $(wildcard include/config/BLK_DEV_IO_TRACE) \
    $(wildcard include/config/PREEMPT_RCU) \
    $(wildcard include/config/TASKS_RCU) \
    $(wildcard include/config/TASKS_TRACE_RCU) \
    $(wildcard include/config/MEMCG_V1) \
    $(wildcard include/config/LRU_GEN) \
    $(wildcard include/config/COMPAT_BRK) \
    $(wildcard include/config/CGROUPS) \
    $(wildcard include/config/BLK_CGROUP) \
    $(wildcard include/config/PSI) \
    $(wildcard include/config/PAGE_OWNER) \
    $(wildcard include/config/EVENTFD) \
    $(wildcard include/config/ARCH_HAS_CPU_PASID) \
    $(wildcard include/config/X86_BUS_LOCK_DETECT) \
    $(wildcard include/config/TASK_DELAY_ACCT) \
    $(wildcard include/config/SYMBIOTE) \
    $(wildcard include/config/STACKPROTECTOR) \
    $(wildcard include/config/ARCH_HAS_SCALED_CPUTIME) \
    $(wildcard include/config/VIRT_CPU_ACCOUNTING_GEN) \
    $(wildcard include/config/NO_HZ_FULL) \
    $(wildcard include/config/POSIX_CPUTIMERS) \
    $(wildcard include/config/POSIX_CPU_TIMERS_TASK_WORK) \
    $(wildcard include/config/KEYS) \
    $(wildcard include/config/SYSVIPC) \
    $(wildcard include/config/DETECT_HUNG_TASK) \
    $(wildcard include/config/IO_URING) \
    $(wildcard include/config/AUDIT) \
    $(wildcard include/config/AUDITSYSCALL) \
    $(wildcard include/config/DEBUG_MUTEXES) \
    $(wildcard include/config/DETECT_HUNG_TASK_BLOCKER) \
    $(wildcard include/config/UBSAN) \
    $(wildcard include/config/UBSAN_TRAP) \
    $(wildcard include/config/COMPACTION) \
    $(wildcard include/config/TASK_XACCT) \
    $(wildcard include/config/CPUSETS) \
    $(wildcard include/config/X86_CPU_RESCTRL) \
    $(wildcard include/config/FUTEX) \
    $(wildcard include/config/PERF_EVENTS) \
    $(wildcard include/config/NUMA_BALANCING) \
    $(wildcard include/config/RSEQ) \
    $(wildcard include/config/DEBUG_RSEQ) \
    $(wildcard include/config/SCHED_MM_CID) \
    $(wildcard include/config/FAULT_INJECTION) \
    $(wildcard include/config/LATENCYTOP) \
    $(wildcard include/config/FUNCTION_GRAPH_TRACER) \
    $(wildcard include/config/MEMCG) \
    $(wildcard include/config/UPROBES) \
    $(wildcard include/config/BCACHE) \
    $(wildcard include/config/VMAP_STACK) \
    $(wildcard include/config/SECURITY) \
    $(wildcard include/config/BPF_SYSCALL) \
    $(wildcard include/config/GCC_PLUGIN_STACKLEAK) \
    $(wildcard include/config/X86_MCE) \
    $(wildcard include/config/KRETPROBES) \
    $(wildcard include/config/RETHOOK) \
    $(wildcard include/config/ARCH_HAS_PARANOID_L1D_FLUSH) \
    $(wildcard include/config/RV) \
    $(wildcard include/config/USER_EVENTS) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/sched.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/pid_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sem_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/shm.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/shmparam.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kmsan_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mutex_types.h \
    $(wildcard include/config/MUTEX_SPIN_ON_OWNER) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/osq_lock.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/spinlock_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rwlock_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/plist_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/hrtimer_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/timerqueue_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rbtree_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/timer_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/seccomp_types.h \
    $(wildcard include/config/SECCOMP) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/refcount_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/resource.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/resource.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/resource.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/resource.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/resource.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/latencytop.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/prio.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/signal_types.h \
    $(wildcard include/config/OLD_SIGACTION) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/signal.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/signal.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/signal.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/signal-defs.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/siginfo.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/siginfo.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/syscall_user_dispatch_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mm_types_task.h \
    $(wildcard include/config/ARCH_WANT_BATCHED_UNMAP_TLB_FLUSH) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/tlbbatch.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/netdevice_xmit.h \
    $(wildcard include/config/NET_EGRESS) \
    $(wildcard include/config/NET_ACT_MIRRED) \
    $(wildcard include/config/NF_DUP_NETDEV) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/task_io_accounting.h \
    $(wildcard include/config/TASK_IO_ACCOUNTING) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/posix-timers_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/rseq.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/seqlock_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kcsan.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rv.h \
    $(wildcard include/config/RV_REACTORS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/uidgid_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/tracepoint-defs.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/kmap_size.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/kmap_size.h \
    $(wildcard include/config/DEBUG_KMAP_LOCAL) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/ext.h \
    $(wildcard include/config/EXT_GROUP_SCHED) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/spinlock.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bottom_half.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/lockdep.h \
    $(wildcard include/config/DEBUG_LOCKING_API_SELFTESTS) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/mmiowb.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/mmiowb.h \
    $(wildcard include/config/MMIOWB) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/spinlock.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/qspinlock.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/qspinlock.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/qrwlock.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/qrwlock.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rwlock.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/spinlock_api_smp.h \
    $(wildcard include/config/INLINE_SPIN_LOCK) \
    $(wildcard include/config/INLINE_SPIN_LOCK_BH) \
    $(wildcard include/config/INLINE_SPIN_LOCK_IRQ) \
    $(wildcard include/config/INLINE_SPIN_LOCK_IRQSAVE) \
    $(wildcard include/config/INLINE_SPIN_TRYLOCK) \
    $(wildcard include/config/INLINE_SPIN_TRYLOCK_BH) \
    $(wildcard include/config/UNINLINE_SPIN_UNLOCK) \
    $(wildcard include/config/INLINE_SPIN_UNLOCK_BH) \
    $(wildcard include/config/INLINE_SPIN_UNLOCK_IRQ) \
    $(wildcard include/config/INLINE_SPIN_UNLOCK_IRQRESTORE) \
    $(wildcard include/config/GENERIC_LOCKBREAK) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rwlock_api_smp.h \
    $(wildcard include/config/INLINE_READ_LOCK) \
    $(wildcard include/config/INLINE_WRITE_LOCK) \
    $(wildcard include/config/INLINE_READ_LOCK_BH) \
    $(wildcard include/config/INLINE_WRITE_LOCK_BH) \
    $(wildcard include/config/INLINE_READ_LOCK_IRQ) \
    $(wildcard include/config/INLINE_WRITE_LOCK_IRQ) \
    $(wildcard include/config/INLINE_READ_LOCK_IRQSAVE) \
    $(wildcard include/config/INLINE_WRITE_LOCK_IRQSAVE) \
    $(wildcard include/config/INLINE_READ_TRYLOCK) \
    $(wildcard include/config/INLINE_WRITE_TRYLOCK) \
    $(wildcard include/config/INLINE_READ_UNLOCK) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK) \
    $(wildcard include/config/INLINE_READ_UNLOCK_BH) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK_BH) \
    $(wildcard include/config/INLINE_READ_UNLOCK_IRQ) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK_IRQ) \
    $(wildcard include/config/INLINE_READ_UNLOCK_IRQRESTORE) \
    $(wildcard include/config/INLINE_WRITE_UNLOCK_IRQRESTORE) \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/time32.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/time.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/uidgid.h \
    $(wildcard include/config/MULTIUSER) \
    $(wildcard include/config/USER_NS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/highuid.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/buildid.h \
    $(wildcard include/config/VMCORE_INFO) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kmod.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/umh.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/gfp.h \
    $(wildcard include/config/ZONE_DMA) \
    $(wildcard include/config/ZONE_DMA32) \
    $(wildcard include/config/ZONE_DEVICE) \
    $(wildcard include/config/CONTIG_ALLOC) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mmzone.h \
    $(wildcard include/config/ARCH_FORCE_MAX_ORDER) \
    $(wildcard include/config/PAGE_BLOCK_ORDER) \
    $(wildcard include/config/CMA) \
    $(wildcard include/config/MEMORY_ISOLATION) \
    $(wildcard include/config/ZSMALLOC) \
    $(wildcard include/config/UNACCEPTED_MEMORY) \
    $(wildcard include/config/IOMMU_SUPPORT) \
    $(wildcard include/config/SWAP) \
    $(wildcard include/config/HUGETLB_PAGE) \
    $(wildcard include/config/TRANSPARENT_HUGEPAGE) \
    $(wildcard include/config/LRU_GEN_STATS) \
    $(wildcard include/config/LRU_GEN_WALKS_MMU) \
    $(wildcard include/config/MEMORY_FAILURE) \
    $(wildcard include/config/PAGE_EXTENSION) \
    $(wildcard include/config/DEFERRED_STRUCT_PAGE_INIT) \
    $(wildcard include/config/HAVE_MEMORYLESS_NODES) \
    $(wildcard include/config/SPARSEMEM_EXTREME) \
    $(wildcard include/config/SPARSEMEM_VMEMMAP_PREINIT) \
    $(wildcard include/config/HAVE_ARCH_PFN_VALID) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/list_nulls.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/wait.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/seqlock.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mutex.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/debug_locks.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/pageblock-flags.h \
    $(wildcard include/config/HUGETLB_PAGE_SIZE_VARIABLE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/page-flags-layout.h \
  /home/user/Symbi-OS/linux-nodbg/include/generated/bounds.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mm_types.h \
    $(wildcard include/config/HAVE_ALIGNED_STRUCT_PAGE) \
    $(wildcard include/config/HUGETLB_PMD_PAGE_TABLE_SHARING) \
    $(wildcard include/config/SLAB_FREELIST_HARDENED) \
    $(wildcard include/config/USERFAULTFD) \
    $(wildcard include/config/ANON_VMA_NAME) \
    $(wildcard include/config/PER_VMA_LOCK) \
    $(wildcard include/config/HAVE_ARCH_COMPAT_MMAP_BASES) \
    $(wildcard include/config/MEMBARRIER) \
    $(wildcard include/config/FUTEX_PRIVATE_HASH) \
    $(wildcard include/config/AIO) \
    $(wildcard include/config/MMU_NOTIFIER) \
    $(wildcard include/config/SPLIT_PMD_PTLOCKS) \
    $(wildcard include/config/IOMMU_MM_DATA) \
    $(wildcard include/config/KSM) \
    $(wildcard include/config/MM_ID) \
    $(wildcard include/config/CORE_DUMP_DEFAULT_ELF_HEADERS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/auxvec.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/auxvec.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/auxvec.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kref.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/refcount.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rbtree.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rcupdate.h \
    $(wildcard include/config/TINY_RCU) \
    $(wildcard include/config/RCU_STRICT_GRACE_PERIOD) \
    $(wildcard include/config/RCU_LAZY) \
    $(wildcard include/config/RCU_STALL_COMMON) \
    $(wildcard include/config/KVM_XFER_TO_GUEST_WORK) \
    $(wildcard include/config/RCU_NOCB_CPU) \
    $(wildcard include/config/TASKS_RCU_GENERIC) \
    $(wildcard include/config/TASKS_RUDE_RCU) \
    $(wildcard include/config/TREE_RCU) \
    $(wildcard include/config/DEBUG_OBJECTS_RCU_HEAD) \
    $(wildcard include/config/PROVE_RCU) \
    $(wildcard include/config/ARCH_WEAK_RELEASE_ACQUIRE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/context_tracking_irq.h \
    $(wildcard include/config/CONTEXT_TRACKING_IDLE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rcutree.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/maple_tree.h \
    $(wildcard include/config/MAPLE_RCU_DISABLED) \
    $(wildcard include/config/DEBUG_MAPLE_TREE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rwsem.h \
    $(wildcard include/config/RWSEM_SPIN_ON_OWNER) \
    $(wildcard include/config/DEBUG_RWSEMS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/completion.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/swait.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/uprobes.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/timer.h \
    $(wildcard include/config/DEBUG_OBJECTS_TIMERS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/ktime.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/jiffies.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/jiffies.h \
  /home/user/Symbi-OS/linux-nodbg/include/generated/timeconst.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/ktime.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/timekeeping.h \
    $(wildcard include/config/GENERIC_CMOS_UPDATE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/clocksource_ids.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/debugobjects.h \
    $(wildcard include/config/DEBUG_OBJECTS) \
    $(wildcard include/config/DEBUG_OBJECTS_FREE) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/uprobes.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/notifier.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/srcu.h \
    $(wildcard include/config/TINY_SRCU) \
    $(wildcard include/config/NEED_SRCU_NMI_SAFE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/workqueue.h \
    $(wildcard include/config/DEBUG_OBJECTS_WORK) \
    $(wildcard include/config/FREEZER) \
    $(wildcard include/config/WQ_WATCHDOG) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/workqueue_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rcu_segcblist.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/srcutree.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rcu_node_tree.h \
    $(wildcard include/config/RCU_FANOUT) \
    $(wildcard include/config/RCU_FANOUT_LEAF) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/percpu_counter.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/mmu.h \
    $(wildcard include/config/MODIFY_LDT_SYSCALL) \
    $(wildcard include/config/ADDRESS_MASKING) \
    $(wildcard include/config/BROADCAST_TLB_FLUSH) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/page-flags.h \
    $(wildcard include/config/PAGE_IDLE_FLAG) \
    $(wildcard include/config/ARCH_USES_PG_ARCH_2) \
    $(wildcard include/config/ARCH_USES_PG_ARCH_3) \
    $(wildcard include/config/HUGETLB_PAGE_OPTIMIZE_VMEMMAP) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/local_lock.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/local_lock_internal.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/zswap.h \
    $(wildcard include/config/ZSWAP) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/memory_hotplug.h \
    $(wildcard include/config/ARCH_HAS_ADD_PAGES) \
    $(wildcard include/config/MEMORY_HOTREMOVE) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/mmzone.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/mmzone.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/topology.h \
    $(wildcard include/config/USE_PERCPU_NUMA_NODE_ID) \
    $(wildcard include/config/SCHED_SMT) \
    $(wildcard include/config/GENERIC_ARCH_TOPOLOGY) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/arch_topology.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/topology.h \
    $(wildcard include/config/X86_LOCAL_APIC) \
    $(wildcard include/config/SCHED_MC_PRIO) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/mpspec.h \
    $(wildcard include/config/EISA) \
    $(wildcard include/config/X86_MPPARSE) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/mpspec_def.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/x86_init.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/apicdef.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/topology.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/cpu_smt.h \
    $(wildcard include/config/HOTPLUG_SMT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sysctl.h \
    $(wildcard include/config/SYSCTL) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/sysctl.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/elf.h \
    $(wildcard include/config/ARCH_HAVE_EXTRA_ELF_NOTES) \
    $(wildcard include/config/ARCH_USE_GNU_PROPERTY) \
    $(wildcard include/config/ARCH_HAVE_ELF_PROT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/elf.h \
    $(wildcard include/config/X86_X32_ABI) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/ia32.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compat.h \
    $(wildcard include/config/ARCH_HAS_SYSCALL_WRAPPER) \
    $(wildcard include/config/COMPAT_OLD_SIGACTION) \
    $(wildcard include/config/HARDENED_USERCOPY) \
    $(wildcard include/config/ODD_RT_SIGACTION) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sem.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/sem.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/ipc.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rhashtable-types.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/ipc.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/ipcbuf.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/ipcbuf.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/sembuf.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/socket.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/socket.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/socket.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/sockios.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/sockios.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/sockios.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/uio.h \
    $(wildcard include/config/ARCH_HAS_COPY_MC) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/ucopysize.h \
    $(wildcard include/config/HARDENED_USERCOPY_DEFAULT_ON) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/uio.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/socket.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/if.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/libc-compat.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/hdlc/ioctl.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/fs.h \
    $(wildcard include/config/FANOTIFY_ACCESS_PERMISSIONS) \
    $(wildcard include/config/READ_ONLY_THP_FOR_FS) \
    $(wildcard include/config/FS_POSIX_ACL) \
    $(wildcard include/config/CGROUP_WRITEBACK) \
    $(wildcard include/config/IMA) \
    $(wildcard include/config/FILE_LOCKING) \
    $(wildcard include/config/FSNOTIFY) \
    $(wildcard include/config/FS_ENCRYPTION) \
    $(wildcard include/config/FS_VERITY) \
    $(wildcard include/config/EPOLL) \
    $(wildcard include/config/UNICODE) \
    $(wildcard include/config/QUOTA) \
    $(wildcard include/config/FS_DAX) \
    $(wildcard include/config/BLOCK) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/vfsdebug.h \
    $(wildcard include/config/DEBUG_VFS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/wait_bit.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kdev_t.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/kdev_t.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/dcache.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rculist.h \
    $(wildcard include/config/PROVE_RCU_LIST) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rculist_bl.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/list_bl.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bit_spinlock.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/lockref.h \
    $(wildcard include/config/ARCH_USE_CMPXCHG_LOCKREF) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/stringhash.h \
    $(wildcard include/config/DCACHE_WORD_ACCESS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/hash.h \
    $(wildcard include/config/HAVE_ARCH_HASH) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/path.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/list_lru.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/shrinker.h \
    $(wildcard include/config/SHRINKER_DEBUG) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/xarray.h \
    $(wildcard include/config/XARRAY_MULTI) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/mm.h \
    $(wildcard include/config/MMU_LAZY_TLB_REFCOUNT) \
    $(wildcard include/config/ARCH_HAS_MEMBARRIER_CALLBACKS) \
    $(wildcard include/config/ARCH_HAS_SYNC_CORE_BEFORE_USERMODE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sync_core.h \
    $(wildcard include/config/ARCH_HAS_PREPARE_SYNC_CORE_CMD) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/sync_core.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/coredump.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/radix-tree.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/pid.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/capability.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/capability.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/semaphore.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/fcntl.h \
    $(wildcard include/config/ARCH_32BIT_OFF_T) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/fcntl.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/fcntl.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/asm-generic/fcntl.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/openat2.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/migrate_mode.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/percpu-rwsem.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rcuwait.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/signal.h \
    $(wildcard include/config/SCHED_AUTOGROUP) \
    $(wildcard include/config/BSD_PROCESS_ACCT) \
    $(wildcard include/config/TASKSTATS) \
    $(wildcard include/config/STACK_GROWSUP) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/signal.h \
    $(wildcard include/config/DYNAMIC_SIGFRAME) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/jobctl.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/task.h \
    $(wildcard include/config/HAVE_EXIT_THREAD) \
    $(wildcard include/config/ARCH_WANTS_DYNAMIC_TASK_STRUCT) \
    $(wildcard include/config/HAVE_ARCH_THREAD_STRUCT_WHITELIST) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/uaccess.h \
    $(wildcard include/config/ARCH_HAS_SUBPAGE_FAULTS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/fault-inject-usercopy.h \
    $(wildcard include/config/FAULT_INJECTION_USERCOPY) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/nospec.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/uaccess.h \
    $(wildcard include/config/CC_HAS_ASM_GOTO_OUTPUT) \
    $(wildcard include/config/CC_HAS_ASM_GOTO_TIED_OUTPUT) \
    $(wildcard include/config/X86_INTEL_USERCOPY) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mmap_lock.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/smap.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/extable.h \
    $(wildcard include/config/BPF_JIT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/tlbflush.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mmu_notifier.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/interval_tree.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/invpcid.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/pti.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/pgtable.h \
    $(wildcard include/config/DEBUG_WX) \
    $(wildcard include/config/HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD) \
    $(wildcard include/config/ARCH_HAS_PTE_DEVMAP) \
    $(wildcard include/config/ARCH_SUPPORTS_PMD_PFNMAP) \
    $(wildcard include/config/ARCH_SUPPORTS_PUD_PFNMAP) \
    $(wildcard include/config/HAVE_ARCH_SOFT_DIRTY) \
    $(wildcard include/config/ARCH_ENABLE_THP_MIGRATION) \
    $(wildcard include/config/PAGE_TABLE_CHECK) \
    $(wildcard include/config/X86_SGX) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/pkru.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/fpu/api.h \
    $(wildcard include/config/MATH_EMULATION) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/coco.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/pgtable_uffd.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/page_table_check.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/pgtable_64.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/fixmap.h \
    $(wildcard include/config/PROVIDE_OHCI1394_DMA_INIT) \
    $(wildcard include/config/X86_IO_APIC) \
    $(wildcard include/config/PCI_MMCONFIG) \
    $(wildcard include/config/ACPI_APEI_GHES) \
    $(wildcard include/config/INTEL_TXT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/vsyscall.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/fixmap.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/pgtable-invert.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/uaccess_64.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/runtime-const.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/access_ok.h \
    $(wildcard include/config/ALTERNATE_USER_ADDRESS_SPACE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/cred.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/key.h \
    $(wildcard include/config/KEY_NOTIFICATIONS) \
    $(wildcard include/config/NET) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/assoc_array.h \
    $(wildcard include/config/ASSOCIATIVE_ARRAY) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/user.h \
    $(wildcard include/config/VFIO_PCI_ZDEV_KVM) \
    $(wildcard include/config/IOMMUFD) \
    $(wildcard include/config/WATCH_QUEUE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/ratelimit.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/posix-timers.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/alarmtimer.h \
    $(wildcard include/config/RTC_CLASS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/hrtimer.h \
    $(wildcard include/config/HIGH_RES_TIMERS) \
    $(wildcard include/config/TIME_LOW_RES) \
    $(wildcard include/config/TIMERFD) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/hrtimer_defs.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/timerqueue.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rcuref.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rcu_sync.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/delayed_call.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/uuid.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/errseq.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/ioprio.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/rt.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/iocontext.h \
    $(wildcard include/config/BLK_ICQ) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/ioprio.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/fs_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mount.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/mnt_idmapping.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/slab.h \
    $(wildcard include/config/FAILSLAB) \
    $(wildcard include/config/KFENCE) \
    $(wildcard include/config/SLUB_TINY) \
    $(wildcard include/config/SLUB_DEBUG) \
    $(wildcard include/config/SLAB_BUCKETS) \
    $(wildcard include/config/KVFREE_RCU_BATCHED) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/percpu-refcount.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kasan.h \
    $(wildcard include/config/KASAN_STACK) \
    $(wildcard include/config/KASAN_VMALLOC) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kasan-enabled.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kasan-tags.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rw_hint.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/file_ref.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/unicode.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/fs.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/quota.h \
    $(wildcard include/config/QUOTA_NETLINK_INTERFACE) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/dqblk_xfs.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/dqblk_v1.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/dqblk_v2.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/dqblk_qtree.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/projid.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/quota.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/aio_abi.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/unistd.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/unistd.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi/asm/unistd.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi/asm/unistd_64.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/unistd_64_x32.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/unistd_32_ia32.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/compat.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sched/task_stack.h \
    $(wildcard include/config/DEBUG_STACK_USAGE) \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/magic.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/user32.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/compat.h \
    $(wildcard include/config/COMPAT_FOR_U64_ALIGNMENT) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/syscall_wrapper.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/user.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/user_64.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/fsgsbase.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/vdso.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/elf.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/elf-em.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kobject.h \
    $(wildcard include/config/UEVENT_HELPER) \
    $(wildcard include/config/DEBUG_KOBJECT_RELEASE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/sysfs.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kernfs.h \
    $(wildcard include/config/KERNFS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/idr.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kobject_ns.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/moduleparam.h \
    $(wildcard include/config/ALPHA) \
    $(wildcard include/config/PPC64) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/rbtree_latch.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/error-injection.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/error-injection.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/module.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/module.h \
    $(wildcard include/config/HAVE_MOD_ARCH_SPECIFIC) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/build-salt.h \
    $(wildcard include/config/BUILD_SALT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/elfnote.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/elfnote-lto.h \
    $(wildcard include/config/LTO) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/vermagic.h \
    $(wildcard include/config/PREEMPT_BUILD) \
  /home/user/Symbi-OS/linux-nodbg/include/generated/utsrelease.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/vermagic.h \
    $(wildcard include/config/M486SX) \
    $(wildcard include/config/M486) \
    $(wildcard include/config/M586) \
    $(wildcard include/config/M586TSC) \
    $(wildcard include/config/M586MMX) \
    $(wildcard include/config/MATOM) \
    $(wildcard include/config/M686) \
    $(wildcard include/config/MPENTIUMII) \
    $(wildcard include/config/MPENTIUMIII) \
    $(wildcard include/config/MPENTIUMM) \
    $(wildcard include/config/MPENTIUM4) \
    $(wildcard include/config/MK6) \
    $(wildcard include/config/MK7) \
    $(wildcard include/config/MELAN) \
    $(wildcard include/config/MCRUSOE) \
    $(wildcard include/config/MEFFICEON) \
    $(wildcard include/config/MWINCHIPC6) \
    $(wildcard include/config/MWINCHIP3D) \
    $(wildcard include/config/MCYRIXIII) \
    $(wildcard include/config/MVIAC3_2) \
    $(wildcard include/config/MVIAC7) \
    $(wildcard include/config/MGEODEGX1) \
    $(wildcard include/config/MGEODE_LX) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/orc_header.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/orc_hash.h \

.module-common.o: $(deps_.module-common.o)

$(deps_.module-common.o):

.module-common.o: $(wildcard /home/user/Symbi-OS/linux-nodbg/tools/objtool/objtool)
