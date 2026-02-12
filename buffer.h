
//Paola Di Cesare
// CS 3502: Operating Systems
//Assignment 2

#ifndef BUFFER_H
#define BUFFER_H

#define BUFFER_SIZE 10
#define SHM_KEY 0x1234
#define SEM_MUTEX "/sem_mutex"
#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"

typedef struct {
	int value;
	int producer_id;
} item_t;

typedef struct {
	item_t buffer[BUFFER_SIZE];
	int head;
	int tail;
	int count;
} shared_buffer_t;

#endif
