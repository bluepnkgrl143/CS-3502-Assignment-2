#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


//using the same configs from phase 1

#define NUM_ACCOUNTS 2
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

// Updated Account structure with mutex (GIVEN)
typedef struct {
	int account_id;
	double balance;
	int transaction_count;
	pthread_mutex_t lock; // NEW: Mutex for this account
} Account;

//gloabals
Account accounts[NUM_ACCOUNTS];
static double thread_net_track[NUM_THREADS]; //each thread will publish its net amount

// GIVEN: Example of mutex initialization
void initialize_accounts() {
	for (int i = 0; i < NUM_ACCOUNTS; i++) {
		accounts[i].account_id = i;
		accounts[i].balance = INITIAL_BALANCE;
		accounts[i].transaction_count = 0;
		// Initialize the mutex
		pthread_mutex_init(&accounts[i].lock, NULL);
	}
}
// GIVEN: Example deposit function WITH proper protection
void deposit_safe(int account_id, double amount) {
	// Acquire lock BEFORE accessing shared data
	pthread_mutex_lock(&accounts[account_id].lock);
	// ===== CRITICAL SECTION =====
	// Only ONE thread can execute this at a time for this account
	accounts[account_id].balance += amount;
	accounts[account_id].transaction_count++;
	// ============================

	// Release lock AFTER modifying shared data
	pthread_mutex_unlock(&accounts[account_id].lock);
}

// TODO 1: Implement withdrawal_safe() with mutex protection
// Reference: Follow the pattern of deposit_safe() above
// Remember: lock BEFORE accessing data, unlock AFTER
void withdrawal_safe(int account_id, double amount) {
// YOUR CODE HERE
// Hint: pthread_mutex_lock
// Hint: Modify balance
// Hint: pthread_mutex_unlock

    	// Acquire lock BEFORE accessing shared data
    	pthread_mutex_lock(&accounts[account_id].lock);

    	// ===== CRITICAL SECTION =====
    	// Only ONE thread can execute this at a time for this account
    	accounts[account_id].balance -= amount;
    	accounts[account_id].transaction_count++;
    	// ============================

    	// Release lock AFTER modifying shared data
    	pthread_mutex_unlock(&accounts[account_id].lock);
}

// TODO 2: Update teller_thread to use safe functions
// Change: deposit_unsafe -> deposit_safe
// Change: withdrawal_unsafe -> withdrawal_safe

void* teller_thread(void* arg) {
    	int teller_id = *(int*)arg;

    	unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self()
                        ^ (unsigned int)teller_id;

	double net_track = 0.0;

    	for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
        	int account_idx = rand_r(&seed) % NUM_ACCOUNTS;
        	double amount   = (rand_r(&seed) % 100) + 1;
        	int operation   = rand_r(&seed) % 2;

        	if (operation == 1) {
            		deposit_safe(account_idx, amount);
			net_track += amount; //deposit increases system total
            		printf("Teller %d: Deposited $%.2f to Account %d\n",
                   		teller_id, amount, account_idx);
        	} else {
            		withdrawal_safe(account_idx, amount);
			net_track -= amount; //withdrawal decreases system total;
            		printf("Teller %d: Withdrew $%.2f from Account %d\n",
                   		teller_id, amount, account_idx);
       		}
    	}

	thread_net_track[teller_id] = net_track;
   	return NULL;
}

void cleanup_mutexes() {
        for (int i = 0; i < NUM_ACCOUNTS; i++) {
		pthread_mutex_destroy(&accounts[i].lock);
        }
}

int main() {
    	printf("=== Phase 2: Mutex-Protected Transactions ===\n\n");

    	initialize_accounts();

    	// Display initial state
    	printf("Initial State:\n");
    	for (int i = 0; i < NUM_ACCOUNTS; i++) {
        	printf("  Account %d: $%.2f\n", i, accounts[i].balance);
    	}

    	// Create thread and ID arrays
    	pthread_t threads[NUM_THREADS];
    	int thread_ids[NUM_THREADS];
// TODO 3: Add performance timing
// Reference: Section 7.2 "Performance Measurement"
// Hint: Use clock_gettime(CLOCK_MONOTONIC, &start);
	//...Performance timing starts...
    	struct timespec start, end;
    	clock_gettime(CLOCK_MONOTONIC, &start);
    	// Create all threads
    	for (int i = 0; i < NUM_THREADS; i++) {
        	thread_ids[i] = i;
        	int rc = pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
        	if (rc != 0) {
            		perror("pthread_create");
            		exit(EXIT_FAILURE);
        	}
    	}
    	// Wait for all threads to complete
    	for (int i = 0; i < NUM_THREADS; i++) {
        int rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            	perror("pthread_join");
            	exit(EXIT_FAILURE);
        	}
    	}
    	//...Performance timing ends....
    	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) +
		(end.tv_nsec - start.tv_nsec) / 1e9;
	printf("Time: %.4f seconds\n", elapsed);
    	// ............

    	// Final results
    	printf("\n=== Final Results ===\n");
    	double actual_total = 0.0;
    	for (int i = 0; i < NUM_ACCOUNTS; i++) {
        	printf("Account %d: $%.2f (%d transactions)\n",
               		i, accounts[i].balance, accounts[i].transaction_count);
        	actual_total += accounts[i].balance;
    	}

	//Computes what the total should be
	double net_track = 0.0;
	for (int i = 0; i < NUM_THREADS; i++){
		net_track += thread_net_track[i];
	}

	//The expected total
	double expected_total = NUM_ACCOUNTS* INITIAL_BALANCE + net_track;

    	printf("\nExpected total: $%.2f\n", expected_total);
    	printf("Actual total:   $%.2f\n", actual_total);
    	double diff = actual_total - expected_total;
    	if (diff < 0) diff = -diff;
    	printf("Difference:     $%.2f\n", actual_total - expected_total);

    	if (diff > 1e-6) {
        	printf("\nWARNING: discrepancy detected.\n");
    	} else {
        	printf("\nNo discrepancy detected.\n");
    	}

// TODO 4: Add mutex cleanup in main()
// Reference: man pthread_mutex_destroy
// Important: Destroy mutexes AFTER all threads complete!
    	cleanup_mutexes();

    	return 0;
}
