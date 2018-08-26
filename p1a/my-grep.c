#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  FILE *fp;
  char *buffer;
  size_t buffersize = 32;
  buffer = (char *)malloc(buffersize * sizeof(char));

  //no command input arguments
  if (argc == 1) {
    printf("my-grep: searchterm [file ...]\n");
    exit(1);
  } else {
    if (argc == 2) {  //no file specified
      while (fgets(buffer, 100, stdin)){
        if (strstr(buffer, argv[1])) {
          printf("%s", buffer);
        }
      }
    }
    for (int i = 2; i < argc; i++) {
      if (!(fp = fopen(argv[i], "r"))) {  //file not opened
        printf("my-grep: cannot open file\n");
        exit(1);
      } else {  //file opened
        while (getinput(&buffer,&buffersize,fp) != EOF) {
          //look for occurrence of search term
          if (strstr(buffer, argv[1])) {
            printf("%s", buffer);
          }
        }
        fclose(fp);
      }
    }
    exit(0);
  }
}
