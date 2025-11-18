#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <vector>
#include <deque>
#include <queue>
#include <pthread.h>

using namespace std;

enum class Strategy { FCFS, RR };

struct Process {
    // Using deque for RR, makes pushing front for incomplete tasks easier
    deque<unsigned int> bursts; // CPU -> IO -> CPU (odd length)

    int id;
    unsigned int blockedEntryTime = 0;
    unsigned int expectedIO = 0;
    unsigned int expectedCPU = 0;
    unsigned int remainingIO = 0;
    unsigned int busyTime; // sum CPU + sum IO
    unsigned int turnaround; // timestamp
    unsigned int waitTime;   // turnaround - (sum CPU + sum IO)
};

struct SharedState {
    queue<Process*> ready;
    queue<Process*> blocked;
    queue<Process*> finished;

    bool waiting = false; // true if worker thread is working
    bool running = true; // global flag

    pthread_mutex_t readyMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t blockedMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t finishedMutex = PTHREAD_MUTEX_INITIALIZER;

    Strategy strat = Strategy::FCFS; // set to FCFS for default
    int pid;

    unsigned int elapsed = 0;
    unsigned int globalTime = 0;
    unsigned int quantum = 2;
};

void* fcfs_worker(void* arg);
void* rr_worker(void* arg);

void calculate_wait(unsigned int clock, Process& p);
void advance_blocked(int cpu_time);
void drain_blocked();
void move_to_blocked(Process& p);
void move_to_ready(Process& p);
void finish_process(Process& p);

#endif