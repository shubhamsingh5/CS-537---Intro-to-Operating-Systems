#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  FILE *fp;
  char *buffer;
  size_t buffersize = 32;
  buffer = (char *)malloc(buffersize * sizeof(char));
  int count;
  char *output = (char *)malloc(buffersize* sizeof(char));
  // unsigned char bytes[4];

  if (argc == 1) {
    printf("my-zip: file1 [file2 ...]\n");
    exit(1);
  } else {
    for (int i = 1; i < argc; i++) {
      if ((fp = fopen(argv[i], "r"))) {
        while (getinput(&buffer,&buffersize,fp) != EOF) {
          char * new_str;
          if((new_str = malloc(strlen(output)+strlen(buffer)+1)) != NULL){
            new_str[0] = '\0'
            strcat(new_str,output);
            strcat(new_str,buffer);
            output = (char *)realloc(output, strlen(new_str)+1);
            strcpy(output, new_str);
          }
        }
      }
    }
    printf("%s", output);
    int len = strlen(output);
    for (int j = 0; j < len; j++) {
      count = 1;
      while (j + 1 < len && output[j] == output[j+1]) {
        count++;
        j++;
      }
      // printf("%d", count);
      // printf("%c", buffer[j]);
      fwrite(&count, sizeof(int), 1, stdout);
      printf("%c", output[j]);
    }
  }
  exit(0);
}
