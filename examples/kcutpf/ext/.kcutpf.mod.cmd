savedcmd_kcutpf.mod := printf '%s\n'   pfadaptor.o pfasm.o init.o | awk '!x[$$0]++ { print("./"$$0) }' > kcutpf.mod
