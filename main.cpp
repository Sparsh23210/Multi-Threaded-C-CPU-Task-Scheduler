
#include <unistd.h>     // POSIX / sleep
#include <pthread.h>    // pthreads
#include <fstream>      // File I/O
#include <sstream>      // string stream
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <queue>
#include "log.h"
#include "schedule.h"
#define NORMAL_EXIT 0

using namespace std;

void handle_blocked(SharedState* state) {
    pthread_mutex_lock(&state->blockedMutex);

    // temp queue to preserve order
    queue<Process*> tmp;
    
    while(!state->blocked.empty()) {
        Process* p = state->blocked.front();
        state->blocked.pop();

        if(p->blockedEntryTime == state->globalTime) {
            tmp.push(p);
            continue;
        }

        if (p->remainingIO <= 1) {
            p->busyTime += p->remainingIO;
            p->remainingIO = 0;

            pthread_mutex_lock(&state->readyMutex);
            state->ready.push(p);
            pthread_mutex_unlock(&state->readyMutex);

        } else {
            p->remainingIO -= 1;
            p->busyTime += 1;

            tmp.push(p);
        }
    }

    state->blocked = tmp;

    pthread_mutex_unlock(&state->blockedMutex);
}

int main (int argc, char** argv) {
    // Set defaults
    string filename;
    int quantum = 0; // default 2
    Strategy strat;
    SharedState state;
    pthread_t worker;

    /*
     * Parse options with getopt
     * -s <strategy> (fcfs || rr), default fcfs
     * -q <quantum>, default 2
     */
    int option;
    while ((option = getopt(argc, argv, "s:q:")) != -1) {
        switch (option) {
        case 's':
            if (string(optarg) == "rr" || string(optarg) == "RR")
                strat = Strategy::RR;
            else if (string(optarg) == "fcfs" || string(optarg) == "FCFS")
                strat = Strategy::FCFS;
            else {
                cerr << "I've never heard of the strategy: " << optarg << endl;
                return 1;
            }
            break;

        case 'q':
            try {
                quantum = stoi(optarg);
                try {
                    if (quantum > 0) {
                        state.quantum = quantum;
                    } else {
                        throw runtime_error("Time quantum must be a number and bigger than 0\n");
                    }
                } catch(const exception& e) {
                    fprintf(stderr, "Time quantum must be a number and bigger than 0\n");
                    exit(NORMAL_EXIT);
                }
            } catch (exception& e) {
                cerr << "Nah, can't accept that quantum... call it quantumania. Q: " << optarg << endl;
                return 1; 
            }
            break;

        default:
            cerr << "Your usage is wrong... Kinda makes you want to go back and try again." << endl;
            return 1;
        }
    }

    int idx = optind;
    if(idx < argc) {
        //process positional arguments
        filename = argv[idx];
    } else {
        cerr << "Where's your file name at bud?" << endl;
        return 1;
    }

    /*
     * Reads the burst file
     * 1. Each line -> one Process
     * 2. Split into integers (bursts)
     * 3. Store burst into struct
     * 4. Put processes into ready queue
     */
    ifstream fin(filename);
    // if file is not open, escape
    if(!fin.is_open()) {
        printf("Unable to open <<%s>>", filename.c_str());
        exit(NORMAL_EXIT);
    }    
   
    string line;
    unsigned int idCounter = 0;
    while(getline(fin, line)) {
        // skip empty or whitespace
        if(line.find_first_not_of(" \t\r\n") == string::npos)
            continue;

        Process* p = new Process;
        p->id = idCounter++;
         
        try {
            istringstream iss(line);
            int val;

            while (iss >> val) {
                try {
                    if (val <= 0) {
                        throw runtime_error("A burst number must be bigger than 0\n");
                        p->bursts.push_back(val);
                    } else {
                        p->bursts.push_back(val);
                    }
                } catch(const exception& e) {
                    fprintf(stderr, "A burst number must be bigger than 0\n");
                    exit(NORMAL_EXIT);
                }
            }

            // make sure count is odd
            if(p->bursts.size() % 2 == 0) {
                throw runtime_error("Error: line must contain an odd number");
            }

            pthread_mutex_lock(&state.readyMutex);
            state.ready.push(p);
            pthread_mutex_unlock(&state.readyMutex);

            // convert deque to vector and log
            vector<unsigned int> temp(p->bursts.begin(), p->bursts.end());
            log_process_bursts(temp.data(), temp.size());

        } catch (const exception &e) {
            printf("There must be an odd number of bursts for each process");
            delete p;
        }
    }
    
    /*
     * Sets up processes and scheduler
     */
    state.running = true;
    if (strat == Strategy::FCFS) {
        pthread_create(&worker, nullptr, &fcfs_worker, &state);
    } else if(strat == Strategy::RR) {
        pthread_create(&worker, nullptr, &rr_worker, &state);
    } else {
        pthread_create(&worker, nullptr, &fcfs_worker, &state);
    }

    while(state.running) {
        handle_blocked(&state);
        usleep(1000); // small sleep to avoid busy waiting

        // EXIT CONDITIONS:
        pthread_mutex_lock(&state.readyMutex);
        bool allDone = state.ready.empty();
        pthread_mutex_unlock(&state.readyMutex);

        pthread_mutex_lock(&state.blockedMutex);
        allDone = allDone && state.blocked.empty();
        pthread_mutex_unlock(&state.blockedMutex);

        if(allDone && !state.waiting) {
            state.running = false;
        }
    }

    pthread_mutex_lock(&state.finishedMutex);
    while(!state.finished.empty()) {
        Process* p = state.finished.front();
        state.finished.pop();
        log_process_completion(p->id, p->turnaround, p->waitTime);
        delete p; // memory cleanup
    }
    pthread_mutex_unlock(&state.finishedMutex);

    return NORMAL_EXIT;
}