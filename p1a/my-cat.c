#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  FILE *fp;

  //no file specified
  if (argc == 1) {
    exit(0);
  } else {
    for (int i = 1; i < argc; i++) {  //file not opened
      if (!(fp = fopen(argv[i], "r"))) {
        printf("my-cat: cannot open file\n");
        exit(1);
      } else {  //file opened
        char buffer[100];
        while (fgets(buffer, 100, fp)) {
          printf("%s", buffer);
        }
        fclose(fp);
      }
    }
    exit(0);
  }
}
