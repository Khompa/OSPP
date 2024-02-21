/* On Mac OS (aka OS X) the ucontext.h functions are deprecated and requires the
   following define.
*/
#define _XOPEN_SOURCE 700

/* On Mac OS when compiling with gcc (clang) the -Wno-deprecated-declarations
   flag must also be used to suppress compiler warnings.
*/

#include <signal.h>   /* SIGSTKSZ (default stack size), MINDIGSTKSZ (minimal
                         stack size) */
#include <stdio.h>    /* puts(), printf(), fprintf(), perror(), setvbuf(), _IOLBF,
                         stdout, stderr */
#include <stdlib.h>   /* exit(), EXIT_SUCCESS, EXIT_FAILURE, malloc(), free() */
#include <ucontext.h> /* ucontext_t, getcontext(), makecontext(),
                         setcontext(), swapcontext() */
#include <stdbool.h>  /* true, false */

#include "sthreads.h"

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

/*******************************************************************************
                             Global data structures

                Add data structures to manage the threads here.
********************************************************************************/

ucontext_t new_ctx;
int tid_global;
thread_t *last_in_line;

/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/




/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/

/*
ucontext.h
getcontext()
setcontext()
makecontext()
swapcontext()
*/

int  init(){
  // reset thread id counter
  tid_global = 0;

  // Initialize main context
  ucontext_t main_ctx;
  
  void *stack = malloc(STACK_SIZE);

  if (stack == NULL) {
    perror("Allocating stack");
    exit(-1);
  }

  if (getcontext(&main_ctx) < 0) {
    perror("getcontext");
    exit(-1);
  }

  (&main_ctx)->uc_link           = NULL;
  (&main_ctx)->uc_stack.ss_sp    = stack;
  (&main_ctx)->uc_stack.ss_size  = STACK_SIZE;
  (&main_ctx)->uc_stack.ss_flags = 0;

  last_in_line -> ctx  = main_ctx; 

  return 1;
}

/*
struct thread
  tid_t tid;
  state_t state;
  ucontext_t ctx; 
  thread_t *next; can use this to create a linked list of threads 
*/


tid_t spawn(void (*start)()){
  //CREATE NEW CONTEXT
  ucontext_t new_ctx;
  
  void *stack = malloc(STACK_SIZE);

  if (stack == NULL) {
    perror("Allocating stack");
    exit(-1);
  }


  if (getcontext(&new_ctx) < 0) {
    perror("getcontext");
    exit(-1);
  }
  
  (&new_ctx)->uc_link           = &(last_in_line -> ctx);
  (&new_ctx)->uc_stack.ss_sp    = stack;
  (&new_ctx)->uc_stack.ss_size  = STACK_SIZE;
  (&new_ctx)->uc_stack.ss_flags = 0;


  //CREATE NEW THREAD
  thread_t *thread = malloc(sizeof(thread_t));
  thread -> tid = tid_global ++;
  thread -> state = ready;
  thread -> ctx = new_ctx;

  //UPDATE CURRENT LAST_IN_LINE
  last_in_line->next = thread;
  (&last_in_line->ctx)->uc_link = &new_ctx;

  //REPLACE LAST IN LINE WITH NEW THREAD
  last_in_line = thread;

  //TODO
  makecontext(ctx, start, 0);


  return thread -> tid;
}  

void yield(){
}

void  done(){
}

tid_t join(tid_t thread) {
  return -1;
}
