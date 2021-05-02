
#include <string.h>
#include <debug.h>

#include "threading.h"

static ThreadSharedData ThreadManagerData;

/* Only call this function once from main-thread */
void ThreadManagerInit(void)
{
    memset(&ThreadManagerData, 0, sizeof(ThreadManagerData));
    pthread_mutex_init(&ThreadManagerData.mutex_manager, NULL);
    pthread_mutex_init(&ThreadManagerData.mutex_threads, NULL);
}

int ThreadManagerCreate(void *thread_entry(void *), void *args)
{
    int retval = 0;

    pthread_mutex_lock(&ThreadManagerData.mutex_manager);

    if (ThreadManagerData.threadCount > THREAD_MAX_NUMBER)
    {
        debug(0, "Error: Could not create new thread - max number of threads reached(%d)\n", THREAD_MAX_NUMBER);
        retval = -1;
    }

    if (retval == 0)
    {
        pthread_create(&ThreadManagerData.threadID[ThreadManagerData.threadCount], NULL, thread_entry, args);
        ThreadManagerData.threadCount++;
    }
    pthread_mutex_unlock(&ThreadManagerData.mutex_manager);
    return retval;
}

void ThreadManagerSetRunnable(void)
{
    ThreadSharedDataRunninSet(1);
}

void ThreadManagerStopAll(void)
{
    printf("Stopping threads\n");

    /* Set flags for all threads to terminate itself */
    ThreadSharedDataRunninSet(0);

    pthread_mutex_lock(&ThreadManagerData.mutex_manager);

    for (int i = 0; i < ThreadManagerData.threadCount; i++)
    {
        pthread_join(ThreadManagerData.threadID[i], NULL);
        ThreadManagerData.threadID[i] = 0;
    }

    ThreadManagerData.threadCount = 0;

    pthread_mutex_unlock(&ThreadManagerData.mutex_manager);
}

int ThreadSharedDataRunninGet(void)
{
    int retval;
    pthread_mutex_lock(&ThreadManagerData.mutex_threads);
    retval = ThreadManagerData.ThreadsRunning;
    pthread_mutex_unlock(&ThreadManagerData.mutex_threads);

    return retval;
}

void ThreadSharedDataRunninSet(int Running)
{
    pthread_mutex_lock(&ThreadManagerData.mutex_threads);
    ThreadManagerData.ThreadsRunning = Running;
    pthread_mutex_unlock(&ThreadManagerData.mutex_threads);
}