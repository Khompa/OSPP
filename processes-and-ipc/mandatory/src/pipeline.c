#include <stdio.h>    // puts(), printf(), perror(), getchar()
#include <stdlib.h>   // exit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>   // getpid(), getppid(),fork()
#include <sys/wait.h> // wait()

#define READ  0
#define WRITE 1

void child_a(int fd[]) {
  //close(fd[READ]);
  dup2(fd[WRITE], STDOUT_FILENO);
  execlp("ls", "ls", "-F", "-1", NULL);
}

void child_b(int fd[]) { 
  close(fd[WRITE]); // ------------------------------
  dup2(fd[READ], STDIN_FILENO);
  execlp("nl", "nl", NULL);
}

int main(void) {
  int fd[2];
  int child_a_int;
  int child_b_int;
  pipe(fd);

  switch (fork()) {
    case 0:
      child_a(fd);
      exit(EXIT_SUCCESS);

    case -1:
      perror("Error: fork didn't work");
      exit(EXIT_FAILURE);

    default:
      break;
  }
  
  switch (fork()) {
    case 0:
      child_b(fd); 
      exit(EXIT_SUCCESS);

    case -1:
      perror("Error: fork didn't work");
      exit(EXIT_FAILURE);

    default:
      break;
  }

  close(fd[WRITE]);
  close(fd[READ]);

  wait(&child_a_int);
  wait(&child_b_int);

  exit(EXIT_SUCCESS);
}
