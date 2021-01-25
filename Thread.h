#include <stdexcept>

class Thread
{
public:
    int mutexLockTimeout;
    bool *canWorkPtr;
    pthread_cond_t *canWorkCondPtr;
    pthread_mutex_t *canWorkMutexPtr;

protected:
    Thread(const int mutexLockTimeout) : mutexLockTimeout(mutexLockTimeout)
    {
        canWorkPtr = new bool(false);
        canWorkCondPtr = new pthread_cond_t();
        canWorkMutexPtr = new pthread_mutex_t();

        pthread_cond_init(canWorkCondPtr, NULL);
        pthread_mutex_init(canWorkMutexPtr, NULL);
    }

    ~Thread()
    {
        delete canWorkMutexPtr;
        delete canWorkCondPtr;
        delete canWorkPtr;
    }
};

class Consumer : public Thread
{
public:
    const long sleep;
    const bool isDebugEnabled;
    pthread_mutex_t *threadsStartedMutexPtr;
    int *threadsStartedPtr;
    const long *variablePtr;
    bool *consumed;
    pthread_cond_t *consumedCondPtr;
    pthread_mutex_t *consumedMutexPtr;

    Consumer(
        const int mutexLockTimeout,
        long sleepMilliseconds,
        bool isDebugEnabled,
        const long *sharedVariablePtr,
        bool *consumed,
        pthread_cond_t *consumedCondPtr,
        pthread_mutex_t *consumedMutexPtr) : Thread(mutexLockTimeout),
                                             sleep(sleepMilliseconds),
                                             isDebugEnabled(isDebugEnabled),
                                             variablePtr(sharedVariablePtr),
                                             consumed(consumed),
                                             consumedCondPtr(consumedCondPtr),
                                             consumedMutexPtr(consumedMutexPtr)

    {
        threadsStartedMutexPtr = new pthread_mutex_t();
        threadsStartedPtr = new int(0);

        pthread_mutex_init(threadsStartedMutexPtr, NULL);
    }

    ~Consumer()
    {
        delete threadsStartedPtr;
        delete threadsStartedMutexPtr;
    }
};

class Producer : public Thread
{
public:
    long *sharedVariablePtr;
    bool *consumedPtr;
    pthread_cond_t *consumedCondPtr;
    pthread_mutex_t *consumedMutexPtr;

    Producer(
        const int mutexLockTimeout,
        long *sharedVariablePtr,
        bool *consumedPtr,
        pthread_cond_t *consumedCondPtr,
        pthread_mutex_t *consumedMutexPtr) : Thread(mutexLockTimeout),
                                             sharedVariablePtr(sharedVariablePtr),
                                             consumedPtr(consumedPtr),
                                             consumedCondPtr(consumedCondPtr),
                                             consumedMutexPtr(consumedMutexPtr)
    {
    }
};

class Interruptor : public Thread
{
public:
    const size_t threadsCount;
    const pthread_t *threads;

    Interruptor(
        const int mutexLockTimeout,
        size_t threadsCount,
        const pthread_t *threads) : Thread(mutexLockTimeout),
                                    threadsCount(threadsCount),
                                    threads(threads)
    {
    }
};