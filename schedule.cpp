
#include <pthread.h>
#include <unistd.h>
#include "schedule.h"
#include "log.h"

/*
* NOTES:
* - mutex blocks main from accessing
* - C++ queue: pop does not return front
* - 
*  
*/

void* fcfs_worker(void* arg) {
    SharedState* state = (SharedState*)arg;

    while(state->running) {
        pthread_mutex_lock(&state->readyMutex);

        // check to make sure ready has processes, if not wait
        if(state->ready.empty()) {
            pthread_mutex_unlock(&state->readyMutex);
            usleep(1000); // 1ms delay
            continue;
        }

        // retrieve current process
        Process* p = state->ready.front();
        state->ready.pop();
        pthread_mutex_unlock(&state->readyMutex);

        state->waiting = true;

        // retrieve cpu burst
        unsigned int cpuBurst = p->bursts.front();
        p->bursts.pop_front();

        // per tick simulation
        for (unsigned int i = 0; i < cpuBurst; i++) {
            usleep(1000);
            state->globalTime += 1;
            p->busyTime += 1;
            p->expectedCPU +=1;
            state->elapsed += 1;
        }

        // check if more bursts to sim
        if(p->bursts.empty()) {
            // log end
            p->turnaround = state->globalTime;
            p->waitTime = p->turnaround - p->busyTime;

            pthread_mutex_lock(&state->finishedMutex);
            state->finished.push(p);
            pthread_mutex_unlock(&state->finishedMutex);

            log_cpuburst_execution(p->id, p->expectedCPU, p->expectedIO, state->globalTime, COMPLETED);
        } else {
            unsigned int ioBurst = p->bursts.front();
            p->bursts.pop_front();
            p->remainingIO = ioBurst;
            p->blockedEntryTime = state->globalTime;

            pthread_mutex_lock(&state->blockedMutex);
            state->blocked.push(p);
            pthread_mutex_unlock(&state->blockedMutex);

            log_cpuburst_execution(p->id, p->expectedCPU, p->expectedIO, state->globalTime, ENTER_IO);
            p->expectedIO += ioBurst;
        }
        state->waiting = false;
    }
    return nullptr;
}

void* rr_worker(void* arg) {
    SharedState* state = (SharedState*)arg;

    while(state->running) {
        pthread_mutex_lock(&state->readyMutex);

        if(state->ready.empty()) {
            pthread_mutex_unlock(&state->readyMutex);
            usleep(1000); // 1 ms delay
            continue;
        }

        Process* p = state->ready.front();
        state->ready.pop();
        pthread_mutex_unlock(&state->readyMutex);

        state->waiting = true;

        unsigned int q = state->quantum;
        unsigned int cpuBurst = p->bursts.front();
        // p->bursts.pop_front();

        // loop through ticks until quantum or cpu expires
        unsigned int ticks = 0;
        while(ticks < q && cpuBurst > 0) {
            usleep(1000);
            state->globalTime +=1;
            state->elapsed += 1;
            p->expectedCPU += 1;
            p->busyTime += 1;
            cpuBurst--;
            ticks++;
        }

        if(cpuBurst == 0) {
            p->bursts.pop_front();

            if(!p->bursts.empty()) {
                unsigned int ioBurst = p->bursts.front();
                p->bursts.pop_front();
                p->remainingIO = ioBurst;
                p->blockedEntryTime = state->globalTime;

                pthread_mutex_lock(&state->blockedMutex);
                state->blocked.push(p);
                pthread_mutex_unlock(&state->blockedMutex);

                p->expectedIO += ioBurst;

                log_cpuburst_execution(p->id, p->expectedCPU, p->expectedIO, state->globalTime, ENTER_IO);
            } else {
                p->turnaround = state->globalTime;
                p->waitTime = p->turnaround - p->busyTime;

                pthread_mutex_lock(&state->finishedMutex);
                state->finished.push(p);
                pthread_mutex_unlock(&state->finishedMutex);

                log_cpuburst_execution(p->id, p->expectedCPU, p->expectedIO, state->globalTime, COMPLETED);
            }
        } else {
            // quantum expired - push back with remaining cpuBurst
            p->bursts.front() = cpuBurst;

            pthread_mutex_lock(&state->readyMutex);
            state->ready.push(p);
            pthread_mutex_unlock(&state->readyMutex);

            log_cpuburst_execution(p->id, p->expectedCPU, p->expectedIO, state->globalTime, QUANTUM_EXPIRED);
        }

        state->waiting = false;
    }
    return nullptr;
}