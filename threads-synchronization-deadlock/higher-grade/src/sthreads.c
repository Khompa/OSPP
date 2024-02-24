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
#include <string.h>

#include "sthreads.h"
#include <sys/time.h> // ITIMER_REAL, ITIMER_VIRTUAL, ITIMER_PROF, struct itimerval, setitimer()
#include <limits.h>   // INT_MAX

/* Stack size for each context. */
#define STACK_SIZE SIGSTKSZ*100

#define TIMEOUT    50          // ms 
#define TIMER_TYPE ITIMER_REAL // Type of timer.

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

struct t_link {
  tid_t id;
  t_link_t *next;
};

struct t_list {
  t_link_t *first;
  t_link_t *last;
  size_t size;
};


list_t thread_list;
list_t waiting_list;
t_list_t term_list;

/*******************************************************************************
                             Auxiliary functions

                      Add internal helper functions here.
********************************************************************************/


void finale() {
  printf("-------- its over :D ---------\n");
}

void list_clear(list_t *list)
{
    thread_t *current = list->first_in_line;
    while (current)
    {
        thread_t *tmp = current;
        current = current->next;
        free(tmp);
        tmp = NULL;
        list->size--;
    }
    list->first_in_line->next = NULL;
}

void list_destroy(list_t *list)
{
    list_clear(list);
    free(list);
    list = NULL;
}

void term_list_clear(t_list_t *list)
{
    t_link_t *current = list->first;
    while (current)
    {
        t_link_t *tmp = current;
        current = current->next;
        free(tmp);
        tmp = NULL;
        list->size--;
    }
    list->first->next = NULL;
}

void term_list_destroy(t_list_t *list)
{
    term_list_clear(list);
    free(list);
    list = NULL;
}

int timer_signal(int timer_type) {
  int sig;

  switch (timer_type) {
    case ITIMER_REAL:
      sig = SIGALRM;
      break;
    case ITIMER_VIRTUAL:
      sig = SIGVTALRM;
      break;
    case ITIMER_PROF:
      sig = SIGPROF;
      break;
    default:
      fprintf(stderr, "ERROR: unknown timer type %d!\n", timer_type);
      exit(EXIT_FAILURE);
  }

  return sig;
}

void set_timer(int type, void (*handler) (int), int ms) {
  struct itimerval timer;
  struct sigaction sa;

  /* Install signal handler for the timer. */
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler =  handler;
  sigaction (timer_signal(type), &sa, NULL);

  /* Configure the timer to expire after ms msec... */
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = ms*1000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;

  if (setitimer (type, &timer, NULL) < 0) {
    perror("Setting timer");
    exit(EXIT_FAILURE);
  };
}

void timer_handler (int signum) {
  static int count = 0;
  fprintf (stdout, "======> timer (%03d)\n", count++);
  yield();
}


/*******************************************************************************
                    Implementation of the Simple Threads API
********************************************************************************/

int  init() {
  /*
  Sets thread id counter to 0. 
  Creates linked lists for ready, waiting and terminated threads. 
  */
  
  // Start thread id counter 
  tid_global = 0;
  
  // Initialize ready linked list
  thread_list.first_in_line = NULL;
  thread_list.last_in_line = NULL;
  thread_list.size = 0; 

  // Initialize waiting linked list
  waiting_list.first_in_line = NULL;
  waiting_list.last_in_line = NULL;
  waiting_list.size = 0;

  // Initialize terminated linked list
  term_list.first = NULL;
  term_list.last = NULL;
  term_list.size = 0;

  // Set timer
  set_timer(TIMER_TYPE, timer_handler, TIMEOUT);

  return 1;
}

tid_t spawn(void (*start)(), ucontext_t *ctx, ucontext_t *next){  
  /*
  Creates thread and its context. 
  Updates ready list to include said thread. 
  */

  //CREATE CONTEXT
  void *stack = malloc(STACK_SIZE);

  if (stack == NULL) {
    perror("Allocating stack");
    exit(-1);
  }

  if (getcontext(ctx) < 0) {
    perror("getcontext");
    exit(-1);
  }
  ctx->uc_link           = next;
  ctx->uc_stack.ss_sp    = stack;
  ctx->uc_stack.ss_size  = STACK_SIZE;
  ctx->uc_stack.ss_flags = 0;

  //CREATE NEW THREAD
  thread_t *thread = malloc(sizeof(thread_t));
  thread -> tid = tid_global ++;
  thread -> state = ready;
  thread -> ctx = ctx;
  thread -> next = NULL;

  //UPDATE LIST
  if (thread_list.size == 0) {
    thread_list.first_in_line = thread;
    thread -> state = running;
  }
  else {
    thread_list.last_in_line->next = thread;
  }

  thread_list.last_in_line = thread;
  thread_list.size ++;

  makecontext(ctx, start, 0);
 
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

  //Set first in line to be new last in line
  thread_list.last_in_line = thread_list.first_in_line;
  thread_list.last_in_line->ctx->uc_link = NULL; //update uclink in new last
  
  thread_list.first_in_line = thread_list.first_in_line -> next;
  thread_list.first_in_line -> state = running;

  // Reset timer
  set_timer(TIMER_TYPE, timer_handler, TIMEOUT);

  if (swapcontext(temp, thread_list.first_in_line->ctx) < 0) {  //&temp, &(&thread_list)->first_in_line->ctx
    perror("swapcontext");
    exit(EXIT_FAILURE);
  }
}

void  done(){
  thread_t *current = waiting_list.first_in_line;
  thread_t *prev = waiting_list.first_in_line;
  while (current != NULL){

    if (current->waiting_for == thread_list.first_in_line->tid) {
        //Update state of current thread
        current->state = ready;
        //Make last in line point to new last in line (currently first in line)
        thread_list.last_in_line->ctx->uc_link = current->ctx; //update uclink is last
        
        thread_list.last_in_line->next = current; //  first/running thread is now last in line again

        //Set first in line to be new last in line
        thread_list.last_in_line = current;
        thread_list.last_in_line->ctx->uc_link = NULL; //update uclink in new last

        //Remove from waiting list
        if (current == waiting_list.first_in_line){
          waiting_list.first_in_line = NULL;
          waiting_list.last_in_line = NULL;
        }
        else{
          prev->next = current->next;
        }
        waiting_list.size--;
        thread_list.size++;
    }
    prev = current;
    current = current->next;
  }

// If no more ready tasks, end
  if (thread_list.size == 0){
    ucontext_t finale_ctx; 
    spawn(finale, &finale_ctx, NULL);
    setcontext(&finale_ctx);
  }

  ucontext_t *temp = thread_list.first_in_line -> ctx;

  //Update state of current thread
  thread_list.first_in_line->state = terminated;

  thread_t *tempo = thread_list.first_in_line;
  t_link_t *terminated = malloc(sizeof(t_link_t));
  terminated->id = tempo->tid;
  terminated->next = NULL;
  
  if (term_list.size == 0) {
    term_list.first = terminated;
  }
  else{
    term_list.last->next = terminated;
  }

  term_list.last = terminated;
  term_list.size++;

  //Update state of next thread
  thread_list.first_in_line = thread_list.first_in_line -> next;
  thread_list.first_in_line -> state = running;
  thread_list.size --;


  if (swapcontext(temp, thread_list.first_in_line->ctx) < 0) {  
    perror("swapcontext");
    exit(EXIT_FAILURE);
  }
  //fprintf(stdout, "%i is first in line after done :)\n", thread_list.first_in_line->tid);
}

tid_t join(tid_t thread) {
  // check if already terminated
  t_link_t *current = term_list.first;
  while (current != NULL){
    if (current->id == thread) {
      return thread;
    }
    current = current->next;
  }


  // Add to wait list and run thread that is next in line
  thread_list.first_in_line -> state = waiting;
  thread_list.first_in_line -> waiting_for = thread;
  
  thread_t copy = *(thread_list.first_in_line); //copy of numbers that has letter as next
  copy.next = NULL;

  if (waiting_list.size == 0) {
    waiting_list.first_in_line = &copy;
    waiting_list.first_in_line->next = NULL;
  }
  else{
    waiting_list.last_in_line->next = &copy;
  }

  waiting_list.last_in_line = &copy;
  waiting_list.last_in_line->next = NULL;
  waiting_list.size++;

  if (thread_list.first_in_line ==  thread_list.first_in_line -> next){
    fprintf(stderr, "Deadlock\n");
    exit(-1);
    thread_list.first_in_line->next = NULL;
  }
  
  thread_list.first_in_line = thread_list.first_in_line -> next;
  thread_list.first_in_line -> state = running;
  thread_list.size--;

  swapcontext(waiting_list.last_in_line->ctx, thread_list.first_in_line->ctx);

  return thread;
}
 