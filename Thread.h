#include <stdexcept>

class Thread
{
public:
    pthread_barrier_t *startBarrierPtr;
    bool *canWorkPtr;
    pthread_barrier_t *canWorkBarrierPtr;
    pthread_mutex_t *canWorkMutexPtr;

protected:
    Thread(int count)
    {
        canWorkPtr = new bool(true);
        canWorkBarrierPtr = new pthread_barrier_t();
        canWorkMutexPtr = new pthread_mutex_t();
        startBarrierPtr = new pthread_barrier_t();

        pthread_barrier_init(canWorkBarrierPtr, NULL, count + 1);
        pthread_mutex_init(canWorkMutexPtr, NULL);
        pthread_barrier_init(startBarrierPtr, NULL, count + 1);
    }

    ~Thread()
    {
        delete startBarrierPtr;
        delete canWorkMutexPtr;
        delete canWorkBarrierPtr;
        delete canWorkPtr;
    }
};

class Consumer : public Thread
{
public:
    const long sleep;
    const bool isDebugEnabled;
    const long *variablePtr;
    bool *consumed;
    pthread_cond_t *consumedCondPtr;
    pthread_mutex_t *consumedMutexPtr;

    Consumer(
        int count,
        long sleep,
        bool isDebugEnabled,
        const long *sharedVariablePtr,
        bool *consumed,
        pthread_cond_t *consumedCondPtr,
        pthread_mutex_t *consumedMutexPtr) : Thread(count),
                                             sleep(sleep),
                                             isDebugEnabled(isDebugEnabled),
                                             variablePtr(sharedVariablePtr),
                                             consumed(consumed),
                                             consumedCondPtr(consumedCondPtr),
                                             consumedMutexPtr(consumedMutexPtr)

    {
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
        int count,
        long *sharedVariablePtr,
        bool *consumedPtr,
        pthread_cond_t *consumedCondPtr,
        pthread_mutex_t *consumedMutexPtr) : Thread(count),
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
        int count,
        size_t threadsCount,
        const pthread_t *threads) : Thread(count),
                                    threadsCount(threadsCount),
                                    threads(threads)
    {
    }
};