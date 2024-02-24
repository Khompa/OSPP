/**
 * bank.c
 *
 * The implementation of the simple bank API defined in bank.h.
 *
 * Author
 *   Karl Marklund <karl.marklund@it.uu.se>
 *
 */

#include "bank.h"  // The bank API

#include <stdbool.h>  // true, false
#include <stdio.h>    // perror()
#include <stdlib.h>   // exit(), EXIT_FAILURE
#include <unistd.h>   // usleep()

#define MIN_SLEEP 10000
#define MAX_RANDOM_SLEEP 10000

int aid_global = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void RANDOM_SLEEP() { usleep(MIN_SLEEP + (rand() % MAX_RANDOM_SLEEP)); }

account_t *account_new(unsigned int balance) {
  account_t *account = malloc(sizeof(account_t));

  account->balance = balance;

  pthread_mutex_lock(&mutex);
  account->aid = aid_global ++;
  pthread_mutex_unlock(&mutex);

  return account;
}

void account_destroy(account_t *account) {
  free(account);
}

void transfer(int amount, account_t *from, account_t *to) {
  /**
   * NOTE: Leave the random RANDOM_SLEEPs. These are used to enforce a more
   * randomized interleaving of the threads.
   */
 
  int from_id = from->aid;  // You must change this.
  int to_id = to->aid;    // You must change this.

  pthread_mutex_lock(&mutex);
  if (from->balance >= amount) {
    RANDOM_SLEEP();
    from->balance -= amount;
    RANDOM_SLEEP();
    to->balance += amount;
    RANDOM_SLEEP();

    printf("%d --- %03d ---> %d\n", from_id, amount, to_id);
  } else {
    printf("%d --- %03d ---> %d Insufficient funds\n", from_id, amount, to_id);
  }
  pthread_mutex_unlock(&mutex);
}
