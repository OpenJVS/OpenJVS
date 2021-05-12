#ifndef THREADING_H_
#define THREADING_H_

#include <pthread.h>

#define THREAD_MAX_NUMBER 32

typedef enum
{
    THREAD_STATUS_SUCCESS,
    THREAD_STATUS_ERROR,
    THREAD_STATUS_TOO_MANY_THREADS
} ThreadStatus;

typedef struct
{
    pthread_mutex_t mutex_manager;
    pthread_t threadID[THREAD_MAX_NUMBER];
    int threadCount;
    /* Data to be shared between threads */
    pthread_mutex_t mutex_threads;
    int ThreadsRunning;
} ThreadSharedData;

ThreadStatus initThreadManager();
ThreadStatus createThread(void *thread_entry(void *), void *args);
void stopAllThreads();
void setThreadsRunning(int Running);
int getThreadsRunning();

#endif // THREADING_H_