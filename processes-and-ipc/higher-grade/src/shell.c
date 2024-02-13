#include "parser.h"    // cmd_t, position_t, parse_commands()
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>     //fcntl(), F_GETFL
#include <sys/wait.h> // wait() 

// echo Hello | sed s/$/1/ | sed s/$/2/ | sed s/$/3/ | sed s/$/4/ | sed s/$/5/ | sed s/$/6/ | sed s/$/7/ | sed s/$/8/ | sed s/$/9/ 


#define READ  0
#define WRITE 1 

/**
 * For simplicitiy we use a global array to store data of each command in a
 * command pipeline .
 */
cmd_t commands[MAX_COMMANDS];

/**
 *  Debug printout of the commands array.
 */
void print_commands(int n) {
  for (int i = 0; i < n; i++) {
    printf("==> commands[%d]\n", i);
    printf("  pos = %s\n", position_to_string(commands[i].pos));
    printf("  in  = %d\n", commands[i].in);
    printf("  out = %d\n", commands[i].out);

    print_argv(commands[i].argv);
  }

}

/**
 * Returns true if file descriptor fd is open. Otherwise returns false.
 */
int is_open(int fd) {
  return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

void fork_error() {
  perror("fork() failed)");
  exit(EXIT_FAILURE);
}

/**
 *  Fork a proccess for command with index i in the command pipeline. If needed,
 *  create a new pipe and update the in and out members for the command..
 */
void fork_cmd(int i, int (*fd)[2], int n) {
  // Each child will create a pipe to write to. The next child will read from this pipe. The last child doesn't need a pipe so only n - 1 pipes are created. 
  if (i < n - 1) {
    if (pipe(fd[i]) == -1){} // Avoids error
  } 

  // Fork
  pid_t pid;
  switch (pid = fork()) {
    case -1:
      fork_error();
    case 0:
      // Child process after a successful fork().

      // Parent should have cleaned such that we only have access to the read end of the previous child and the newly created pipe
      // In the newly created pipe we only want access to the write end, so close the read end:
      close(fd[i][READ]);  

      if (commands[i].pos == first) {
        dup2(fd[i][WRITE], STDOUT_FILENO);
      }

      else if (commands[i].pos == middle) {
        dup2(fd[i - 1][READ], STDIN_FILENO);
        dup2(fd[i][WRITE], STDOUT_FILENO);
      }
      
      else if (commands[i].pos == last) {
        dup2(fd[i - 1][READ], STDIN_FILENO);
      }

      // Execute the command in the contex of the child process.
      execvp(commands[i].argv[0], commands[i].argv);

      // If execvp() succeeds, this code should never be reached.
      fprintf(stderr, "shell: command not found: %s\n", commands[i].argv[0]);
      exit(EXIT_FAILURE);

    default:
      // Parent process after a successful fork()

      // Close unused pipes (only the read end of the ith pipe will be used by the next child)
      if (i != 0) { 
        close(fd[i - 1][READ]);
      }
      if (i != n - 1){ // this pipe doesnt exist
        close(fd[i][WRITE]);
      }
  }
}

/**
 *  Fork one child process for each command in the command pipeline.
 */
void fork_commands(int n) {
  int fd[n - 1][2];
  for (int i = 0; i < n; i++) {
    fork_cmd(i, fd, n);
  }

  /*
  for (int i = 0; i < n; i++) {
    printf("%i \n", is_open(fd[i][0]));
    printf("%i \n", is_open(fd[i][1]));
  }
  */
}

/**
 *  Reads a command line from the user and stores the string in the provided
 *  buffer.
 */
void get_line(char* buffer, size_t size) {
  //getline(&buffer, &size, stdin);
  if (getline(&buffer, &size, stdin) == -1) {
    ///this should never happen but I was getting an error that the result wasnt being used
  }
  buffer[strlen(buffer)-1] = '\0';
}

/**
 * Make the parents wait for all the child processes.
 */ 
void wait_for_all_cmds(int n) {
  for (int i = 0; i < n; i++) {
    wait(NULL);
  }
}

int main() {
  int n;               // Number of commands in a command pipeline.
  size_t size = 228;   // Max size of a command line string.
  char line[size];     // Buffer for a command line string.


  while(true) {
    printf(" >>> ");

    get_line(line, size);

    n = parse_commands(line, commands); // n is number of commands 
                                        // Access commands with commands[i].argv where commands[i].argv[0] is the path variable for the ith command
    fork_commands(n);

    wait_for_all_cmds(n);
  }

  exit(EXIT_SUCCESS);
}
