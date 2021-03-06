/*
Author : Colle Chaude
*/

#include "fifo_thread_lib.h"

// initialise fifo (dynamic alocation of memory)
int init_fifo_thread(fifo_thread_t* __fifo, const size_t __size_fifo, const size_t __size_line)
{
  int ret = 0;
  ret = pthread_mutex_init(&__fifo->mutex_busy, NULL); // mutex lock fifo while modifying
  pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
  ret += sem_init(&__fifo->sem_write, 0, __size_fifo); // semaphore number of empty line
  ret += sem_init(&__fifo->sem_read, 0, 0);// semaphore number of line in the buffer
  
  __fifo->line = malloc(__size_fifo * sizeof(char*)); // allocate memory for the buffer
  for(int i = 0; i< __size_fifo; i++)
  {
    __fifo->line[i] = malloc(__size_line * sizeof(char));
  }
  __fifo->size_fifo = __size_fifo;
  __fifo->size_line = __size_line;
  
  __fifo->pt_read = 0;
  __fifo->pt_write = 0;



  pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  return ret;
}

// destroy fifo an free memory alocated
// unallocate memory and destroy semaphore and mutex
int destroy_fifo_thread(fifo_thread_t* __fifo)
{
  int ret = 0; 
  pthread_mutex_lock(&__fifo->mutex_busy); 
  
  for(int i = 0; i< __fifo->size_fifo; i++)
  {
    free(__fifo->line[i]);
  }
  free(__fifo->line);
  
  ret += sem_destroy(&__fifo->sem_read);
  ret += sem_destroy(&__fifo->sem_write);
  ret += pthread_mutex_unlock(&__fifo->mutex_busy); 
  pthread_mutex_destroy(&__fifo->mutex_busy);
  return ret;
}

// add a new line in the fifo,
// blocking while fifo is full
int add_fifo_thread(fifo_thread_t* __fifo, const char* __line)
{
  return sprintf_fifo_thread(__fifo, __line);
}

// add a new line in the fifo,
// blocking while fifo is full
// handle string paramters as %d, %s ..
int sprintf_fifo_thread(fifo_thread_t* __fifo, const char *__restrict__ __format, ...)
{
  int done = -1;
  sem_wait(&__fifo->sem_write); // wait for an empty line
  pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
  
    va_list args;
    va_start(args, __format);
    done = vsprintf(__fifo->line[__fifo->pt_write], __format, args); // write the line in the buffer
    va_end(args);
  
  
  __fifo->pt_write = (__fifo->pt_write+1) % __fifo->size_fifo; // update de write pointer
  sem_post(&__fifo->sem_read); //  update the number of line in the buffer
  
  pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  return done;
}

// get out the oldezst line of the fifo and return 1 or retrun 0 if empty
int pop_fifo_thread(fifo_thread_t* __fifo, char* __line)
{
  int _ret = 0;

  sem_wait(&__fifo->sem_read); // wait for a line in the buffer
  pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
  strcpy(__line, __fifo->line[__fifo->pt_read]); // copy the line in the buffer
  //sprintf(__line, "%s",__fifo->line[__fifo->pt_read]); // on copie la ligne dans la variable
  __fifo->pt_read = (__fifo->pt_read+1) % __fifo->size_fifo; // update de read pointer
  sem_post(&__fifo->sem_write); // update the number of empty line
  pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  _ret = 1;

  return _ret; // return 1 if there is a line in the buffer else return 0
}

// get out the oldest line of the fifo and return 1 or retrun 0 if empty, non blocking
int try_pop_fifo_thread(fifo_thread_t* __fifo, char* __line)
{
  int _ret = 0;
    if(sem_trywait(&__fifo->sem_read) == 0)// if there is a line in the buffer
    {
      pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
      strcpy(__line, __fifo->line[__fifo->pt_read]); // copy the line in the buffer
      //sprintf(__line, "%s",__fifo->line[__fifo->pt_read]); // on copie la ligne dans la variable
      __fifo->pt_read = (__fifo->pt_read+1) % __fifo->size_fifo; // update de read pointer
      sem_post(&__fifo->sem_write); // update the number of empty line
      pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
      _ret = 1;
    }

  return _ret; // return 1 if there is a line in the buffer else return 0
}


