savedcmd_pfasm.o := gcc -Wp,-MMD,./.pfasm.o.d -nostdinc -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated -I/home/user/Symbi-OS/linux-nodbg/include -I/home/user/Symbi-OS/linux-nodbg/include -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include/uapi -I/home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/uapi -I/home/user/Symbi-OS/linux-nodbg/include/uapi -I/home/user/Symbi-OS/linux-nodbg/include/generated/uapi -include /home/user/Symbi-OS/linux-nodbg/include/linux/compiler-version.h -include /home/user/Symbi-OS/linux-nodbg/include/linux/kconfig.h -D__KERNEL__ -D__ASSEMBLY__ -fno-PIE -m64 -DCC_USING_FENTRY -g  -DMODULE  -DKBUILD_MODNAME='"kcutpf"' -D__KBUILD_MODNAME=kmod_kcutpf -c -o pfasm.o pfasm.S 

source_pfasm.o := pfasm.S

deps_pfasm.o := \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler-version.h \
    $(wildcard include/config/CC_VERSION_TEXT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/kconfig.h \
    $(wildcard include/config/CPU_BIG_ENDIAN) \
    $(wildcard include/config/BOOGER) \
    $(wildcard include/config/FOO) \
  /home/user/Symbi-OS/linux-nodbg/include/generated/asm-offsets.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/percpu.h \
    $(wildcard include/config/X86_64) \
    $(wildcard include/config/SMP) \
    $(wildcard include/config/CC_HAS_NAMED_AS) \
    $(wildcard include/config/USE_X86_SEG_SUPPORT) \
    $(wildcard include/config/X86_32) \
    $(wildcard include/config/UML) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/asm.h \
    $(wildcard include/config/KPROBES) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/extable_fixup_types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/nospec-branch.h \
    $(wildcard include/config/CALL_THUNKS_DEBUG) \
    $(wildcard include/config/MITIGATION_CALL_DEPTH_TRACKING) \
    $(wildcard include/config/NOINSTR_VALIDATION) \
    $(wildcard include/config/MITIGATION_UNRET_ENTRY) \
    $(wildcard include/config/MITIGATION_SRSO) \
    $(wildcard include/config/MITIGATION_RETPOLINE) \
    $(wildcard include/config/MITIGATION_RETHUNK) \
    $(wildcard include/config/MITIGATION_IBPB_ENTRY) \
    $(wildcard include/config/MITIGATION_ITS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/static_key.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/jump_label.h \
    $(wildcard include/config/JUMP_LABEL) \
    $(wildcard include/config/HAVE_ARCH_JUMP_LABEL_RELATIVE) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/jump_label.h \
    $(wildcard include/config/HAVE_JUMP_LABEL_HACK) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/nops.h \
    $(wildcard include/config/64BIT) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/objtool.h \
    $(wildcard include/config/OBJTOOL) \
    $(wildcard include/config/FRAME_POINTER) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/objtool_types.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/linkage.h \
    $(wildcard include/config/FUNCTION_ALIGNMENT) \
    $(wildcard include/config/ARCH_USE_SYM_ANNOTATIONS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler_types.h \
    $(wildcard include/config/DEBUG_INFO_BTF) \
    $(wildcard include/config/PAHOLE_HAS_BTF_TAG) \
    $(wildcard include/config/CC_HAS_SANE_FUNCTION_ALIGNMENT) \
    $(wildcard include/config/ARM64) \
    $(wildcard include/config/LD_DEAD_CODE_DATA_ELIMINATION) \
    $(wildcard include/config/LTO_CLANG) \
    $(wildcard include/config/HAVE_ARCH_COMPILER_H) \
    $(wildcard include/config/CC_HAS_COUNTED_BY) \
    $(wildcard include/config/CC_HAS_MULTIDIMENSIONAL_NONSTRING) \
    $(wildcard include/config/UBSAN_INTEGER_WRAP) \
    $(wildcard include/config/CC_HAS_ASM_INLINE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/stringify.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/export.h \
    $(wildcard include/config/MODVERSIONS) \
    $(wildcard include/config/GENDWARFKSYMS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/compiler.h \
    $(wildcard include/config/TRACE_BRANCH_PROFILING) \
    $(wildcard include/config/PROFILE_ALL_BRANCHES) \
    $(wildcard include/config/CFI_CLANG) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/generated/asm/rwonce.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/rwonce.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/linkage.h \
    $(wildcard include/config/CALL_PADDING) \
    $(wildcard include/config/MITIGATION_SLS) \
    $(wildcard include/config/FUNCTION_PADDING_BYTES) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/ibt.h \
    $(wildcard include/config/X86_KERNEL_IBT) \
    $(wildcard include/config/FINEIBT_BHI) \
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
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/alternative.h \
    $(wildcard include/config/CALL_THUNKS) \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/bug.h \
    $(wildcard include/config/GENERIC_BUG) \
    $(wildcard include/config/DEBUG_BUGVERBOSE) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/instrumentation.h \
  /home/user/Symbi-OS/linux-nodbg/include/asm-generic/bug.h \
    $(wildcard include/config/BUG) \
    $(wildcard include/config/GENERIC_BUG_RELATIVE_POINTERS) \
  /home/user/Symbi-OS/linux-nodbg/include/linux/once_lite.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/cpufeatures.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/msr-index.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/bits.h \
  /home/user/Symbi-OS/linux-nodbg/include/linux/const.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/const.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/const.h \
  /home/user/Symbi-OS/linux-nodbg/include/vdso/bits.h \
  /home/user/Symbi-OS/linux-nodbg/include/uapi/linux/bits.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/unwind_hints.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/orc_types.h \
  /home/user/Symbi-OS/linux-nodbg/arch/x86/include/asm/asm-offsets.h \

pfasm.o: $(deps_pfasm.o)

$(deps_pfasm.o):
