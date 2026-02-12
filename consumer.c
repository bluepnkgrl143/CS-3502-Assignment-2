// Paola Di Cesare
//CS 3502- Operating systems
// Assignment 2


// ============================================
// consumer.c - Consumer process
// ============================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <semaphore.h>
#include <fcntl.h>

#include "buffer.h"

// Global variables for cleanup
shared_buffer_t *buffer = NULL;
sem_t *mutex = NULL;
sem_t *empty = NULL;
sem_t *full  = NULL;
int shm_id = -1;

static void cleanup() {
    // Detach shared memory
    if (buffer && buffer != (void *)-1) {
        shmdt(buffer);
        buffer = NULL;
    }
    // Close semaphores
    if (mutex && mutex != SEM_FAILED) sem_close(mutex);
    if (empty && empty != SEM_FAILED) sem_close(empty);
    if (full  && full  != SEM_FAILED) sem_close(full);

    mutex = empty = full = NULL;
}

static void signal_handler(int sig) {
    printf("\nConsumer: Caught signal %d, cleaning up...\n", sig);
    cleanup();
    _exit(0);
}

int main(int argc, char *argv[]) {
    // 1) Parse command-line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <consumer_id> <num_items>\n", argv[0]);
        return 1;
    }
    int consumer_id = atoi(argv[1]);
    int num_items   = atoi(argv[2]);
    if (consumer_id < 0 || num_items < 0) {
        fprintf(stderr, "Error: consumer_id and num_items must be non-negative.\n");
        return 1;
    }

    // Install signal handlers for clean exit
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    // Small jitter for demo realism
    srand((unsigned int)time(NULL) + (unsigned int)(consumer_id * 100));

    // 2) Attach to existing shared memory
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), 0666);
    if (shm_id < 0) {
        perror("shmget failed (is the producer running to create it?)");
        return 1;
    }

    buffer = (shared_buffer_t *)shmat(shm_id, NULL, 0);
    if (buffer == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    // 3) Open the three named semaphores
    mutex = sem_open(SEM_MUTEX, 0);
    if (mutex == SEM_FAILED) { perror("sem_open(mutex)"); cleanup(); return 1; }

    empty = sem_open(SEM_EMPTY, 0);
    if (empty == SEM_FAILED) { perror("sem_open(empty)"); cleanup(); return 1; }

    full  = sem_open(SEM_FULL,  0);
    if (full == SEM_FAILED)  { perror("sem_open(full)");  cleanup(); return 1; }

    printf("Consumer %d: Starting to consume %d items\n", consumer_id, num_items);

    // 4) Consume exactly num_items
    for (int i = 0; i < num_items; i++) {
        // --- Consumer synchronization protocol ---
        // Wait for an item
        if (sem_wait(full) == -1) {
            perror("sem_wait(full)");
            break;
        }

        // Enter critical section
        if (sem_wait(mutex) == -1) {
            perror("sem_wait(mutex)");
            // Return the 'full' token if we couldn't enter CS
            sem_post(full);
            break;
        }

        // --- Critical section: remove item from circular buffer ---
        item_t item = buffer->buffer[buffer->head];
        buffer->head  = (buffer->head + 1) % BUFFER_SIZE;
        buffer->count--;

        // Print while still in CS to keep output ordered
        printf("Consumer %d: Consumed value %d from Producer %d\n",
               consumer_id, item.value, item.producer_id);
        fflush(stdout);

        // Exit critical section
        if (sem_post(mutex) == -1) {
            perror("sem_post(mutex)");
            break;
        }

        // Signal that a slot is now empty
        if (sem_post(empty) == -1) {
            perror("sem_post(empty)");
            break;
        }

        // Simulate consumption time
        usleep((useconds_t)(rand() % 100000)); // up to 100ms
    }

    printf("Consumer %d: Finished consuming %d items\n", consumer_id, num_items);

    // 6) Clean up resources
    cleanup();
    return 0;
}
