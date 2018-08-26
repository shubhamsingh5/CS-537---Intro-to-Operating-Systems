#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "pstat.h"

int main(int argc, char const *argv[]) {
  int x = settickets(20);
  printf(1, "error: %d\n", x);
  struct pstat * stat;
  stat = malloc(sizeof(*stat));
  int y = getpinfo(stat);
  for (int i = 0; i < NPROC; i++) {
    if (stat->inuse[i] == 1) {
      printf(1, "%d\n", stat->tickets[i]);
    }
  }
  y = y+0;
  exit();
}
