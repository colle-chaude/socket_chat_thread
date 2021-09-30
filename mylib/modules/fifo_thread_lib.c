#include "fifo_thread_lib.h"

int init_fifo_thread(fifo_thread_t* __fifo, const size_t __size_fifo, const size_t __size_line)
{
  pthread_mutex_init(&__fifo->mutex_busy, NULL);
  pthread_mutex_lock(&__fifo->mutex_busy); 
  sem_init(&__fifo->sem_available, 0, __size_fifo);
  
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
}

int destroy_fifo_thread(fifo_thread_t* __fifo)
{
  pthread_mutex_lock(&__fifo->mutex_busy); 
  sem_destroy(&__fifo->sem_available);
  
  for(int i = 0; i< __fifo->size_fifo; i++)
  {
    free(__fifo->line[i]);
  }
  free(__fifo->line);
  pthread_mutex_unlock(&__fifo->mutex_busy); 
  pthread_mutex_destroy(&__fifo->mutex_busy);

}

int add_fifo_thread(fifo_thread_t* __fifo, const char* __line)
{
  sem_wait(&__fifo->sem_available);// wait for an available line in the fifo 
  pthread_mutex_lock(&__fifo->mutex_busy); // lock the fifo
  strcpy(__fifo->line[__fifo->pt_write], __line); // store the new line
  __fifo->pt_write++; // update de write pointer
  if(__fifo->pt_write >= __fifo->size_fifo)
  {
    __fifo->pt_write = 0;
  }
  pthread_mutex_unlock(&__fifo->mutex_busy);
}

int sprintf_fifo_thread(fifo_thread_t* __fifo, const char *__restrict__ __format, ...)
{
  int done = -1;
  sem_wait(&__fifo->sem_available);
  pthread_mutex_lock(&__fifo->mutex_busy);
  //strcpy(__fifo->line[__fifo->pt_write], __line);
  
    va_list args;
    va_start(args, __format);
    done = vsprintf(__fifo->line[__fifo->pt_write], __format, args);
    va_end(args);
  
  
  __fifo->pt_write++;
  if(__fifo->pt_write >= __fifo->size_fifo)
  {
    __fifo->pt_write = 0;
  }
  pthread_mutex_unlock(&__fifo->mutex_busy);
  
  return done;
}

int pop_fifo_thread(fifo_thread_t* __fifo, char* __line)
{
  int _ret = 0;
  sem_getvalue(&__fifo->sem_available, &_ret);
  if(_ret< __fifo->size_fifo)
  {
    pthread_mutex_lock(&__fifo->mutex_busy);
    
    sem_getvalue(&__fifo->sem_available, &_ret);
    if(_ret< __fifo->size_fifo)
    {

      strcpy(__line, __fifo->line[__fifo->pt_read]);
      __fifo->pt_read++;
      if(__fifo->pt_read >= __fifo->size_fifo)
      {
        __fifo->pt_read = 0;
      }
      sem_post(&__fifo->sem_available);
      _ret = 1;
    
    }
    else
      _ret = 0;
    
    pthread_mutex_unlock(&__fifo->mutex_busy);
  }
  else
    _ret = 0;

  return _ret;
}