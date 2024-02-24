/**
 * bank_test.c
 *
 * Test of the simple bank API implementation. A number of treads attempting to
 * do random money transfers between bank accounts.
 *
 * History
 *
 * 2024-02-23
 *
 * First version by Karl Marklund <karl.markund@it.uu.se>.
 *
 */

#include "bank.h"  // The bank API

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N_ACCOUNTS 10
#define N_THREADS 5
#define ROUNDS 100

account_t *accounts[N_ACCOUNTS];

int sum() {
  int sum = 0;

  for (int id = 0; id < N_ACCOUNTS; id++) {
    sum += accounts[id]->balance;
  }
  return sum;
}

// I think this can be removed?
void print_account(account_t *account) {
  int from_id = -1;  // You must change this. -------------------------------------------------------------------------------------------
  int to_id = -1;    // You must change this. -------------------------------------------------------------------------------------------

  printf("%d   %d\n", from_id, to_id);

  printf("CHECK TO SEE IF PRINTACCOUNT IS EVER ACCESSED \n-------------------\n----------------------\n----------------------\n----------------------\n----------------------\n");
}

void print_accounts() {
  printf("\nAccount Amount\n");
  printf("------------------\n");

  account_t *account;
  int balance;

  for (int id = 0; id < N_ACCOUNTS; id++) {
    account = accounts[id];
    balance = account->balance;

    int account_id = account->aid;  // You must change this. ((changed))

    printf("%d\t%d\n", account_id, balance);
  }

  printf("------------------\n");
  printf("   Sum: %d\n", sum());
}

void *thread(void *arg) {
  int from, to, amount;

  from = rand() % N_ACCOUNTS;
  to = rand() % N_ACCOUNTS;

  while (to == from) {
    to = rand() % N_ACCOUNTS;
  }

  amount = 50 * (1 + rand() % 5);

  transfer(amount, accounts[from], accounts[to]);

  return NULL;
}

bool negative_balance() {
  for (int id = 0; id < N_ACCOUNTS; id++) {
    if (accounts[id]->balance < 0) return true;
  }
  return false;
}

void sanity_check(int init_sum) {
  printf("\n");

  bool error = false;

  // Due to data races a balance may become negative.

  if (negative_balance()) {
    printf("ERROR: Negative balance due to DATA RACE!\n\n");
    exit(EXIT_FAILURE);
  }

  // NOTE: Due to data races extra money may also be added to an account. This
  // is not as easy to detect as a negative balance and we do not check this in
  // any way.

  // NOTE: Data races may cancel each other out. Hence the system invariant, the
  // conversation of the total amount of money, may not be broken, even if there
  // is an undetected data race.

  int end_sum = sum();

  if (end_sum == init_sum) {
    printf("SUCCESS: System invariant not broken!\n\n");
  } else {
    error = true;
    printf("ERROR: System invariant broken!\n\n");
  }
  printf("Total amount of money was initially %d and is now %d.\n", init_sum,
         end_sum);

  if (error) exit(EXIT_FAILURE);
} 

int main(void) {
  pthread_t threads[N_THREADS];

  // Seed the pseudo random number generator.
  srand(time(NULL));

  // Create the bank accounts.

  for (int id = 0; id < N_ACCOUNTS; id++) {
    accounts[id] = account_new(0);
  }

  accounts[0]->balance = 500;
  accounts[3]->balance = 200;

  print_accounts();

  // Save the initial total sum of money. This will be used a system invariant.
  int init_sum = sum();

  // Do ROUNDS number of tests. In each round N_THREADS attempts do one random
  // money transfer each.

  for (int i = 1; i <= ROUNDS; i++) {
    printf("\nRound %d of %d\n\n", i, ROUNDS);

    for (int i = 0; i < N_THREADS; i++) {
      pthread_create(&threads[i], NULL, thread, NULL);
    }

    for (int i = 0; i < N_THREADS; i++) {
      pthread_join(threads[i], NULL);
    }

    print_accounts();
    sanity_check(init_sum);
  }
}