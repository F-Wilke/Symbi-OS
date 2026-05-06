savedcmd_kcutpf.o := ld -m elf_x86_64 -z noexecstack --no-warn-rwx-segments   -r -o kcutpf.o @kcutpf.mod  ; /home/user/Symbi-OS/linux-nodbg/tools/objtool/objtool --hacks=jump_label --hacks=noinstr --hacks=skylake --ibt --orc --retpoline --rethunk --sls --static-call --uaccess --prefix=16  --link  --module kcutpf.o

kcutpf.o: $(wildcard /home/user/Symbi-OS/linux-nodbg/tools/objtool/objtool)
