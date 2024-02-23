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


int tid_global;

struct list {
  thread_t *first_in_line;
  thread_t *last_in_line;
  size_t size;
};


list_t thread_list;

/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/

void finale() {
  printf("-------- its over :D ---------\n");
}

/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/

int  init() {
  // reset thread id counter 
  if (tid_global != 0){
    puts("ERROROROROROR");
  }
  tid_global = 0;
  
  thread_list.first_in_line = NULL;
  thread_list.last_in_line = NULL;
  thread_list.size = 0; 


  return 1;
}

/*
struct thread
  tid_t tid;
  state_t state;
  ucontext_t ctx; 
  thread_t *next; can use this to create a linked list of threads 
*/

tid_t spawn(void (*start)(), ucontext_t *ctx, ucontext_t *next){  
  //CREATE NEW CONTEXT
  
  void *stack = malloc(STACK_SIZE);

  if (stack == NULL) {
    perror("Allocating stack");
    exit(-1);
  }

  if (getcontext(ctx) < 0) {
    perror("getcontext");
    exit(-1);
  }
  ctx->uc_link            = next;
  ctx->uc_stack.ss_sp    = stack;
  ctx->uc_stack.ss_size  = STACK_SIZE;
  ctx->uc_stack.ss_flags = 0;


  //CREATE NEW THREAD
  thread_t *thread = malloc(sizeof(thread_t));
  thread -> tid = tid_global ++;
  thread -> state = ready;
  thread -> ctx = ctx;

  //UPDATE LIST
  if (thread_list.size == 0) {
    thread_list.first_in_line = thread;
    thread -> state = running;
  }
  if ((&thread_list)->size > 0) {
    thread_list.last_in_line->next = thread;
  }
  thread_list.size ++;

  //REPLACE LAST IN LINE WITH NEW THREAD
  thread_list.last_in_line = thread;

  makecontext(ctx, start, 0);
  //puts("made it to end of spawn :)"); 
  return thread -> tid;
}  

void yield(){
  // Remember current context
  ucontext_t *temp = thread_list.first_in_line -> ctx;

  //Update state of current thread
  thread_list.first_in_line->state = ready;
  //Make last in line point to new last in line (currently first in line)
  thread_list.last_in_line->ctx->uc_link = thread_list.first_in_line->ctx; //update uclink is last
  
  thread_list.last_in_line->next = thread_list.first_in_line; //  first/running thread is now last in line again

  thread_list.last_in_line->ctx->uc_link = NULL; //update uclink in new last
  //Set first in line to be new last in line
  thread_list.last_in_line = thread_list.first_in_line;
  
  //Update state of next thread
  thread_list.first_in_line = thread_list.first_in_line -> next;
  thread_list.first_in_line -> state = running;
  /*
  while (thread_list.first_in_line->state != ready){
    if (thread_list.first_in_line->next == NULL){
        done();
    }
  }
  */

  if (swapcontext(temp, thread_list.first_in_line->ctx) < 0) {  //&temp, &(&thread_list)->first_in_line->ctx
    perror("swapcontext");
    exit(EXIT_FAILURE);
  }
  //puts("made it to yield :D"); 
}

void  done(){
  //puts("something was DONE");
  //free stuff  

  //Save thread id 

  if (thread_list.size == 0){
    ucontext_t finale_ctx; 
    spawn(finale, &finale_ctx, NULL);
    setcontext(&finale_ctx);
  }
  ucontext_t *temp = thread_list.first_in_line -> ctx;

  //Update state of current thread
  thread_list.first_in_line->state = terminated;

  //Update state of next thread
  thread_list.first_in_line = thread_list.first_in_line -> next;
  thread_list.first_in_line -> state = running;

  thread_list.size --;
  
  if (swapcontext(temp, thread_list.first_in_line->ctx) < 0) {  //&temp, &(&thread_list)->first_in_line->ctx
    perror("swapcontext");
    exit(EXIT_FAILURE);
  }
  //puts("made it to done :D"); 
}

tid_t join(tid_t thread) {
  return -1;
}
 