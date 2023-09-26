#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//Just need a random number for the array, it will work for any number you give it
#define MAX_FACTORS 10000
#define BUFFER_SIZE 10

// Initialize needed variables
int buffer[BUFFER_SIZE];
int bufferCount = 0;
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t bufferNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t bufferNotFull = PTHREAD_COND_INITIALIZER;

int producerBuffer[BUFFER_SIZE][MAX_FACTORS];
int producerBufferCount = 0;
int producerBufferWriteIndex = 0;
int producerBufferReadIndex = 0;
pthread_mutex_t producerBufferMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t producerBufferNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t producerBufferNotFull = PTHREAD_COND_INITIALIZER;
int producerBufferDone = 0;

void *producer(void *arg) {
  int *numbers = (int *)arg;
  int count = numbers[0];
  for (int i = 1; i <= count; i++) {
    int n = numbers[i];
    int factors[MAX_FACTORS];
    int factorCount = 0;
    int j = 2;
    while (n > 1) {
      if (n % j == 0) {
        factors[factorCount++] = j;
        n /= j;
      } else {
        j++;
      }
    }
    factors[factorCount] = -1;

    pthread_mutex_lock(&producerBufferMutex);
    while (producerBufferCount == BUFFER_SIZE) {
      pthread_cond_wait(&producerBufferNotFull, &producerBufferMutex);
    }

    int *bufferEntry = producerBuffer[producerBufferWriteIndex];
    bufferEntry[0] = numbers[i];
    for (int j = 0; j <= factorCount; j++) {
      bufferEntry[j + 1] = factors[j];
    }
    producerBufferWriteIndex = (producerBufferWriteIndex + 1) % BUFFER_SIZE;
    producerBufferCount++;

    pthread_cond_signal(&producerBufferNotEmpty);
    pthread_mutex_unlock(&producerBufferMutex);
  }
  //Check to see if buffer is done
  pthread_mutex_lock(&producerBufferMutex);
  producerBufferDone = 1;
  pthread_cond_broadcast(&producerBufferNotEmpty);
  pthread_mutex_unlock(&producerBufferMutex);
  return NULL;
}

void *consumer(void *arg) {
  while (1) {
    pthread_mutex_lock(&producerBufferMutex);
    while (producerBufferCount == 0 && !producerBufferDone) {
      pthread_cond_wait(&producerBufferNotEmpty, &producerBufferMutex);
    }
    //break condition, stop infinite loop
    if (producerBufferCount == 0 && producerBufferDone) {
      pthread_mutex_unlock(&producerBufferMutex);
      break;
    }

    int *bufferEntry = producerBuffer[producerBufferReadIndex];
    int n = bufferEntry[0];
    int *factors = &bufferEntry[1];
    printf("%d:", n);
    for (int i = 0; factors[i] != -1; i++) {
      printf(" %d", factors[i]);
    }
    printf("\n");

    producerBufferReadIndex = (producerBufferReadIndex + 1) % BUFFER_SIZE;
    producerBufferCount--;

    pthread_cond_signal(&producerBufferNotFull);
    pthread_mutex_unlock(&producerBufferMutex);
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("PLEASE ENTER NUMBERS TO FACTOR\n");
    return 1;
  }

  pthread_t producerThread, consumerThread;
  int numbers[argc];
  numbers[0] = argc - 1;
  for (int i = 1; i < argc; i++) {
    numbers[i] = atoi(argv[i]);
  }

  pthread_create(&producerThread, NULL, producer, numbers);
  pthread_create(&consumerThread, NULL, consumer, NULL);

  pthread_join(producerThread, NULL);

  while (1) {
    pthread_mutex_lock(&producerBufferMutex);
    if (producerBufferCount == 0) {
      pthread_mutex_unlock(&producerBufferMutex);
      break;
    }

    int *bufferEntry = producerBuffer[producerBufferReadIndex];
    int n = bufferEntry[0];
    int *factors = &bufferEntry[1];
    printf("%d:", n);
    for (int i = 0; factors[i] != -1; i++) {
      printf(" %d", factors[i]);
    }
    printf("\n");

    producerBufferReadIndex = (producerBufferReadIndex + 1) % BUFFER_SIZE;
    producerBufferCount--;

    pthread_cond_signal(&producerBufferNotFull);
    pthread_mutex_unlock(&producerBufferMutex);
  }

  pthread_join(consumerThread, NULL);

  return 0;
}
s