# CSC360 - A1 - Concurrent Bounded Buffer

## bbuffer.c
 * There exist 3 concurrency-control mechanisms: not_full, not_empty and buffer_access_lock
 * not_full is used to signal add_to_buffer, indicating there is a vacant space to add a value
 * not_empty is used to signal remove_from_buffer, indicating there is a value now able to be removed
 * the buffer_access_lock prevents is shared between both condition variables (not full and not empty) to prevent multiple accesses to the same buffer entry

## Main functions
 * Add to buffer inserts a value to the next in entry of the bounded-buffer
 * Add waits until it has the lock and the buffer is not full
 * It then inserts the value, and signals that the buffer is not empty (for threads waiting on that condition)
 * It then returns the lock so that other threads may access the buffer
 * Remove from buffer removes a value to the next out entry of the bounded-buffer
 * Remove waits until it has the lock and the buffer is not empty
 * It then removes the value, and signals that the buffer is not full (for threads waiting on that condition)
 * It then returns the lock so that other threads may access the buffer

### Helper functions
 * buffer_full returns true/false whether the buffer is full
 * buffer_empty returns true/false whether the buffer is empty
 * valid operand check whether atoi() succeeded. A value such as &*s is converted to 0 from atoi()
 * print_bounded_buffer prints the bounded buffer to stdout

## myserver.c
 * [add] calls add_to_buffer a the operand to the buffer if it is not 0 or not an integer
 * [remove] calls remove_from_buffer and returns the value it removed
 * [sum] calls remove_from_buffer equal to the operand provided. It then returns the sum of the values removed
 * [debug] toggles on debug mode. With debug mode ON, the bounded buffer will be printed after every server action

## Scheduled Add
In this version of the bounded buffer, the add operation works slightly different. There is now a pseudo-round-robin scheduler. When the add operation is called, a new scheduled process is created and added to the scheduler. The scheduler sleeps periodically, and checks if there are processes on the scheduler (max number of processes is equal to the buffer size). When a process is called upon the scheduler, it will add as much to bounded_buffer[insertion_index] as possible (MAX_ADD_VALUE).
If there are still some value to add remaining, it will put the process at the back of scheduler with the value remaining to be added. A value can only be removed once the process is finished. Also maintained is an array of which processes have finished, this stops an early ADD from causing a removal to take 0. In other words, the process filling in next_out must complete for a removal to occur.

## EXAMPLE:
* bounded_buffer of size 3 [-][-][-]
* MAX_ADD_VALUE of 16
* T0 REMOVE
* bbuffer is empty, T0 will wait
* T1 REMOVE
* bbuffer is empty, T1 will wait
* Four sequenced add requests come in, ADD 40, ADD 20, ADD 5, ADD 6
* 3 process added to schedule, ADD 40, ADD, 20, ADD 5
* ADD 6 will create a process once a spot opens
* Scheduler wakes up

## FIRST ROUND
* bounded_buffer [16][16][5]
* ADD 5 is done, we cannot remove yet because ADD 40 is unfinished

## SECOND ROUND
* bounded_buffer [32][20][5]
* ADD 20 is done, we cannot remove yet because ADD 40 is unfinished

## THIRD ROUND
* bounded_buffer [40][20][5]
* REMOVE 40 goes through
* bounded_buffer [0][20][5]
* ADD 6 goes onto the scheduler
* since ADD 20 is done it will be removed
* bounded_buffer [6][0][5]
