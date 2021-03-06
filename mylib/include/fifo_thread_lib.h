/*
Author : Colle Chaude
Date : 10/12/2021
*/

#ifndef FIFO_THREAD_LIB_H
#define FIFO_THREAD_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

// fifo trhead is a thread safe fifo
typedef struct
{
  pthread_mutex_t mutex_busy; // lock fifo while modifying 
  sem_t sem_write; // nuber of line available
  sem_t sem_read; // nuber of line in the buffer
  char** line; // the buffer of lines

  size_t pt_write; // write index (should never be equal or bigger than size_fifo)
  size_t pt_read; // read index (should never be equal or bigger than size_fifo)

  size_t size_fifo; // nubre of line that th fifo can store 
  size_t size_line;  // maximum nuber of character per line
}fifo_thread_t;


int init_fifo_thread(fifo_thread_t* __fifo, const size_t __size_fifo, const size_t __size_line); // initialaise fifo (dynamic alocation of memory)
int destroy_fifo_thread(fifo_thread_t* __fifo);                                                  // destroy fifo an free memory alocated

int add_fifo_thread(fifo_thread_t* __fifo, const char* __line);                                  // add a new line in the fifo, blocking while fifo is full  
int sprintf_fifo_thread(fifo_thread_t* __fifo, const char *__restrict__ __format, ...);          // same as add_fifo_thread, but handle string paramters as %d, %s ..
int pop_fifo_thread(fifo_thread_t* __fifo, char* __line);                                        // get out the oldest line of the fifo and return 1 or retrun 0 if empty 
int try_pop_fifo_thread(fifo_thread_t* __fifo, char* __line);                                        // get out the oldest line of the fifo and return 1 or retrun 0 if empty 





#endif // FIFO_THREAD_LIB_H