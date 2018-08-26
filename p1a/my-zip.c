#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  FILE *fp;
  size_t buffersize = 32;
  char *buffer = NULL;
  int count = 1;
  int size = 0;
  char *output = (char *)malloc(buffersize* sizeof(char));

  //no command input arguments
  if (argc == 1) {
    printf("my-zip: file1 [file2 ...]\n");
    exit(1);
  } else {
    for (int i = 1; i < argc; i++) {  //file opened
      if ((fp = fopen(argv[i], "r"))) {
        //get file size
        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        //change size of buffer string based on file size
        buffer = (char *) realloc(buffer, size+1);
        int numchars = fread(buffer, sizeof(char), size, fp);
        //temp string
        char * new_str;
        //change size of new string based on buffer
        if((new_str = malloc(strlen(output)+numchars+1)) != NULL){
          new_str[0] = '\0';
          //combine new file with old file
          strcat(new_str,output);
          strcat(new_str,buffer);
          output = (char *)realloc(output, strlen(new_str)+1);
          //copy temp string into output string
          strcpy(output, new_str);
        }
        fclose(fp);
      } else {
        printf("my-zip: cannot open file\n");  //file could not be opened
        exit(1);
      }
    }
    int len = strlen(output);
    //loop over combined output string
    for (int j = 0; j < len; j++) {
      count = 1;
      //update count if characters are the same
      while (j + 1 < len && output[j] == output[j+1]) {
        count++;
        j++;
      }
      fwrite(&count, sizeof(int), 1, stdout);
      printf("%c", output[j]);
    }
    exit(0);
  }
}
