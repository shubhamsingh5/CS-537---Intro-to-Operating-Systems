Project 3b:
Shubham Singh, Yu-Lin Yang

For the NULL pointer dereference, we changed the program start address form 0 to pagesize in exec.c, changed the copyuvm method in vm.c to start looping from pagesize instead of 0 and changed the program load address from 0x0 to 0x1000 in the user makefile. We added checks for NULL pointers in syscall.c

For the page table protection, we first implemented the system calls, making the function sys_mprotect and sys_munprotect in sysproc.c call a wrapper function in vm.c. The wrapper functions called walkpgdir to obtain the page table entry, then changed the protection bits.
