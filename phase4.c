#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS  2
#define INITIAL_BALANCE 1000.0

typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

void initialize_accounts(void) {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

void cleanup_mutexes () {
	for ( int i = 0; i < NUM_ACCOUNTS ; i ++) {
		pthread_mutex_destroy (& accounts [i]. lock );
	}
}

typedef struct {
    	int from_id;
    	int to_id;
   	double amount;
} Task;

// STRATEGY 1: Lock Ordering ( RECOMMENDED )
// ALGORITHM :
// To prevent circular wait , always acquire locks in consistent order

int safe_transfer(int from_id, int to_id, double amount) {
	if (from_id == to_id || amount <= 0.0) return -1;
    	if (from_id < 0 || from_id >= NUM_ACCOUNTS || to_id < 0 || to_id >= NUM_ACCOUNTS) return -1;

//Step 1: Identify which account ID is lower
	int first  = (from_id < to_id) ? from_id : to_id;
    	int second = (from_id < to_id) ? to_id   : from_id;
//Step 2: Lock lower ID first
   	pthread_mutex_lock(&accounts[first].lock);
//Step 3: Lock higher ID second
    	pthread_mutex_lock(&accounts[second].lock);

//Step 4: Perform transfer
	if (accounts[from_id].balance < amount) {
        	pthread_mutex_unlock(&accounts[second].lock);
        	pthread_mutex_unlock(&accounts[first].lock);
        	return -1;
    	}
	//Transfer
	accounts[from_id].balance -= amount;
    	accounts[from_id].transaction_count++;
    	accounts[to_id].balance   += amount;
    	accounts[to_id].transaction_count++;

	printf("Thread %lu: Transferred $%.2f from %d -> %d\n",(unsigned long)pthread_self(), amount, from_id, to_id);

//Step 5: Unlock in reverse order
	pthread_mutex_unlock(&accounts[second].lock);
    	pthread_mutex_unlock(&accounts[first].lock);
   	return 0;
}

void* worker(void* arg) {
    	Task* t = (Task*)arg;
    	int result = safe_transfer(t->from_id, t->to_id, t->amount);

	if (result == 0) {
        	printf("Thread %lu: transfer %d -> %d of $%.2f OK\n",
               	(unsigned long)pthread_self(), t->from_id, t->to_id, t->amount);
    	} else {
        	printf("Thread %lu: transfer %d -> %d of $%.2f FAILED (rc=%d)\n",
               	(unsigned long)pthread_self(), t->from_id, t->to_id, t->amount, result);
    	}
    	return NULL;
}


int main(void) {
    	printf("=== Phase 4: Deadlock Resolution via Lock Ordering ===\n\n");

    	initialize_accounts();

    	//initial state
    	printf("Initial State:\n");
    	for (int i = 0; i < NUM_ACCOUNTS; i++) {
        	printf("  Account %d: $%.2f\n", i, accounts[i].balance);
    	}
    	puts("");

	pthread_t th[NUM_THREADS];
    	Task tasks[NUM_THREADS] = {
        	{ .from_id = 0, .to_id = 1, .amount = 150.0, },
        	{ .from_id = 1, .to_id = 0, .amount = 200.0, }
    	};

	for (int i = 0; i < NUM_THREADS; i++) {
        	int rc = pthread_create(&th[i], NULL, worker, &tasks[i]);
        	if (rc != 0) { perror("pthread_create"); exit(EXIT_FAILURE); }
    	}
    	for (int i = 0; i < NUM_THREADS; i++) {
        	pthread_join(th[i], NULL);
    	}

	printf("\nFinal State:\n");
	double total = 0.0;
    	for (int i = 0; i < NUM_ACCOUNTS; i++) {
        	printf("  Account %d: $%.2f (%d tx)\n",
               	i, accounts[i].balance, accounts[i].transaction_count);
    	total += accounts[i].balance;
	}
	printf(" System total: $%.2f\n", total);

    	cleanup_mutexes();
    	return 0;
}
