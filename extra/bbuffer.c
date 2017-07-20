/*Required Headers*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bbuffer.h"
#include <semaphore.h>
#include "server.h"
#include <time.h>
#include "requests.h"
#include <stdbool.h>

/*
 * Declarations for bounded-buffer shared variables -- plus concurrency-control
 * variables -- must START here.
 */
struct buffer_process{
     int process_id;
     int value_remaining;
     int insertion_index;
     clock_t start_time;
     struct buffer_process* next_process;
 };

int process_count;
int bounded_buffer[BOUNDED_BUFFER_SIZE];
bool finished[BOUNDED_BUFFER_SIZE];
int next_in, next_out, items_in_buffer;
pthread_mutex_t buffer_access_lock, schedule_access_lock;
pthread_cond_t not_empty, not_full;
struct buffer_process* current_process = NULL;
struct buffer_process* last_process = NULL;

/* Initialize sequencing items for the bounded buffer */
void initialize_bounded_buffer() {
    next_in,next_out, items_in_buffer, process_count = 0;
    int interval = 5;
    int status, i;
    pthread_t schedule_thread;

    if (pthread_create(&schedule_thread, 0, schedule, (void *)&interval) < 0) {
        fprintf(stderr, "Could not create schedule thread\n");
        exit(1);
    }

    current_process = malloc(sizeof(struct buffer_process));
    if (current_process == NULL) {
      fprintf(stderr, "Error creating current_process pointer\n");
      exit(1);
    }

    last_process = malloc(sizeof(struct buffer_process));
    if (last_process == NULL) {
      fprintf(stderr, "Error creating last_process pointer\n");
      exit(1);
    }

    status = pthread_mutex_init(&buffer_access_lock, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating buffer_access_lock\n");
        exit(1);
    }

    status = pthread_cond_init(&not_empty, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating not_empty condtion\n");
        exit(1);
    }

    status = pthread_cond_init(&not_full, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating not_full condtion\n");
        exit(1);
    }
}

/* The schedule function that runs, it sleeps periodically if it has no work to do */
void *schedule(void *arg) {
    int interval = *(int *)arg;
    struct buffer_process* process_ptr;

    for (;;) {
        if(process_count == 0){
            printf("<3 Schedule is empty ... time for a nap\n");
            sleep(interval);
        } else {

            /* Grab the next process */
            process_ptr = current_process->next_process;
            if(process_ptr->value_remaining - MAX_ADD_VALUE > 0){
              process_ptr->value_remaining = (process_ptr->value_remaining - MAX_ADD_VALUE);
              add_to_buffer(MAX_ADD_VALUE, process_ptr->insertion_index);
              move_current_process_to_end();
            } else {
              printf("Finishing process %d\n",process_ptr->process_id);
              add_to_buffer(process_ptr->value_remaining, process_ptr->insertion_index);
              finish_current_buffer_process();
            }
        }
    }
}

void create_buffer_process(int value_to_add){

    /* Wait until the buffer & schedule are not full, and make sure we don't insertion_index
    over a process that has not finished processing */
    pthread_mutex_lock(&schedule_access_lock);
    while(schedule_full() || buffer_full() || finished[next_in] == true){
      pthread_cond_wait(&not_full, &schedule_access_lock);
    }
    pthread_mutex_lock(&buffer_access_lock);
    struct buffer_process *new_process = malloc(sizeof(struct buffer_process));

    if (new_process == NULL) {
      fprintf(stderr, "Error creating new buffer_process\n");
      exit(1);
    }

    new_process->start_time = clock();
    new_process->process_id = process_count;
    new_process->value_remaining = value_to_add;
    new_process->insertion_index = next_in;
    next_in = (next_in + 1) % BOUNDED_BUFFER_SIZE;
    printf("Created buffer process %d with add value of %d\n", process_count, value_to_add);

    if(process_count == 0){
      current_process->next_process = new_process;
      last_process->next_process = new_process;
    } else {
      last_process->next_process->next_process = new_process;
      last_process->next_process = new_process;
    }

    process_count++;
    items_in_buffer++;
    pthread_mutex_unlock(&buffer_access_lock);
    pthread_mutex_unlock(&schedule_access_lock);
}

void finish_current_buffer_process(){
    clock_t end_time;
    double total_time = 0;

    if(process_count > 0){
      pthread_mutex_lock(&schedule_access_lock);
      struct buffer_process* process_ptr = current_process->next_process;
      current_process->next_process = current_process->next_process->next_process;

      end_time = clock();
      total_time = (double)(end_time - process_ptr->start_time) / CLOCKS_PER_SEC;
      finished[process_ptr->insertion_index] = true;
      printf("Process %d has finished. Total time: %f\n", process_ptr->process_id ,total_time);
      free(process_ptr);
      process_count--;
      print_bounded_buffer();
      
      pthread_cond_signal(&not_empty);
      pthread_mutex_unlock(&schedule_access_lock);
    }
}

void move_current_process_to_end(){
    pthread_mutex_lock(&schedule_access_lock);
    if(process_count > 1){
      last_process->next_process->next_process = current_process->next_process;
      current_process->next_process = current_process->next_process->next_process;
      last_process->next_process = last_process->next_process->next_process;
      last_process->next_process->next_process = NULL;
    }
    pthread_mutex_unlock(&schedule_access_lock);
}

/* Helper fuction */
int buffer_empty(){
    return items_in_buffer == 0;
}

int buffer_full(){
    return items_in_buffer == BOUNDED_BUFFER_SIZE;
}

int schedule_full(){
    return process_count == BOUNDED_BUFFER_SIZE;
}

/* Helper fuction */
int valid_operand(int operand){
    return operand != 0;
}

/*Print out the BB sequentially, for debugging*/
void print_bounded_buffer(){
    int i;
    printf("items in buffer %d     process_count %d\n",items_in_buffer,process_count);
    for(i = 0; i < BOUNDED_BUFFER_SIZE; i++){
      printf("bounded_buffer[%d]: %d     finished? %d\n", i, bounded_buffer[i], finished[i]);
    }
}

/* Add an item to the buffer */
void add_to_buffer(int value, int insertion_index) {
    pthread_mutex_lock(&buffer_access_lock);
    bounded_buffer[insertion_index] += value;
    pthread_mutex_unlock(&buffer_access_lock);
}

/* Remove an item from the buffer */
int remove_from_buffer(){
    int value_removed;

    /*Ensure that access to the buffer is mutually exclusive*/
    pthread_mutex_lock(&buffer_access_lock);

    /*Wait until the buffer is not empty*/
    while(buffer_empty() && (finished[next_out] == false)){
      pthread_cond_wait(&not_empty, &buffer_access_lock);
    }

    /* Critical Section */
    value_removed = bounded_buffer[next_out];
    bounded_buffer[next_out] = 0;
    finished[next_out] = false;
    next_out = (next_out + 1) % BOUNDED_BUFFER_SIZE;
    items_in_buffer--;
    print_bounded_buffer();

    /* Signal that the buffer is not full */
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&buffer_access_lock);
    return value_removed;
}
