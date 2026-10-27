# Multi-Threaded C++ CPU & Task Scheduler

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build](https://img.shields.io/badge/Build-CMake%20%7C%20Make-brightgreen.svg)]()
[![Concurrency](https://img.shields.io/badge/Concurrency-Threads%20%26%20Mutexes-orange.svg)]()
[![Standard](https://img.shields.io/badge/Standard-POSIX%20%26%20Windows-lightgrey.svg)]()

A high-performance, multithreaded CPU and process scheduling simulator built in modern **C++17**. The simulator models real-world operating system task execution across four distinct scheduling strategies, featuring dynamic process aging to prevent starvation, thread-safe synchronization queues, real-time operating system metrics (CPU utilization %, throughput, context-switch latency), and cross-platform build infrastructure.

---

## Key Architectural & Engineering Highlights

### 1. Modern C++17 Architecture & RAII
- **Standard Threading Model:** Refactored legacy C POSIX `<pthread.h>` constructs to native C++17 `<thread>`, `<mutex>`, `<condition_variable>`, and `<atomic>`.
- **Thread Synchronization & Safety:** Replaced manual lock/unlock sequences with RAII-compliant `std::lock_guard` and `std::unique_lock`, eliminating race conditions and deadlocks across concurrent ready, blocked, and execution queues.
- **Smart Pointer Memory Management:** Enforced clean lifecycle management using `std::shared_ptr<Process>` and STL containers (`std::deque`, `std::vector`), eliminating memory leaks and raw pointer errors.

### 2. Advanced Scheduling Algorithms
- **First-Come, First-Served (FCFS):** Non-preemptive scheduling executing task bursts in order of arrival.
- **Round Robin (RR):** Preemptive scheduling using a configurable time quantum, ensuring fair CPU slice distribution.
- **Priority Scheduling with Starvation Prevention (Aging):** Dynamic priority calculation where prolonged time in the ready queue automatically increments task priority by an aging boost factor, preventing lower-priority thread starvation.
- **Shortest Remaining Time First (SRTF):** Preemptive Shortest Job First that dynamically preempts currently executing tasks when a shorter CPU burst arrives or unblocks from I/O.

### 3. Real-Time Performance Analytics Engine
Calculates production-grade OS scheduling metrics:
- **CPU Utilization %:** Active computation time vs. idle and context-switch overhead time.
- **Throughput:** Completed processes per unit time tick.
- **Context-Switch Overhead:** Granular tracking of context-switch frequency and configurable hardware save/restore state delay costs.
- **Latency Metrics:** Comprehensive tracking of Turnaround Time, Waiting Time, and Initial Response Time (Average, Minimum, Maximum).

### 4. Interactive CLI & Flexible Input
- Interactive process burst creation directly via terminal.
- Support for static configuration burst files (`CPU -> IO -> CPU ...`).
- Fast benchmarking mode (`--fast`) or variable-speed tick delay simulation (`--delay <us>`).

---

## Repository Structure

```text
multi-threaded-scheduling/
├── CMakeLists.txt              # Cross-platform CMake build configuration
├── Makefile                    # Makefile supporting Linux, macOS, and MinGW
├── .gitignore                  # Git ignore rules for build artifacts
├── README.md                   # Project documentation and performance analysis
├── bursts_rr_3.txt             # Benchmark workload configuration
├── include/
│   ├── process.hpp             # Process definition, burst tracking & smart pointers
│   ├── scheduler.hpp           # CPUScheduler engine, worker thread & synchronization
│   ├── metrics.hpp             # Real-time metrics engine (CPU %, throughput, switch cost)
│   └── logger.hpp              # Thread-safe console and event logger
├── src/
│   ├── main.cpp                # CLI parser, interactive mode & orchestration
│   ├── scheduler.cpp           # Scheduling algorithms & I/O queue management
│   ├── metrics.cpp             # Statistical analytics & dashboard reporting
│   └── logger.cpp              # Formatted event logging implementation
└── tests/
    ├── sample_bursts.txt       # Standard FCFS / RR test workloads
    ├── bursts_rr_3.txt         # Multi-burst benchmark case
    └── priority_bursts.txt     # Priority aging test cases
```

---

## Build & Installation

### Option A: Using CMake (Recommended)
```bash
# Create build directory
mkdir build && cd build

# Configure and compile
cmake ..
cmake --build .
```

### Option B: Using Make
```bash
# Build binary
make

# Clean build artifacts
make clean
```
*(On Windows with MinGW, run `mingw32-make`)*

---

## Usage & CLI Reference

```text
Usage: scheduler [OPTIONS] [bursts_file]

Options:
  -s, --algo <fcfs|rr|priority|srtf>   Scheduling algorithm (default: fcfs)
  -q, --quantum <int>                  Time quantum for Round Robin / Priority (default: 2)
  --aging-interval <int>               Ticks before priority is boosted (default: 5)
  --aging-boost <int>                  Amount to increment priority by (default: 1)
  --cs-cost <int>                      Context switch overhead ticks (default: 0)
  --delay <microseconds>               Simulation tick delay (default: 500us)
  --fast                               Run simulation without delays (benchmarking)
  -i, --interactive                    Interactive CLI process creator mode
  -h, --help                           Show help menu
```

---

## Execution Examples & Output

### 1. Round Robin Execution
```bash
./scheduler -s rr -q 2 tests/bursts_rr_3.txt --fast
```

**Console Output:**
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
```

### 2. Comprehensive Analytics Dashboard
```text
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

### 3. Priority Aging Starvation Prevention
```bash
./scheduler -s priority --aging-interval 2 --aging-boost 1 tests/priority_bursts.txt --fast
```
```text
[AGING] T=2 | Process P0 aged: priority boosted from 10 to 11
[AGING] T=2 | Process P1 aged: priority boosted from 12 to 13
[AGING] T=4 | Process P0 aged: priority boosted from 11 to 12
```

---

## Algorithm Performance Comparison

| Metric | FCFS | Round Robin (q=2) | Priority + Aging | SRTF |
| :--- | :---: | :---: | :---: | :---: |
| **Average Turnaround Time** | 13.67 | 14.00 | 26.50 | 14.33 |
| **Average Wait Time** | 3.67 | 4.00 | 16.50 | 4.33 |
| **Average Response Time** | 3.00 | **1.67** | 7.50 | **1.67** |
| **Context Switches** | **5** | 8 | 8 | **5** |
| **Starvation Prevention** | None | Fair Slice | Dynamic Aging | Preemption |

---

## Resume Bullet Points

You can showcase this project on your resume with the following bullet points:

- **Multi-Threaded C++ CPU & Task Scheduler** | *C++17, Multithreading, STL, Mutexes, CMake*
  - Developed a thread-safe CPU scheduling simulator in modern C++17 implementing **Round Robin**, **SRTF**, **FCFS**, and **Priority Scheduling** with dynamic aging to prevent thread starvation.
  - Architected asynchronous synchronization between CPU workers and I/O queues using `std::mutex`, `std::unique_lock`, and `std::condition_variable`, eliminating deadlocks and race conditions.
  - Implemented memory safety and RAII using `std::shared_ptr` smart pointers and STL containers, replacing legacy C-style pointer allocations and raw mutex calls.
  - Engineered a real-time metrics tracking engine to quantify system performance, measuring **CPU utilization %**, **throughput**, **context-switch latency overhead**, and turnaround/response times.
  - Configured cross-platform build infrastructure using **CMake** and **Make** with strict compiler flags (`-Wall -Wextra -Wpedantic`).
