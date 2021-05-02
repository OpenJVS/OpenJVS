#ifndef THREADING_H_
#define THREADING_H_

#include <pthread.h>

#define THREAD_MAX_NUMBER 32

typedef struct
{
    pthread_mutex_t mutex_manager;
    pthread_t threadID[THREAD_MAX_NUMBER];
    int threadCount;
    /* Data to be shared between threads */
    pthread_mutex_t mutex_threads;
    int ThreadsRunning;
} ThreadSharedData;

void ThreadManagerInit(void);
void ThreadManagerSetRunnable(void);
void ThreadManagerStopAll(void);
int ThreadManagerCreate(void *thread_entry(void *), void *args);
void ThreadSharedDataRunninSet(int Running);
int ThreadSharedDataRunninGet(void);
#endif // THREADING_H_