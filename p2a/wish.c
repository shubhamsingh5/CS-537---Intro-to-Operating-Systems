#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//constants
#define EXIT "exit"
#define CD "cd"
#define PATH "path"
#define DELIM " \t\n\b\a"

char *path[100];
char error_message[30] = "An error has occurred\n";

//method declarations
void loop(FILE *mode);
char **parse_and(char *input);
char **parse_whitespace(char *input);
void parse_redirection(char *input);
void execute(char **args);
void execute_exit(char **args);
void execute_path(char **args);
void execute_cd(char **args);
void execute_redirect(char **args, char* outputFile);

int main(int argc, char const *argv[]) {
  FILE *fp;

  //set path to "/bin" by default
  path[0] = "/bin";

  //interactive mode
  if (argc == 1) {
    printf("wish> ");
    fflush(stdout);
    loop(stdin);
  }
  //batch mode
  else if (argc == 2) {
    if (!(fp = fopen(argv[1], "r"))) { //file not opened
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    } else {    //file opened
      loop(fp);
    }
  }
  else {
    //error mode
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  return 0;
}

void loop(FILE *mode) {
  size_t buffersize = 32;
  char *input = malloc(buffersize * sizeof(char));
  //list of parallel commands
  char **command_queue = malloc(buffersize * sizeof(char *));
  //whitespace seperated command
  char **command = malloc(buffersize * sizeof(char *));

  //get each line of input
  while (getline(&input, &buffersize, mode) != EOF) {
    if (strcmp(input, "\n")) {    //check if command is entered
      if (strstr(input, "&")) {   //check for parallel commands
        //add commands into list
        command_queue = parse_and(input);
        //loop through command and execute
        for (int i = 0; command_queue[i] != NULL; i++) {
          //check if command is a redirection
          if (strstr(command_queue[i], ">")) parse_redirection(command_queue[i]);
          else {    //not redirection, parse normally
            command = parse_whitespace(command_queue[i]);
            //check for NULL to prevent segfaults
            if (command[0] != NULL) {
              //check for builtin commands
              if (!strcmp(command[0], EXIT)) execute_exit(command);
              else if (!strcmp(command[0], PATH)) execute_path(command);
              else if (!strcmp(command[0], CD)) execute_cd(command);

              else execute(command);
            }
          }
        }
      }
      //if not a parallel command, check for redirection
      else if (strstr(input, ">")) parse_redirection(input);
      else {    //parse and execute normally
        command = parse_whitespace(input);
        if (command[0] != NULL) {
          //check for builtin commands
          if (!strcmp(command[0], EXIT)) execute_exit(command);
          else if (!strcmp(command[0], PATH)) execute_path(command);
          else if (!strcmp(command[0], CD)) execute_cd(command);

          else execute(command);
        }
      }
    }
    if (mode == stdin) {
      printf("wish> ");
      fflush(stdout);
    }
  }
  free(command);
  free(input);
}

//parse by "&"
char **parse_and(char *input) {
  int buffersize = 32;

  //raw input
  char **raw = malloc(buffersize * sizeof(char *));
  char *tok = malloc(buffersize * sizeof(char));
  int pos = 0;

  if (raw == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  tok = strtok(input, "&");
  while (tok != NULL) {
    raw[pos] = tok;
    pos++;

    if (pos >= buffersize) {
      buffersize += 32;
      raw = realloc(raw, buffersize * sizeof(char *));
      if (raw == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
      }
    }
    tok = strtok(NULL, "&");
  }
  raw[pos] = NULL;

  return raw;
}

//parse by ">"
void parse_redirection(char *input) {
  int buffersize = 32;

  //raw input
  char **raw = malloc(buffersize * sizeof(char *));
  //command after seperating
  char **command = malloc(buffersize * sizeof(char *));
  //file name after seperating
  char **file = malloc(buffersize * sizeof(char *));
  char *tok = malloc(buffersize * sizeof(char));
  int pos = 0;

  if (raw == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  tok = strtok(input, ">");
  while (tok != NULL) {
    raw[pos] = tok;
    pos++;

    if (pos >= buffersize) {
      buffersize += 32;
      raw = realloc(command, buffersize * sizeof(char *));
      if (raw == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
      }
    }
    tok = strtok(NULL, ">");
  }
  raw[pos] = NULL;

  if (raw[2] != NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  }

  //remove whitespace from command
  command = parse_whitespace(raw[0]);
  //remove whitespace from file
  file = parse_whitespace(raw[1]);
  //check if file is NULL
  if (file[0] == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  }
  //check if more than one output file
  if (file[1] != NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  }
  //execute command
  execute_redirect(command, file[0]);
}

//parse by whitespace
char **parse_whitespace(char *input) {
  int buffersize = 32;
  char **command = malloc(buffersize * sizeof(char *));

  if (command == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  char *tok;
  int pos = 0;

  //parse whitespace
  tok = strtok(input, DELIM);
  while (tok != NULL) {
    command[pos] = tok;
    pos++;

    if (pos >= buffersize) {
      buffersize += 32;
      command = realloc(command, buffersize * sizeof(char *));
      if (command == NULL) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
      }
    }
    tok = strtok(NULL, DELIM);
  }
  command[pos] = NULL;

  return command;
}

//execute normal commands
void execute(char **args) {
  pid_t pid;

  //append path to command
  char *pathexec = malloc(64 * sizeof(char));
  for (int i = 0; path[i] != NULL; i++) {
    char *a = strcat(strdup(path[i]), "/");
    char *temp = strcat(a, args[0]);
    if (!access(temp, X_OK)) {
      pathexec = temp;
      break;
    }
  }

  //check if path is NULL
  if (pathexec == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  }

  pid = fork();
  if (pid == 0) {
    //child
    if (execv(pathexec, args) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
  }
  else if (pid < 0) {
    //error forking
    write(STDERR_FILENO, error_message, strlen(error_message));
  }
  else {
    //parent process
    pid = wait(NULL);
  }
}

//execute exit command
void execute_exit(char **args) {
  if (args[1] != NULL)
    write(STDERR_FILENO, error_message, strlen(error_message));
  else exit(0);
}

//execute cd command
void execute_cd(char **args) {
  if (args[1] == NULL) {    //check for no argument
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  } else if (args[2] != NULL) {   //check for multiple arguments
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  } else {
    if (chdir(args[1]) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      return;
    }
  }
}

//execute path command
void execute_path(char **args) {
  int i = 0;
  while (args[i+1] != NULL) {
    path[i] = malloc(strlen(args[i+1]) * sizeof(char));
    path[i] = strcpy(path[i], args[i+1]);
    i++;
  }
  path[i] = NULL;
}

//execute redirected command
void execute_redirect(char **args, char* outputFile) {
  pid_t pid;
  int fd; //file descriptor

  //append path to command
  char *pathexec = malloc(64 * sizeof(char));
  for (int i = 0; path[i] != NULL; i++) {
    char *a = strcat(strdup(path[i]), "/");
    char *temp = strcat(a, args[0]);
    if (!access(temp, X_OK)) {
      pathexec = temp;
      break;
    }
  }

  //check if path is NULL
  if (pathexec == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    return;
  }

  pid = fork();
  if (pid == 0) {
    fd = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
    dup2(fd, STDOUT_FILENO);
    close(fd);
    //child
    if (execv(pathexec, args) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      exit(1);
    }
  }
  else if (pid < 0) {
    //error forking
    write(STDERR_FILENO, error_message, strlen(error_message));
  } else {
    //parent process
    pid = wait(NULL);
  }
}
