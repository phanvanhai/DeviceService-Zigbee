
#include <stdio.h>
#include <pthread.h> 
#include<string.h>
#include <stdlib.h>
#include <string.h>
#include "queue1.h"
//QueueInit(&CoreData,&MetaData)
pthread_mutex_t a_mutex; /*Khai báo biến mutex toàn cục*/
char test[39];
void* do_loop (void* data)
{
  for(int i=0;i<500;i++)
  {
    pthread_mutex_lock (&a_mutex);
    AddNode(&CoreData,"000");
    pthread_mutex_unlock (&a_mutex);
  }
}
void* do_loop2 (void* data)
{

  for(int i=0;i<500;i++)
  {
    pthread_mutex_lock (&a_mutex);
    AddNode(&CoreData,"000");
    pthread_mutex_unlock (&a_mutex);
  }
}
void* do_loop1 (void* data)
{
  for (int i=0;i<500;i++)
  {
    if(!TAILQ_EMPTY(&CoreData))
    {
      pthread_mutex_lock (&a_mutex);
      TakeNode(&CoreData,test);
      printf("%s\n",test);
      pthread_mutex_unlock (&a_mutex);

    }
    else printf("\nEmpty\n");
    
  }
}


int main (int agrc, char* argv[])
{
  int res;
  res = pthread_mutex_init (&a_mutex, NULL);
  QueueInit(&CoreData,&MetaData);
  AddNode(&CoreData,"thanhtung123");
  int thr_id; 
  pthread_t p_thread; 
  int thr_id1; 
  pthread_t p_thread1;
  int thr_id12; 
  pthread_t p_thread2;
  thr_id = pthread_create (&p_thread, NULL, do_loop, NULL);
  thr_id = pthread_create (&p_thread1, NULL, do_loop1, NULL);
  thr_id = pthread_create (&p_thread2, NULL, do_loop2, NULL);
  pthread_join (p_thread, NULL);
  pthread_join (p_thread2, NULL);
  //return 0;
  //printf("%s",data1);
//thr_id = pthread_create (&p_thread1, NULL, do_loop1, NULL);
  pthread_join (p_thread1, NULL);

  printf("\nFINISH\n");
  return 0;
}
  //do_loop ((void*)b);