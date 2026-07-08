# kldso

kld.so is a elf binary interpreter that is meant to front end any standard
interpreter.   It uses to additonaly elf sections to drive its core behaviour and 
environment variables to control optional aspects.

It's core function is to perpare the kernel side extensions/adaptors for
a dynamically privileged binary.  

Given that it functions as a elf binary interpreter it cannot utilize
external shared libraries including libc.  As such it functionality 
is implemented in a standalone fashion -- all system calls go through
its own stub code.  Much of the stub code was developed with codex and claude.
The core logic however was first developed in kld and then migrated 
to kld.so.  Intially kld.so was execing a shell script kldso.sh which then
invoked kld with the original arguments.  This approach, however functional, was
not transparent and tools like gdb would not act as expected.

Execution begins with the `_start` code in kldso_entry.S

## `_start@kldso_entry.S`

This code is responsible for preserving the initial kernel provided
stack and register contents so that when it maps and invokes the 
original interpret it begins as if the kernel had directly launched it.

The following outlines the logic of `_start`

1. clear frame pointer (rbp = 0)
2. save stack pointer (kld_init_rsp = rsp)
3. load c main arguments from stack into registers for call to `c_entry`
4. align rsp to 16 bytes (as per abi requirements)
5. call `c_entry`
6. If we get back here, which we do not expect, then invoke exit system call

## `c_entry@kldso_main.c`

1. calculate envp based on argv and argc (this is so that we can both
   pickup the env variables that we consult and so that we can 
   extend the env that we pass on to the "real" interpret
2. pickup values for the following from env
   - `debug=getenv(KLD_DEBUG)`
   - `noexec=getenv(KLD_NOEXEC)`
   - `kld_library_path=getenv(KLD_LIBRARY_PATH)`
   - `old_ld_library_path=getenv(LD_LIBRARY_PATH)`

Please finish me ;-)

