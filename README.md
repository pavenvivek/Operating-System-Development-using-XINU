# This repository contains code for Operating System development using Xinu

# Build instructions

Copy the file compile/Makedefs.EXAMPLE to compile/Makedefs and make appropriate changes if necessary.  Make sure that the correct COMPILER_ROOT, LIBGCC_LOC and CONF_LFLAGS are set.

The PLATFORM variable should be set to one of:

- arm-qemu
- arm-bbb
- x86-qemu
- x86-galileo

The code implements different operating system components, such as File System, Network Management, Synchronization, Thread Management, etc. This work is part of P536 Advanced Operating System coursework. The files created & edited as part of the project includes the following:

- all files in apps folder
- xsh_hello.c, xsh_prodcons.c, xsh_run.c in shell folder
- future.c, fs.c, ufu.c in system folder
