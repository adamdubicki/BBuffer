#ifndef _BBUFFER_H_
#define _BBUFFER_H_

#define BOUNDED_BUFFER_SIZE 3
#define MAX_ADD_VALUE 20

void initialize_bounded_buffer();
void *schedule();
void create_buffer_process(int);
void finish_current_buffer_process();
void move_current_process_to_end();
int schedule_full();
int buffer_empty();
int valid_operand(int);
void print_bounded_buffer();
void add_to_buffer(int,int);
int remove_from_buffer();

#endif
