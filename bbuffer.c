/*Required Headers*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bbuffer.h"
#include <semaphore.h>

/*
 * Declarations for bounded-buffer shared variables -- plus concurrency-control
 * variables -- must START here.
 */

int bounded_buffer[BOUNDED_BUFFER_SIZE];
int next_in, next_out, items_in_buffer;
pthread_mutex_t buffer_access_lock;
pthread_cond_t not_full, not_empty;

/* Initialize sequencing items for the bounded buffer */
void initialize_bounded_buffer() {
    next_in,next_out, items_in_buffer = 0;

    int status, i;

    status = pthread_mutex_init(&buffer_access_lock, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating buffer_access_lock\n");
        exit(1);
    }

    status = pthread_cond_init(&not_full, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating not_full condition\n");
        exit(1);
    }

    status = pthread_cond_init(&not_empty, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating not_empty condtion\n");
        exit(1);
    }
}

/* Helper fuction */
int buffer_full(){
    return items_in_buffer == BOUNDED_BUFFER_SIZE;
}

/* Helper fuction */
int buffer_empty(){
    return items_in_buffer == 0;
}

/* Helper fuction */
int valid_operand(int operand){
    return operand != 0;
}

/*Print out the BB sequentially, for debugging*/
void print_bounded_buffer(){
    int i;
    for(i = 0; i < BOUNDED_BUFFER_SIZE; i++){
      printf("bounded_buffer[%d]: %d\n", i, bounded_buffer[i]);
    }
}

/* Add an item to the buffer */
void add_to_buffer(int value) {
    /*Ensure that access to the buffer is mutually exclusive*/
    pthread_mutex_lock(&buffer_access_lock);

    /*Wait until the buffer is not full*/
    while(buffer_full()){
      pthread_cond_wait(&not_full, &buffer_access_lock);
    }

    /* Critical Section */
    bounded_buffer[next_in] = value;
    next_in = (next_in + 1) % BOUNDED_BUFFER_SIZE;
    items_in_buffer++;

    /* Signal that the buffer is not empty */
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&buffer_access_lock);
}

/* Remove an item from the buffer */
int remove_from_buffer(){
    int value_removed;

    /*Ensure that access to the buffer is mutually exclusive*/
    pthread_mutex_lock(&buffer_access_lock);

    /*Wait until the buffer is not empty*/
    while(buffer_empty()){
      pthread_cond_wait(&not_empty, &buffer_access_lock);
    }

    /* Critical Section */
    value_removed = bounded_buffer[next_out];
    bounded_buffer[next_out] = 0;
    next_out = (next_out + 1) % BOUNDED_BUFFER_SIZE;
    items_in_buffer--;

    /* Signal that the buffer is not full */
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&buffer_access_lock);
    return value_removed;
}
