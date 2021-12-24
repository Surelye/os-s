#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define THREAD_NUM 8

pthread_mutex_t mutex_buffer;

sem_t sem_empty;
sem_t sem_full;

int buffer[10];
int count = 0;

void * producer(void * args)
{
  while (1)
    {
      // Produce
      int x = rand() % 100;
      // sleep(1);
      
      sem_wait(&sem_empty);
      pthread_mutex_lock(&mutex_buffer);
      buffer[count++] = x;
      pthread_mutex_unlock(&mutex_buffer);
      sem_post(&sem_full);
    }
}

void * consumer(void * args)
{
  while(1)
    {
      int y;
      
      sem_wait(&sem_full);
      pthread_mutex_lock(&mutex_buffer);
      y = buffer[count - 1];
      count--;
      pthread_mutex_unlock(&mutex_buffer);
      sem_post(&sem_empty);

      // Consume
      printf("Got a number %d.\n", y);
      // sleep(1);
    }
}

int main(int argc, char * argv[])
{
  srand(time(NULL));
  pthread_t threads[THREAD_NUM];
  pthread_mutex_init(&mutex_buffer, NULL);
  sem_init(&sem_empty, 0, 10); // отвечает за количество свободных мест в буфере
  sem_init(&sem_full, 0, 0); 
  int i;

  if (pthread_create(&threads[0], NULL, &consumer, NULL) != 0)
    {
      perror("Failed to create thread.\n");
    }
  
  for (i = 1; i < THREAD_NUM; i++)
    {
      if (pthread_create(&threads[i], NULL, &producer, NULL) != 0)
	{
	  perror("Failed to create thread.\n");
	}
    }

  for (i = 0; i < THREAD_NUM; i++)
    {
      if (pthread_join(threads[i], NULL) != 0)
	{
	  perror("Failed to join thread.\n");
	}
    }
}
