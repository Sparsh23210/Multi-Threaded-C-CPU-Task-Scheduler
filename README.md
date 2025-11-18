# CPU Scheduling Simulator

This project is a **multithreaded CPU Scheduling Simulator** written in C++.  
It models two common scheduling algorithms — **First-Come, First-Served (FCFS)** and **Round Robin (RR)** — using **POSIX threads (pthreads)** for concurrent simulation of process execution and I/O blocking.

---

## Description

This program simulates CPU process scheduling using **multithreaded C++** with **POSIX threads**.  
It models how an operating system manages CPU and I/O bursts under two strategies — **First-Come, First-Served (FCFS)** and **Round Robin (RR)**.

Each process alternates between CPU and I/O bursts, moving through queues (ready, blocked, and finished) as time progresses.  
The simulator logs CPU executions, context switches, and turnaround/wait times, offering a clear visualization of how scheduling algorithms affect process performance.  

The program provides configurable runtime options, including the scheduling strategy and time quantum for RR, and uses mutex synchronization to ensure safe multithreaded operation.

---

## Features

- Supports **FCFS (First-Come, First-Served)** and **Round Robin (RR)** scheduling.
- Configurable **time quantum** for Round Robin.
- Accurate logging of:
  - CPU and IO bursts per process.
  - Elapsed simulation time.
  - Process turnaround and wait times.
- Thread-safe shared state management with `pthread_mutex_t`.
- Graceful error handling for malformed input.

---

## Structure

| File | Description |
|------|--------------|
| `main.cpp` | Entry point. Parses command-line arguments, loads bursts file, initializes scheduler threads. |
| `schedule.cpp` | Implements FCFS and RR worker threads, simulating CPU and IO execution. |
| `schedule.h` | Defines `Process`, `SharedState`, and scheduler function prototypes. |
| `log.cpp` | Handles standardized logging for execution and completion events. |
| `log.h` | Declares logging utilities and enums for execution stop reasons. |
| `bursts_rr_3.txt` | Sample input file containing process burst definitions. |
| `Makefile` | Automates the build process. |

---

## Input Format

Each line in the input file represents one process.

- Each number represents a burst duration (CPU → IO → CPU → ...).
- Each process must have an **odd number** of bursts (always ending with a CPU burst).

### Example: `bursts_rr_3.txt`

4 4 2
1 7 3
3 2 4


**Interpretation:**
- Process 0: CPU 4 → IO 4 → CPU 2  
- Process 1: CPU 1 → IO 7 → CPU 3  
- Process 2: CPU 3 → IO 2 → CPU 4  

---

## Compilation

To build the program:

```bash
make
