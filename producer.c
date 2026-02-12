// Paola Di Cesare
//CS 3502-Operating System
//Assignment 2


// ============================================
// producer.c - Producer process
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
#include <fcntl.h>  // O_CREAT

#include "buffer.h"

// Global handles (for cleanup)
shared_buffer_t *buffer = NULL;
sem_t *mutex = NULL;
sem_t *empty = NULL;
sem_t *full  = NULL;
int shm_id = -1;

// --- Cleanup helpers ---
void cleanup() {
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

void signal_handler(int sig) {
    printf("\nProducer: Caught signal %d, cleaning up...\n", sig);
    cleanup();
    _exit(0);
}

int main(int argc, char *argv[]) {
    // 1) Parse command line
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <producer_id> <num_items>\n", argv[0]);
        return 1;
    }
    int producer_id = atoi(argv[1]);
    int num_items   = atoi(argv[2]);
    if (producer_id < 0 || num_items < 0) {
        fprintf(stderr, "Error: producer_id and num_items must be non-negative integers.\n");
        return 1;
    }

    // Signal handlers 
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    // Seed randomness 
    srand((unsigned int)time(NULL) + (unsigned int)producer_id);

    // 2) Create or attach to shared memory
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        return 1;
    }

    buffer = (shared_buffer_t *)shmat(shm_id, NULL, 0);
    if (buffer == (void *)-1) {
        perror("shmat failed");
        return 1;
    }

    // Initialize ring buffer fields conservatively if they look uninitialized
    if (buffer->count < 0 || buffer->count > BUFFER_SIZE) {
        buffer->head = 0;
        buffer->tail = 0;
        buffer->count = 0;
    }

    // 3) Open the three named semaphores (create if they don't exist)
    mutex = sem_open(SEM_MUTEX, O_CREAT, 0644, 1);             // binary mutex
    if (mutex == SEM_FAILED) { perror("sem_open(mutex)"); cleanup(); return 1; }

    empty = sem_open(SEM_EMPTY, O_CREAT, 0644, BUFFER_SIZE);   // empty slots
    if (empty == SEM_FAILED) { perror("sem_open(empty)"); cleanup(); return 1; }

    full  = sem_open(SEM_FULL,  O_CREAT, 0644, 0);             // full slots
    if (full == SEM_FAILED)  { perror("sem_open(full)");  cleanup(); return 1; }

    printf("Producer %d: Starting to produce %d items\n", producer_id, num_items);

    // 4) Produce num_items with unique values
    for (int i = 0; i < num_items; i++) {
        // Create unique item value
        item_t item;
        item.value       = producer_id * 1000 + i;
        item.producer_id = producer_id;

        // 5) Synchronization protocol for producer:
        // Wait for an empty slot
        if (sem_wait(empty) == -1) {
            perror("sem_wait(empty)");]
            break;
        }

        // Enter critical section
        if (sem_wait(mutex) == -1) {
            perror("sem_wait(mutex)");
            // Return the slot we took if we can't enter the CS
            sem_post(empty);
            break;
        }

        // --- Critical section: add item to circular buffer ---
        buffer->buffer[buffer->tail] = item;
        buffer->tail  = (buffer->tail + 1) % BUFFER_SIZE;
        buffer->count++;

        // Print each production while still holding mutex 
        printf("Producer %d: Produced value %d\n", producer_id, item.value);
        fflush(stdout);

        // Exit critical section
        if (sem_post(mutex) == -1) {
            perror("sem_post(mutex)");
            break;
        }

        // Signal that a full slot is available
        if (sem_post(full) == -1) {
            perror("sem_post(full)");
            break;
        }

        // Small random delay to simulate work
        usleep((useconds_t)(rand() % 100000)); // up to 100ms
    }

    printf("Producer %d: Finished producing %d items\n", producer_id, num_items);

    cleanup();
    return 0;
}
