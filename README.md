# Multi-Threaded C++ CPU & Task Scheduler

A multi-threaded CPU scheduling simulator written in modern C++17. It models how an operating system kernel dispatches processes, manages I/O blocking, and balances workloads across four classic scheduling strategies.

The project uses native C++ threads (`std::thread`), mutexes (`std::mutex`), and condition variables (`std::condition_variable`) to coordinate execution between a simulated CPU worker and an asynchronous I/O manager.

---

## What Problem This Solves

Operating systems must constantly decide which process gets CPU time, when to pause a process waiting on disk/network I/O, and how to avoid letting low-priority tasks wait forever. 

This simulator models that lifecycle:
1. Processes alternate between **CPU bursts** and **I/O bursts**.
2. When a process finishes its CPU burst, it moves to a **Blocked Queue** to simulate I/O delay.
3. While I/O is running in the background, the CPU continues running other ready tasks.
4. Once I/O finishes, the process returns to the **Ready Queue** to compete for the CPU again.

---

## Implemented Algorithms

| Algorithm | Flag | Preemptive? | Notes |
| :--- | :--- | :---: | :--- |
| **First-Come, First-Served (FCFS)** | `-s fcfs` | No | Runs each CPU burst to completion in arrival order. |
| **Round Robin (RR)** | `-s rr -q <n>` | Yes | Slices CPU time into fixed quantums. Prevents CPU hogging. |
| **Priority with Aging** | `-s priority` | Yes | Higher priority runs first. Tasks waiting too long in the ready queue receive dynamic priority boosts to prevent starvation. |
| **Shortest Remaining Time First (SRTF)** | `-s srtf` | Yes | Preempts the running task if an unblocked or newly arrived task has a shorter remaining burst. |

### How Aging Prevents Starvation
In static priority scheduling, low-priority tasks can get starved if higher-priority tasks keep showing up. 

With aging enabled:
- If a process sits in the ready queue for more than `--aging-interval` ticks, its priority increases by `--aging-boost`.
- Over time, even low-priority jobs gain enough priority to run.

---

## Engineering & Concurrency Design

- **Thread Decoupling:** One thread acts as the CPU execution core while the main thread manages I/O timers and queue transitions.
- **RAII-Protected State:** Shared queues (`ready`, `blocked`, `finished`) are protected with `std::lock_guard` and `std::unique_lock`. Locks are automatically acquired and released at scope boundaries.
- **Signaling over Busy-Waiting:** When the ready queue is empty, the worker thread sleeps on a `std::condition_variable` rather than spinning in a busy loop. It wakes up immediately when the I/O thread signals that a process is ready.
- **Memory Safety:** Processes are passed across queues via `std::shared_ptr<Process>`, preventing memory leaks or dangling pointers.

---

## Performance Metrics

At the end of each run, the simulator generates a performance report:
- **CPU Utilization %:** Active execution time vs. total elapsed time (including idle time and context switches).
- **Throughput:** Number of completed processes per simulation tick.
- **Context Switch Overhead:** Number of task switches and the simulated latency penalty added per switch (`--cs-cost`).
- **Turnaround & Wait Times:** Average, min, and max times spent in the system and waiting in the ready queue.
- **Response Time:** Time from process arrival to its very first tick on the CPU.

---

## Project Structure

```text
├── CMakeLists.txt              # CMake build configuration
├── Makefile                    # Makefile (supports Windows MinGW and Linux/macOS)
├── .gitignore
├── README.md
├── bursts_rr_3.txt             # Sample workload file
├── include/
│   ├── process.hpp             # Process definition & burst tracking
│   ├── scheduler.hpp           # CPUScheduler engine & queue management
│   ├── metrics.hpp             # Performance tracker & report generator
│   └── logger.hpp              # Thread-safe event logging
├── src/
│   ├── main.cpp                # CLI parser & entry point
│   ├── scheduler.cpp           # Scheduling logic & worker thread
│   ├── metrics.cpp             # System metrics calculation
│   └── logger.cpp              # Formatted log printing
└── tests/
    ├── sample_bursts.txt       # Standard bursts
    ├── bursts_rr_3.txt         # Multi-process benchmark
    └── priority_bursts.txt     # Priority aging test cases
```

---

## Building the Project

### Using CMake
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Using Make
```bash
make
```
*(On Windows with MinGW, run `mingw32-make`)*

---

## How to Run

### Command-Line Options
```text
Usage: scheduler [OPTIONS] [bursts_file]

Options:
  -s, --algo <fcfs|rr|priority|srtf>   Scheduling algorithm (default: fcfs)
  -q, --quantum <int>                  Time quantum for Round Robin (default: 2)
  --aging-interval <int>               Ticks before priority is boosted (default: 5)
  --aging-boost <int>                  Priority increase per interval (default: 1)
  --cs-cost <int>                      Context switch delay penalty in ticks (default: 0)
  --delay <microseconds>               Real-time delay per tick (default: 500us)
  --fast                               Run simulation without delays (instant benchmark)
  -i, --interactive                    Interactive mode to enter tasks live from terminal
  -h, --help                           Show help menu
```

### Examples

**1. Round Robin with time quantum = 2:**
```bash
./scheduler -s rr -q 2 --fast bursts_rr_3.txt
```

**2. Priority scheduling with aging (boost every 2 ticks):**
```bash
./scheduler -s priority --aging-interval 2 --aging-boost 1 --fast tests/priority_bursts.txt
```

**3. Shortest Remaining Time First:**
```bash
./scheduler -s srtf --fast bursts_rr_3.txt
```

**4. Simulate hardware context-switch latency (1 tick per switch):**
```bash
./scheduler -s rr -q 2 --cs-cost 1 --fast bursts_rr_3.txt
```

**5. Interactive mode (type bursts directly in terminal):**
```bash
./scheduler -i -s rr -q 3
```

---

## Sample Run & Output

```text
4 4 2 
1 7 3 
3 2 4 
P0: executed cpu bursts = 2, executed io bursts = 0, time elapsed = 2, quantum expired
P1: executed cpu bursts = 1, executed io bursts = 7, time elapsed = 3, enter io
P2: executed cpu bursts = 2, executed io bursts = 0, time elapsed = 5, quantum expired
P0: executed cpu bursts = 4, executed io bursts = 4, time elapsed = 7, enter io
P2: executed cpu bursts = 3, executed io bursts = 2, time elapsed = 8, enter io
P1: executed cpu bursts = 3, executed io bursts = 7, time elapsed = 10, quantum expired
P0: executed cpu bursts = 6, executed io bursts = 4, time elapsed = 12, completed
P1: executed cpu bursts = 4, executed io bursts = 7, time elapsed = 13, completed
P2: executed cpu bursts = 5, executed io bursts = 2, time elapsed = 15, quantum expired
P2: executed cpu bursts = 7, executed io bursts = 2, time elapsed = 17, completed
P0: turnaround time = 12, wait time = 2
P1: turnaround time = 13, wait time = 2
P2: turnaround time = 17, wait time = 8

================================================================
               SCHEDULER PERFORMANCE REPORT                    
================================================================
 Algorithm Used             : Round Robin (RR)
 Total Processes Completed  : 3
 Total Simulation Time      : 17 ticks
 Total Active CPU Time      : 17 ticks
 Total Idle CPU Time        : 0 ticks
 CPU Utilization            : 100.00%
 Throughput                 : 0.1765 processes/tick
 Total Context Switches     : 8
 Context Switch Cost/Unit   : 0 ticks
 Total Context Switch Delay : 0 ticks
----------------------------------------------------------------
 METRIC                AVG        MIN        MAX
----------------------------------------------------------------
 Turnaround Time          14.00         12         17
 Waiting Time              4.00          2          8
 Response Time             1.67          -          -
================================================================
```

---

## Resume Description

If listing this project on a resume or LinkedIn, here is a concise description:

- **Multi-Threaded CPU Scheduling Simulator** | *C++17, Multithreading, STL, Mutexes, CMake*
  - Developed a thread-safe CPU scheduler in C++17 simulating Round Robin, FCFS, SRTF, and Priority Scheduling with dynamic aging to prevent process starvation.
  - Decoupled CPU execution and I/O wait cycles across concurrent threads synchronized via `std::mutex`, `std::unique_lock`, and `std::condition_variable`.
  - Implemented safe resource management using `std::shared_ptr` and standard containers, replacing legacy C-style pointer allocations and raw mutex calls.
  - Built a performance analytics engine calculating CPU utilization percentage, throughput, context-switch latency overhead, and turnaround/response times.
