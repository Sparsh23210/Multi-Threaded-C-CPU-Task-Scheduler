#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

#include "process.hpp"
#include "logger.hpp"
#include "metrics.hpp"

enum class SchedulerAlgorithm {
    FCFS,
    RR,
    PRIORITY_AGING,
    SRTF
};

struct SchedulerConfig {
    SchedulerAlgorithm algorithm = SchedulerAlgorithm::FCFS;
    unsigned int quantum = 2;              // For Round-Robin
    unsigned int agingInterval = 5;         // Ticks before priority boosts
    int agingBoost = 1;                     // Amount priority increases
    unsigned int contextSwitchCost = 0;     // Ticks penalty for switching tasks
    unsigned int tickDelayUs = 500;         // Delay in microseconds per tick
};

class CPUScheduler {
public:
    explicit CPUScheduler(const SchedulerConfig& config = SchedulerConfig{});
    ~CPUScheduler();

    // Prevent copying
    CPUScheduler(const CPUScheduler&) = delete;
    CPUScheduler& operator=(const CPUScheduler&) = delete;

    void addProcess(const ProcessPtr& process);
    void run();

    [[nodiscard]] const MetricsTracker& getMetricsTracker() const;
    [[nodiscard]] const std::vector<ProcessPtr>& getFinishedProcesses() const;

private:
    void workerRoutine();
    void handleBlockedProcesses();
    void applyAgingMechanism();
    ProcessPtr selectNextProcess();

    SchedulerConfig config_;
    MetricsTracker metricsTracker_;

    // Thread-safe queues
    std::deque<ProcessPtr> readyQueue_;
    std::deque<ProcessPtr> blockedQueue_;
    std::vector<ProcessPtr> finishedQueue_;

    mutable std::mutex readyMutex_;
    mutable std::mutex blockedMutex_;
    mutable std::mutex finishedMutex_;
    std::condition_variable cvReady_;

    std::atomic<bool> isRunning_{false};
    std::atomic<bool> isWorkerActive_{false};
    std::atomic<unsigned int> globalTime_{0};
    std::atomic<unsigned int> totalActiveCpuTime_{0};

    int lastExecutedPid_{-1};
    std::thread workerThread_;
};

#endif // SCHEDULER_HPP
