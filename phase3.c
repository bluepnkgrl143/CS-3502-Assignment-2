#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_ACCOUNTS 2
#define NUM_THREADS  2
#define INITIAL_BALANCE 1000.0
#define _GNU_SOURCE

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
    	int use_deadlock_path;
} TransferArgs;

// TODO 1: Implement complete transfer function
// Use the example above as reference
// Add balance checking ( sufficient funds ?)
// Add error handling

int transfer_deadlock(int from_id, int to_id, double amount) {
    	if (from_id == to_id) {
        	fprintf(stderr, "Error: cannot transfer between the same account (%d).\n", from_id);
        	return -1;
    	}
    	if (from_id < 0 || from_id >= NUM_ACCOUNTS || to_id   < 0 || to_id   >= NUM_ACCOUNTS) {
        	fprintf(stderr, "Error: invalid account IDs: from=%d to=%d\n", from_id, to_id);
        	return -1;
    	}
    	if (amount <= 0.0) {
        	fprintf(stderr, "Error: transfer amount must be positive (got %.2f)\n", amount);
        	return -1;
    	}

    	// Lock source first
    	pthread_mutex_lock(&accounts[from_id].lock);
    	printf("Thread %lu: Locked account %d\n",(unsigned long)pthread_self(), from_id);
	usleep(100);
    	// lock destination next
    	printf("Thread %lu: Waiting for account %d\n",(unsigned long)pthread_self(), to_id);
    	pthread_mutex_lock(&accounts[to_id].lock);

	//Critical section
	if (accounts[from_id].balance < amount) {
		fprintf(stderr,
               	"Thread %lu: Insufficient funds in account %d (bal=%.2f, need=%.2f)\n",
                (unsigned long)pthread_self(),from_id, accounts[from_id].balance, amount);

        	pthread_mutex_unlock(&accounts[to_id].lock);
        	pthread_mutex_unlock(&accounts[from_id].lock);
        	return -1;
    	}
	//Transfer
	accounts[from_id].balance -= amount;
    	accounts[from_id].transaction_count++;
    	accounts[to_id].balance   += amount;
    	accounts[to_id].transaction_count++;

	printf("Thread %lu: Transferred $%.2f from %d -> %d\n",(unsigned long)pthread_self(), amount, from_id, to_id);

	pthread_mutex_unlock(&accounts[to_id].lock);
    	pthread_mutex_unlock(&accounts[from_id].lock);
   	return 0;
}


// TODO 2: Create threads that will deadlock
// Thread 1: transfer (0, 1, amount ) // Locks 0, wants 1
// Thread 2: transfer (1, 0, amount ) // Locks 1, wants 0
// Result : Circular wait !


void* transfer_thread(void* arg) {
    	TransferArgs* a = (TransferArgs*)arg;

    	// Call the intentionally deadlock-prone transfer
    	int result = transfer_deadlock(a->from_id, a->to_id, a->amount);
    	if (result == 0) {
        	printf("Thread %lu: transfer %d -> %d of $%.2f completed\n",
               	(unsigned long)pthread_self(), a->from_id, a->to_id, a->amount);
    	} else {
        	printf("Thread %lu: transfer %d -> %d of $%.2f failed\n",
               	(unsigned long)pthread_self(), a->from_id, a->to_id, a->amount);
    	}
    	return NULL;
}

// TODO 3: Implement deadlock detection
// Add timeout counter in main ()
// If no progress for 5 seconds , report suspected deadlock
// Reference : time ( NULL ) for simple timing

//needed help putting this section together and debugging. 
int wait_for_threads(pthread_t* threads, int count, int timeout_seconds) {
    	time_t start = time(NULL);
	int remaining = count;

    	while (1) {
        int progressed = 0;
	// Try to join each thread without blocking permanently
        	for (int i = 0; i < remaining; i++) {
            		void* ret=NULL;
            		int result = pthread_tryjoin_np(threads[i], &ret);
            		if (result == 0) {
                		// Thread finished — reduce count
                		// Shift remaining threads down
                		for (int j = i; j < remaining; j++) {
                   			threads[j] = threads[j+1];
                		}
				remaining--;
				progressed = 1;
                		i--; // check same index again
            		}
        	}

        	if (remaining == 0) {
            		return 0; // all threads finished
        	}

        	if (!progressed) {
            		time_t now = time(NULL);
            		if (difftime(now, start) >= timeout_seconds) {
                		return 1; // suspected deadlock
            		}
            		// Sleep a bit to avoid busy-waiting
            		usleep(100 * 1000); // 100 ms
        	}
	}
}


int main(void) {
    	printf("=== Phase 3: Deadlock Creation & Detection ===\n\n");

    	initialize_accounts();

    	//initial state
    	printf("Initial State:\n");
    	for (int i = 0; i < NUM_ACCOUNTS; i++) {
        	printf("  Account %d: $%.2f\n", i, accounts[i].balance);
    	}
    	puts("");

	//Create threads that will deadlock
    	pthread_t threads[2];
    	TransferArgs args[2];

	// Thread 0: locks account 0, then waits for 1
	args[0].from_id = 0;
	args[0].to_id   = 1;
	args[0].amount  = 100.0;
    	args[0].use_deadlock_path = 1;

	// Thread 1: locks account 1, then waits for 0
	args[1].from_id = 1;
	args[1].to_id   = 0;
	args[1].amount  = 200.0;
	args[1].use_deadlock_path = 1;

	int result = pthread_create(&threads[0], NULL, transfer_thread, &args[0]);
	if (result != 0) { perror("pthread_create T0"); exit(EXIT_FAILURE); }

	result = pthread_create(&threads[1], NULL, transfer_thread, &args[1]);
	if (result != 0) { perror("pthread_create T1"); exit(EXIT_FAILURE); }

	//Circular wait
	printf("Both threads started, deadlock should be happening next...\n");

	// TODO 3: Implement deadlock detection
	// Add timeout counter in main ()
	// If no progress for 5 seconds , report suspected deadlock
	// Reference : time ( NULL ) for simple timing

 	int timeout_seconds = 5;
    	int status = wait_for_threads(threads, NUM_THREADS, timeout_seconds);

    	if (status == 0) {
        	printf("\nAll threads completed (no deadlock this run).\n");
    	} else if (status == 1) {
        	printf("\nSUSPECTED DEADLOCK: no progress for %d seconds.\n", timeout_seconds);
        	printf("Coffman conditions likely present:\n");
        	printf("  1) Mutual Exclusion: each account lock is a mutex\n");
        	printf("  2) Hold and Wait: each thread holds one lock while waiting for another\n");
        	printf("  3) No Preemption: locks are not forcibly taken away\n");
        	printf("  4) Circular Wait: T0 waits for T1, and T1 waits for T0\n");
	} else {
        	fprintf(stderr, "\nError while waiting for threads.\n");
    	}

	printf("\nFinal State:\n");
    	for (int i = 0; i < NUM_ACCOUNTS; i++) {
        	printf("  Account %d: $%.2f (%d tx)\n",
               	i, accounts[i].balance, accounts[i].transaction_count);
    	}

    	cleanup_mutexes();
    	return 0;
}
