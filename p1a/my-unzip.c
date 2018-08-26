#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  FILE *fp;
  int count = 0;

  //no command input arguments
  if (argc == 1) {
    printf("my-unzip: file1 [file2 ...]\n");
    exit(1);
  } else {
    for (int i = 1; i < argc; i++) {  //file opened
      if ((fp = fopen(argv[i], "r"))) {
        //get count
        while ((fread(&count, sizeof(int), 1, fp) != 0)) {
          //get char
          int c = fgetc(fp);
          for (int j = 0; j < count; j++) {
            printf("%c", c);
          }
        }
      } else {
        printf("my-unzip: cannot open file\n"); //file could not be opened
        exit(1);
      }
      fclose(fp);
    }
    exit(0);
  }
}
