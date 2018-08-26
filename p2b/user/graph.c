#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

void spin(int a)
{
  settickets(a);
	int i = 0;
  int j = 0;
  int k = 0;
	for(i = 0; i < 50; ++i)
	{
		for(j = 0; j < 500000; ++j)
		{
      k = j % 10;
      k = k + 1;
    }
	}
}


int
main(int argc, char *argv[])
{
   struct pstat st;
   int count = 0;
   int i = 1;
   int pid[NPROC];
   printf(1,"Spinning...\n");
   while(i < 4)
   {
      pid[i] = fork();
	    if(pid[i] == 0)
     {
		    spin(47*i);
		    exit();
      }
	  i++;
   }
   sleep(500);
   //spin();
   getpinfo(&st);
   for(i = 0; i < NPROC; i++) {
      if (st.inuse[i]) {
	       count++;
         printf(1, "pid: %d tickets: %d ticks: %d\n", st.pid[i], st.tickets[i], st.ticks[i]);
      }
   }
   for(i = 0; i < 4; i++)
   {
	    kill(pid[i]);
   }
   while (wait() > 0);
   exit();
}
