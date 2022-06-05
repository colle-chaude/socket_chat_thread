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





// initialise fifo (dynamic alocation of memory)
int init_fifo_thread_sub(fifo_thread_sub_t* __fifo, const size_t __size_fifo, const size_t __size_line, const int __nb_sub_max)
{
  int ret = 0;
  ret = pthread_mutex_init(&__fifo->mutex_busy, NULL); // mutex lock fifo while modifying
  pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo

  __fifo->sem_read = malloc(sizeof(sem_t)*__nb_sub_max); // allocate memory for the semaphore
    //pthread_sem_init(&__fifo->sem_read[i], 0, 0); // semaphore number of empty line
  

  __fifo->line = malloc(__size_fifo * sizeof(char*)); // allocate memory for the buffer
  for(int i = 0; i< __size_fifo; i++)
  {
    __fifo->line[i] = malloc(__size_line * sizeof(char));
  }
  __fifo->size_fifo = __size_fifo;
  __fifo->size_line = __size_line;
  
  __fifo->pt_write = 0;

  __fifo->nb_sub = 0;
  __fifo->nb_sub_max = __nb_sub_max;



  pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  return ret;
}

// destroy fifo an free memory alocated
// unallocate memory and destroy semaphore and mutex
int destroy_fifo_thread_sub(fifo_thread_sub_t* __fifo)
{
  int ret = 0; 
  pthread_mutex_lock(&__fifo->mutex_busy); 
  
  for(int i = 0; i< __fifo->size_fifo; i++)
  {
    free(__fifo->line[i]);
  }
  free(__fifo->line);
  
  for(int i = 0; i<__fifo->nb_sub; i++)
  {
    ret += sem_destroy(&__fifo->sem_read[i]);
  }
  free(__fifo->sem_read);
  
  
  ret += pthread_mutex_unlock(&__fifo->mutex_busy); 
  pthread_mutex_destroy(&__fifo->mutex_busy);
  return ret;
}



int add_sub_fifo_thread_sub(fifo_thread_sub_t* __fifo)
{
  int ret = -1;
  pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
  if(__fifo->nb_sub < __fifo->nb_sub_max)
  {
    ret = __fifo->nb_sub;
    __fifo->nb_sub++;
  }
  pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  return ret;
}

int post_new_line_fifo_thread_sub(fifo_thread_sub_t* __fifo) // update the fullness of the buffer
{
  int ret = 0;
  for(int i = 0; i<__fifo->nb_sub; i++)
  {
    int val;
    sem_getvalue(&__fifo->sem_read[i], &val);
    if(val < __fifo->size_fifo)
    {
      sem_post(&__fifo->sem_read[i]);
    }
    else 
    {
      ret ++;
    }
  }
  return ret;
}


// add a new line in the fifo,
// blocking while fifo is full
int add_fifo_thread_sub(fifo_thread_sub_t* __fifo, const char* __line)
{
  return sprintf_fifo_thread_sub(__fifo, __line);
}

// add a new line in the fifo,
// blocking while fifo is full
// handle string paramters as %d, %s ..
int sprintf_fifo_thread_sub(fifo_thread_sub_t* __fifo, const char *__restrict__ __format, ...)
{
  int done = -1;
  pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
  
    va_list args;
    va_start(args, __format);
    done = vsprintf(__fifo->line[__fifo->pt_write], __format, args); // write the line in the buffer
    va_end(args);
  
  
  __fifo->pt_write = (__fifo->pt_write+1) % __fifo->size_fifo; // update de write pointer

  post_new_line_fifo_thread_sub(__fifo); // update the number of empty line
  
  pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  return done;
}

// get out the oldezst line of the fifo and return 1 or retrun 0 if empty
int pop_fifo_thread_sub(fifo_thread_sub_t* __fifo, char* __line, const int __sub)
{
  int _ret = 0;
  int index;
  sem_wait(&__fifo->sem_read[__sub]); // wait for a line in the buffer
  pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
  
  if(sem_getvalue(&__fifo->sem_read[__sub], &index) == 0)
  {
    index = (__fifo->pt_write - (index +1) ) % __fifo->size_fifo; // update de read pointer

    strcpy(__line, __fifo->line[index]); // copy the line in the buffer
    
    _ret = 1;
  }

  pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  return _ret; // return 1 if there is a line in the buffer else return 0
}

// get out the oldest line of the fifo and return 1 or retrun 0 if empty, non blocking
int try_pop_fifo_thread_sub(fifo_thread_sub_t* __fifo, char* __line, const int __sub)
{
  int _ret = 0;
  int index;

  if(sem_trywait(&__fifo->sem_read[__sub]) == 0)// if there is a line in the buffer
  {
    pthread_mutex_lock(&__fifo->mutex_busy); // lock fifo
      
      if(sem_getvalue(&__fifo->sem_read[__sub], &index) == 0)
      {
        index = (__fifo->pt_write - (index +1) ) % __fifo->size_fifo; // update de read pointer

        strcpy(__line, __fifo->line[index]); // copy the line in the buffer
        
        _ret = 1;
      }

      pthread_mutex_unlock(&__fifo->mutex_busy); // unlock fifo
  }

  return _ret; // return 1 if there is a line in the buffer else return 0


}