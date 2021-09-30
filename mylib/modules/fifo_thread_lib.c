#include "fifo_thread_lib.h"

int init_fifo_thread(fifo_thread_t* __fifo, const size_t __size_fifo, const size_t __size_line)
{
  int ret = 0;
  ret = pthread_mutex_init(&__fifo->mutex_busy, NULL);
  pthread_mutex_lock(&__fifo->mutex_busy); 
  ret += sem_init(&__fifo->sem_write, 0, __size_fifo);
  ret += sem_init(&__fifo->sem_read, 0, 0);
  
  __fifo->line = malloc(__size_fifo * sizeof(char*));
  for(int i = 0; i< __size_fifo; i++)
  {
    __fifo->line[i] = malloc(__size_line * sizeof(char));
  }
  __fifo->size_fifo = __size_fifo;
  __fifo->size_line = __size_line;
  
  __fifo->pt_read = 0;
  __fifo->pt_write = 0;



  pthread_mutex_unlock(&__fifo->mutex_busy); 
  return ret;
}

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

int add_fifo_thread(fifo_thread_t* __fifo, const char* __line)
{
  return sprintf_fifo_thread(__fifo, __line);
}

int sprintf_fifo_thread(fifo_thread_t* __fifo, const char *__restrict__ __format, ...)
{
  int done = -1;
  sem_wait(&__fifo->sem_write);
  pthread_mutex_lock(&__fifo->mutex_busy);
  
    va_list args;
    va_start(args, __format);
    done = vsprintf(__fifo->line[__fifo->pt_write], __format, args);
    va_end(args);
  
  
  __fifo->pt_write = (__fifo->pt_write+1) % __fifo->size_fifo; // update de write pointer
  sem_post(&__fifo->sem_read);
  
  pthread_mutex_unlock(&__fifo->mutex_busy);
  
  return done;
}

int pop_fifo_thread(fifo_thread_t* __fifo, char* __line)
{
  int _ret = 0;
    if(sem_trywait(&__fifo->sem_read) == 0)
    {
      pthread_mutex_lock(&__fifo->mutex_busy);
    

      _ret = 1+ sprintf(__line, "%s",__fifo->line[__fifo->pt_read]);
      _ret = 1; 
      __fifo->pt_read = (__fifo->pt_read+1) % __fifo->size_fifo; // update de write pointer
      
      sem_post(&__fifo->sem_write);
      pthread_mutex_unlock(&__fifo->mutex_busy);
    }

  return _ret;
}