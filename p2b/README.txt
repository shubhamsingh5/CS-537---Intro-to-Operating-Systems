Shubham Singh
singh68@wisc.edu
9075214321

For this project I first implemented the system calls by doing the following:
  - implement function in proc.c
  - add function implementation to proc.c
  - add system call to sysproc.c
  - add syscall def to syscall.h
  - add declaration to defs.h
  - add declaration to user.h
  - add to usys.S
  - add def to syscall.c

Then I implemented the scheduler, which basically involved finding the total number of tickets, picking a random winner, and looping through the processes till ticket count matched winner.

For my random number generator, I used a linear congruential generator, which I seeded with a counter that incremented on each iteration of the scheduler loop.


