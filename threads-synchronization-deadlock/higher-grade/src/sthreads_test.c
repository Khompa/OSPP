#include <stdlib.h>   // exit(), EXIT_FAILURE, EXIT_SUCCESS 
#include <stdio.h>    // printf(), fprintf(), stdout, stderr, perror(), _IOLBF
#include <stdbool.h>  // true, false
#include <limits.h>   // INT_MAX

#include "sthreads.h" // init(), spawn(), yield(), done()
#include <signal.h>
#include <string.h>
#include <sys/time.h> // ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF, struct itimerval, setitimer()
#include <limits.h>   // INT_MAX

#include <unistd.h>    // sleep()

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

#define MAX_SLEEP_TIME  3

#define TIMEOUT    50          // ms 
#define TIMER_TYPE ITIMER_REAL // Type of timer.

/*******************************************************************************
                   Functions to be used together with spawn()

    You may add your own functions or change these functions to your liking.
********************************************************************************/


/* Prints the sequence 0, 1, 2, .... INT_MAX over and over again.
 */
void numbers() {
  int n = 0;
  while (true) {
    printf(" n = %d\n", n);
    n = (n + 1) % (INT_MAX);
   if (n == 3) join(1);
   if (n > 10) done();
   sleep(rand() % MAX_SLEEP_TIME);
    //yield();
  } 
}

/* Prints the sequence a, b, c, ..., z over and over again.
 */
void letters() {
  char c = 'a';

  while (true) {
      printf(" c = %c\n", c);
      if (c == 'f'){join(0);}
      if (c == 'z') done();
      sleep(rand() % MAX_SLEEP_TIME);
      //yield();
      c = (c == 'z') ? 'a' : c + 1;
    }
}

/* Calculates the nth Fibonacci number using recursion.
 */
int fib(int n) {
  switch (n) {
  case 0:
    return 0; 
  case 1:
    return 1;
  default:
    return fib(n-1) + fib(n-2);
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This is deliberately an unnecessary slow and CPU intensive
   implementation where each number in the sequence is calculated recursively
   from scratch.
*/

void fibonacci_slow() {
  int n = 0;
  int f;
  while (true) {
    f = fib(n);
    if (f < 0) {
      // Restart on overflow.
      n = 0;
    }
    printf(" fib(%02d) = %d\n", n, fib(n));
    n = (n + 1) % INT_MAX;
    if (n > 200) done();
    yield();
  }
}

/* Print the Fibonacci number sequence over and over again.

   https://en.wikipedia.org/wiki/Fibonacci_number

   This implementation is much faster than fibonacci().
*/
void fibonacci_fast() {
  int a = 0;
  int b = 1;
  int n = 0;
  int next = a + b;

  while(true) {
    printf(" fib(%02d) = %d\n", n, a);
    next = a + b;
    a = b;
    b = next;
    n++;
    if (a < 0) {
      // Restart on overflow.
      a = 0;
      b = 1;
      n = 0;
    }
  }
}

/* Prints the sequence of magic constants over and over again.

   https://en.wikipedia.org/wiki/Magic_square
*/
void magic_numbers() {
  int n = 3;
  int m;
  while (true) {
    m = (n*(n*n+1)/2);
    if (m > 0) {
      printf(" magic(%d) = %d\n", n, m);
      n = (n+1) % INT_MAX;
    } else {
      // Start over when m overflows.
      n = 3;
    }
    //yield();
    if (n > 20) join(0);
    if (n > 100) done();
  }
}

/*******************************************************************************
                                     main()

            Here you should add code to test the Simple Threads API.
********************************************************************************/

int main(){
  setvbuf(stdout, 0, _IOLBF, 0);
  setvbuf(stderr, 0, _IOLBF, 0);
  
  puts("\n==== Test program for the Simple Threads API ====\n");
  ucontext_t numbers_ctx;
  ucontext_t letters_ctx; 
  ucontext_t fibonacci_slow_ctx;
  ucontext_t magic_numbers_ctx;

  init(); // Initialization
  spawn(numbers, &numbers_ctx, &fibonacci_slow_ctx);
  spawn(fibonacci_slow, &fibonacci_slow_ctx, &magic_numbers_ctx);
  spawn(magic_numbers, &magic_numbers_ctx, &letters_ctx);
  spawn(letters, &letters_ctx, NULL);

  /* Transfers control to the foo context. */

  setcontext(&numbers_ctx);
  
  fprintf(stderr, "ERROR! A successful call to setcontext() does not return!\n");
}