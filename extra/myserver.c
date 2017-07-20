#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "requests.h"
#include "server.h"

/*
 * This data needed to manage the pool of Pthreads available to
 * respond to client requests. It is a little tricky to determine
 * when a thread is completed, but we take advantage here that
 * server threads (other than the "heartbeat" thread) are only ever
 * created when some file descriptor is opened for a client request.
 */
int thread_fd[MAX_THREADS];
pthread_t thread_pool[MAX_THREADS];
pthread_mutex_t pool_mutex;
int debug_mode = 0;

void *heartbeat(void *);
void *server_action(void *);


/*
 * Some functions to help with finding the first available thread from the
 * "pool" of threads (i.e., really the array named "thread_pool").
 */


int grab_thread_index() {
    int i;
    int result = -1;

    pthread_mutex_lock(&pool_mutex);
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_fd[i] == -1) {
            result = i;
            break;
        }
    }
    pthread_mutex_unlock(&pool_mutex);

    return result;
}


void set_thread_fd(int index, int fd) {
    if (0 <= index && index < MAX_THREADS) {
        thread_fd[index] = fd;
    }
}


void return_thread_index(int fd) {
    int i;

    pthread_mutex_lock(&pool_mutex);
    for (i = 0; i < MAX_THREADS; i++) {
        if (thread_fd[i] == fd) {
            thread_fd[i] = -1;
            break;
        }
    }
    pthread_mutex_unlock(&pool_mutex);
}


/*
 * TO DO:
 *
 * For each of the cases in the switch statement, you'll need to
 * call appropriate bounded-buffer functions and work with
 * the results to compute what is needed.
 *
 * YOU ARE NOT TO ADD ANY POSIX SYNCHRONIZATION OPERATIONS TO THIS
 * FUNCTION. All synchronization MUST take place in bbuffer.c.
 */
void *server_action(void *arg) {
    int comm_fd = *(int *)arg;
    char str[OUTPUT_BUFFER_SIZE];
    char operand[OUTPUT_BUFFER_SIZE];
    char result_message[OUTPUT_BUFFER_SIZE];
    char temp_result[OUTPUT_BUFFER_SIZE];
    char *separator = "";
    int  value, operand_int;
    int sum, i;
    pc_op_t operation;

    bzero(str, OUTPUT_BUFFER_SIZE);
    read(comm_fd, str, OUTPUT_BUFFER_SIZE);
    operation = parse_request(str, operand);
    operand_int =  atoi(operand);

    switch(operation) {
        case REMOVE:
            printf("server: REMOVE\n");
            value = remove_from_buffer();
            sprintf(result_message, "client: REMOVED %d\n", value);
            break;

        case ADD:
            printf("server: ADD\n");
            printf("server: parameter is %s\n", operand);
            if(valid_operand(operand_int)){
              create_buffer_process(operand_int);
              sprintf(result_message, "client: ADDED PROCESS TO SCHEDULE\n");
            } else {
              sprintf(result_message, "Operand was either 0 or not an integer\n");
            }
            break;

        case SUM:
            printf("server: SUM\n");
            printf("server: parameter is %s\n", operand);
            if(valid_operand(operand_int)){
              sum = 0;
              for(i = 0; i < operand_int; i++){
                sum += remove_from_buffer();
              }
              sprintf(result_message, "client: REMOVED %d VALUES ... SUM IS %d\n", operand_int, sum);
            } else {
              sprintf(result_message, "Operand was either 0 or not an integer\n");
            }
            break;

        case DEBUG:
            printf("server: DEBUG\n");
            if(debug_mode){
                sprintf(result_message, "DEBUG MODE DISABLED\n");
                debug_mode = 0;
            } else {
                sprintf(result_message, "DEBUG MODE ENABLED\n");
                debug_mode = 1;
            }
            break;

        case UNKNOWN:
        default:
            printf("server: UNKNOWN\n");
            sprintf(result_message, "client: UNKNOWN OPERATION\n");
            break;
    }

    if(debug_mode){
      print_bounded_buffer();
    }

    send_http_response(comm_fd, result_message);
    close(comm_fd);
    return_thread_index(comm_fd);
}



void initialize() {
    int status;
    int i;

    /*
     * Each thread created by server-loop in main() will have the file
     * description # for which it was generated stored in thread_fd.
     * This array will also server as a proxy for what threads are available,
     * i.e., if thread_fd[i] == -1 then thread[i] is available.
     *
     */
    for (i = 0; i < MAX_THREADS; i++) {
        thread_fd[i] = -1;
    }

    status = pthread_mutex_init(&pool_mutex, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating pool_mutex\n");
        exit(1);
    }

    /*
     * This function calls YOUR code for initializing needed bounded-buffer
     * data- and sychronization structures.
     */
    initialize_bounded_buffer();
}


void *heartbeat(void *arg) {
    int interval = *(int *)arg;

    for (;;) {
        printf("<3\n");
        sleep(interval);
    }
}

int main() {

    /* Vars for setting up heartbeat */
    pthread_t heart, schedule;
    int interval = 5;

    /* Vars for setting up server threads */
    char str[OUTPUT_BUFFER_SIZE];
    int listen_fd, comm_fd;
    int i;
    int thread_index;
    int num_threads = 0;
    int bind_result;
    void *result;
    int status;


    initialize();


    /*
     * First start the heartbeat -- just to convince ourselves that
     * "accept()" is not blocking the whole process...
     *
     */

    if (pthread_create(&heart, 0, heartbeat, (void *)&interval) < 0) {
        fprintf(stderr, "Could not create heartbeat thread\n");
        return 1;
    }

    /*
     * Now set up server threads as needed -- i.e., as separate clients
     * connect to the server.
     */

    listen_fd = setup_listener();

    while (comm_fd = accept(listen_fd, (struct sockaddr*) NULL, NULL)) {

        thread_index = grab_thread_index();

        if (thread_index == -1) {
            fprintf(stderr, "No more threads available.\n");
            exit(1);
        }

        set_thread_fd(thread_index, comm_fd);

        if (pthread_create(&thread_pool[thread_index], 0,
                server_action, &comm_fd) < 0)
        {
            fprintf(stderr, "Could not create thread %d\n", num_threads);
            exit(1);
        }
        num_threads = thread_index;
        printf("Created server thread %d\n", num_threads);
    }
}
