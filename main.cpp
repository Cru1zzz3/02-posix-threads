#include <pthread.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include "Thread.h"

int get_tid()
{
    static std::atomic_int counter = 0;
    thread_local std::unique_ptr<int> id = nullptr;
    if (id == nullptr)
    {
        id = std::make_unique<int>(++counter);
    }

    return *id;
}

void *producer_routine(void *args)
{
    auto producer = (Producer *)args;
    
    pthread_mutex_lock(producer->canWorkMutexPtr);
    while (!*producer->canWorkPtr)
        pthread_cond_wait(producer->canWorkCondPtr, producer->canWorkMutexPtr);
    pthread_mutex_unlock(producer->canWorkMutexPtr);

    std::string tmp;
    std::getline(std::cin, tmp);
    auto input = tmp.c_str();
    while (true)
    {
        char *end;
        auto now = std::strtol(input, &end, 10);
        if (input == end)
            break;

        input = end;
    
        pthread_mutex_lock(producer->consumedMutexPtr);
        while (!*producer->consumedPtr)
            pthread_cond_wait(producer->consumedCondPtr, producer->consumedMutexPtr);
        pthread_mutex_unlock(producer->consumedMutexPtr);

        (*producer->sharedVariablePtr) = now;
        (*producer->consumedPtr) = false;
        pthread_cond_signal(producer->consumedCondPtr);

        pthread_mutex_unlock(producer->consumedMutexPtr);
    }
    return nullptr;
}

void *consumer_routine(void *args)
{
    auto consumer = (Consumer *)args;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

    pthread_mutex_lock(consumer->threadsStartedMutexPtr);
    ++(*consumer->threadsStartedPtr);
    pthread_mutex_unlock(consumer->threadsStartedMutexPtr);
    
    pthread_mutex_lock(consumer->canWorkMutexPtr);
    while (!*consumer->canWorkPtr)
        pthread_cond_wait(consumer->canWorkCondPtr, consumer->canWorkMutexPtr);
    pthread_mutex_unlock(consumer->canWorkMutexPtr);

    long sum = 0;
    auto shouldBreak = false;
    while(true)
    {
        pthread_mutex_lock(consumer->consumedMutexPtr);
        while(true)
        {
            if (*consumer->consumed == false)
                break;

            pthread_mutex_lock(consumer->canWorkMutexPtr);
            if (*consumer->canWorkPtr == false)
            {
                pthread_mutex_unlock(consumer->canWorkMutexPtr);
                break;
            }
            pthread_mutex_unlock(consumer->canWorkMutexPtr);
            pthread_cond_wait(consumer->consumedCondPtr, consumer->consumedMutexPtr);
        }

        if (!*consumer->consumed)
        {
            sum += *consumer->variablePtr;
            (*consumer->consumed) = true;
        }

        if (consumer->isDebugEnabled)
        {
            std::cout << "(" << get_tid() << ", " << sum << ")" << std::endl;
        }

        pthread_cond_broadcast(consumer->consumedCondPtr);
        pthread_mutex_unlock(consumer->consumedMutexPtr);

        if (shouldBreak)
        {
            break;
        }

        if (consumer->sleep > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % consumer->sleep + 1));
    }

    pthread_exit((void *)sum);
}

void *consumer_interruptor_routine(void *args)
{
    auto interruptor = (Interruptor *)args;
    get_tid();
    
    pthread_mutex_lock(interruptor->canWorkMutexPtr);
    while (!*interruptor->canWorkPtr)
        pthread_cond_wait(interruptor->canWorkCondPtr, interruptor->canWorkMutexPtr);
    pthread_mutex_unlock(interruptor->canWorkMutexPtr);

    while (true)
    {
        pthread_cancel(interruptor->threads[std::rand() % interruptor->threadsCount]);

        pthread_mutex_lock(interruptor->canWorkMutexPtr);
        auto shouldBreak = !*interruptor->canWorkPtr;
        pthread_mutex_unlock(interruptor->canWorkMutexPtr);

        if (shouldBreak)
        {
            return nullptr;
        }
    }
}

void start_threads(Thread *params)
{
    pthread_mutex_lock(params->canWorkMutexPtr);
    *(params->canWorkPtr) = true;
    pthread_mutex_unlock(params->canWorkMutexPtr);
    pthread_cond_broadcast(params->canWorkCondPtr);
}

int run_threads(int count, long sleepMs, bool isDebugEnabled)
{
    pthread_t producer;

    pthread_t interruptor;
    auto consumers = new pthread_t[count];
    auto sharedVar = 0L;
    const int timeout = 300;
    auto consumed = true;
    pthread_cond_t consumedCondPtr;
    pthread_mutex_t consumedMutexPtr;
    Producer producerParams(
        timeout,
        &sharedVar,
        &consumed,
        &consumedCondPtr,
        &consumedMutexPtr);
    Interruptor interruptorParams(
        timeout,
        count,
        consumers);
    Consumer consumersParams(
        timeout,
        sleepMs,
        isDebugEnabled,
        &sharedVar,
        &consumed,
        &consumedCondPtr,
        &consumedMutexPtr);

    pthread_cond_init(&consumedCondPtr, NULL);
    pthread_mutex_init(&consumedMutexPtr, NULL);
    pthread_create(&producer, NULL, producer_routine, (void *)&producerParams);
    pthread_create(&interruptor, NULL, consumer_interruptor_routine, (void *)&interruptorParams);

    for (auto i = 0; i < count; ++i)
    {
        pthread_create(&consumers[i], NULL, consumer_routine, (void *)&consumersParams);
    }

    for (;;)
    {
        pthread_mutex_lock(consumersParams.threadsStartedMutexPtr);
        auto started = *consumersParams.threadsStartedPtr;
        pthread_mutex_unlock(consumersParams.threadsStartedMutexPtr);
        if (started == count)
        {
            break;
        }
    }

    start_threads(&interruptorParams);
    start_threads(&producerParams);
    start_threads(&consumersParams);

    pthread_join(producer, NULL);

    pthread_mutex_lock(interruptorParams.canWorkMutexPtr);
    *(interruptorParams.canWorkPtr) = false;
    pthread_mutex_unlock(interruptorParams.canWorkMutexPtr);

    pthread_join(interruptor, NULL);

    pthread_mutex_lock(consumersParams.canWorkMutexPtr);
    *(consumersParams.canWorkPtr) = false;
    pthread_mutex_unlock(consumersParams.canWorkMutexPtr);

    pthread_mutex_lock(consumersParams.consumedMutexPtr);
    while(!consumersParams.consumed)
        pthread_cond_wait(consumersParams.consumedCondPtr, consumersParams.consumedMutexPtr);
    pthread_mutex_unlock(consumersParams.consumedMutexPtr);

    pthread_cond_broadcast(consumersParams.consumedCondPtr);
    pthread_mutex_unlock(consumersParams.consumedMutexPtr);

    long totalSum = 0;
    long sum = 0;
    for (auto i = 0; i < count; ++i)
    {
        pthread_join(consumers[i], (void **)&sum);
        totalSum += sum;
    }

    delete[] consumers;

    return (int)totalSum;
}

int argToInt(char *argv)
{
    char *tmp;
    auto n = std::strtol(argv, &tmp, 10);

    return (int)n;
}

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4)
    {
        return 1;
    }

    auto n = argToInt(argv[1]);
    auto sleepMs = argToInt(argv[2]);
    bool isDebugEnabled = false;
    if (argc == 4)
    {
        if (std::strncmp("-debug", argv[3], std::strlen("-debug")) != 0)
        {
            return 1;
        }
        isDebugEnabled = true;
    }

    std::cout << run_threads(n, sleepMs, isDebugEnabled) << std::endl;
    return 0;
}