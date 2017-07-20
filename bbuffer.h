#ifndef _BBUFFER_H_
#define _BBUFFER_H_

#define BOUNDED_BUFFER_SIZE 3

void initialize_bounded_buffer();
int buffer_full();
int buffer_empty();
int valid_operand(int operand);
void print_bounded_buffer();
void add_to_buffer(int);
int  remove_from_buffer();

#endif
