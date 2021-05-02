
#include <string.h>
#include <stdio.h>

#include "console/debug.h"
#include "controller/threading.h"

static ThreadSharedData ThreadManagerData;

/* Only call this function once from main-thread */
ThreadStatus initThreadManager()
{
    memset(&ThreadManagerData, 0, sizeof(ThreadManagerData));
    pthread_mutex_init(&ThreadManagerData.mutex_manager, NULL);
    pthread_mutex_init(&ThreadManagerData.mutex_threads, NULL);
    return THREAD_STATUS_SUCCESS;
}

ThreadStatus createThread(void *thread_entry(void *), void *args)
{
    pthread_mutex_lock(&ThreadManagerData.mutex_manager);

    ThreadStatus status = THREAD_STATUS_SUCCESS;
    if (ThreadManagerData.threadCount > THREAD_MAX_NUMBER)
        status = THREAD_STATUS_TOO_MANY_THREADS;

    if (status == THREAD_STATUS_SUCCESS)
    {
        pthread_create(&ThreadManagerData.threadID[ThreadManagerData.threadCount], NULL, thread_entry, args);
        ThreadManagerData.threadCount++;
    }

    pthread_mutex_unlock(&ThreadManagerData.mutex_manager);

    return status;
}

void stopAllThreads()
{
    /* Set flags for all threads to terminate itself */
    setThreadsRunning(0);

    pthread_mutex_lock(&ThreadManagerData.mutex_manager);

    for (int i = 0; i < ThreadManagerData.threadCount; i++)
    {
        pthread_join(ThreadManagerData.threadID[i], NULL);
        ThreadManagerData.threadID[i] = 0;
    }

    ThreadManagerData.threadCount = 0;

    pthread_mutex_unlock(&ThreadManagerData.mutex_manager);
}

int getThreadsRunning()
{
    int threadsRunning = -1;
    pthread_mutex_lock(&ThreadManagerData.mutex_threads);
    threadsRunning = ThreadManagerData.ThreadsRunning;
    pthread_mutex_unlock(&ThreadManagerData.mutex_threads);
    return threadsRunning;
}

void setThreadsRunning(int running)
{
    pthread_mutex_lock(&ThreadManagerData.mutex_threads);
    ThreadManagerData.ThreadsRunning = running;
    pthread_mutex_unlock(&ThreadManagerData.mutex_threads);
}