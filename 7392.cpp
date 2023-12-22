/* a simple producer/consumer using semaphores and threads
   usage on Linux:
     g++ -o 7392 7392.cpp -lpthread
     ./7392
*/
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <queue>

#define BUFFER_SIZE 3

void *mCounter(void *);
void *mMonitor(void *);
void *mCollector(void *);

std::queue<int> buffer;
int counter = 0;

sem_t counterMutex;
sem_t bufferMutex;
sem_t bufferFull;
sem_t bufferEmpty;

/* main() -- read command line and create threads, then
             print result when the threads have quit */

int main() {
  // Seed random number generator
  srandom(time(NULL));
  // Initialize semaphores
  sem_init(&counterMutex, 0, 1);
  sem_init(&bufferMutex, 0, 1);
  sem_init(&bufferFull, 0, 0);
  sem_init(&bufferEmpty, 0, BUFFER_SIZE);

  // Create mCounter threads
  const int numCounters = 5;
  pthread_t counterThreads[numCounters];
  int counterThreadIds[numCounters];
  for (int i = 0; i < numCounters; i++) {
    counterThreadIds[i] = i;
    pthread_create(&counterThreads[i], NULL, mCounter, &counterThreadIds[i]);
  }

  // Create mMonitor thread
  pthread_t monitorThread;
  pthread_create(&monitorThread, NULL, mMonitor, NULL);

  // Create mCollector thread
  pthread_t collectorThread;
  pthread_create(&collectorThread, NULL, mCollector, NULL);

  // Wait for all threads to finish
  for (int i = 0; i < numCounters; i++) {
    pthread_join(counterThreads[i], NULL);
  }
  pthread_join(monitorThread, NULL);
  pthread_join(collectorThread, NULL);

  // Destroy semaphores
  sem_destroy(&counterMutex);
  sem_destroy(&bufferMutex);
  sem_destroy(&bufferFull);
  sem_destroy(&bufferEmpty);

  return 0;
}

void *mCounter(void *arg) {
  int sleepPeriod = random() % 1000000; // microseconds
  int threadId = *static_cast<int *>(arg);
  while (true) {
    usleep(sleepPeriod);
    printf("Counter thread %d: received a message\n", threadId);

    printf("Counter thread %d: waiting to write\n", threadId);
    sem_wait(&counterMutex);

    counter++;
    printf("Counter thread %d: now adding to counter, counter value=%d\n", threadId, counter);
    sem_post(&counterMutex);
  }

  return NULL;
}

void *mMonitor(void *arg) {
  int t = random() % 1000000; // Time interval in microseconds
  while (true) {
    usleep(t);

    printf("Monitor thread: waiting to read counter\n");
    sem_wait(&counterMutex);
    int counterValue = counter;
    printf("Monitor thread: reading a count value of %d\n", counterValue);
    counter = 0;
    sem_post(&counterMutex);

    sem_wait(&bufferEmpty);
    sem_wait(&bufferMutex);
    if (buffer.size() < BUFFER_SIZE) {
      if (counterValue == 0) {
        printf("Monitor thread: nothing to write\n");
      }
      else{
        printf("Monitor thread: writing to buffer at position %d\n", buffer.size());
        buffer.push(counterValue);
      }
    } else {
      printf("Monitor thread: Buffer full!!\n");
    }
    sem_post(&bufferMutex);
    sem_post(&bufferFull);
  }

  return NULL;
}


void *mCollector(void *arg) {
  int t = random() % 100000; // Time interval in microseconds
  while (true) {
    usleep(t);

    sem_wait(&bufferFull);
    sem_wait(&bufferMutex);
    if (!buffer.empty()) {
      int value = buffer.front();
      buffer.pop();
      printf("Collector thread: reading from the buffer at position %d\n", buffer.size());
    } else {
      printf("Collector thread: nothing is in the buffer!\n");
    }
    sem_post(&bufferMutex);
    sem_post(&bufferEmpty);
  }

  return NULL;
}
