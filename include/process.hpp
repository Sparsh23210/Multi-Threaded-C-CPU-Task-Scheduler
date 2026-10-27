#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <deque>
#include <string>
#include <memory>
#include <vector>

/**
 * @brief Represents a process with alternating CPU and IO bursts.
 */
struct Process {
    unsigned int id = 0;
    std::deque<unsigned int> bursts; // Sequence of CPU -> IO -> CPU bursts

    // Priority scheduling attributes
    int basePriority = 10;
    int currentPriority = 10; // Modified dynamically by aging mechanism

    // Timing and tracking metrics
    unsigned int arrivalTime = 0;
    unsigned int readyQueueEntryTime = 0;
    unsigned int blockedEntryTime = 0;

    unsigned int totalCpuExecuted = 0;
    unsigned int totalIoExecuted = 0;
    unsigned int totalWaitTime = 0;
    unsigned int turnaroundTime = 0;
    unsigned int responseTime = 0;

    bool hasStarted = false;
    unsigned int firstCpuTime = 0;

    // Current remaining time for the current burst
    unsigned int remainingCpuBurst = 0;
    unsigned int remainingIoBurst = 0;

    Process() = default;

    Process(unsigned int pid, const std::vector<unsigned int>& burstList, int priority = 10)
        : id(pid), bursts(burstList.begin(), burstList.end()), basePriority(priority), currentPriority(priority) {
        if (!bursts.empty()) {
            remainingCpuBurst = bursts.front();
        }
    }

    [[nodiscard]] unsigned int totalRemainingBurstTime() const {
        unsigned int total = 0;
        for (const auto& b : bursts) {
            total += b;
        }
        return total;
    }
};

using ProcessPtr = std::shared_ptr<Process>;

#endif // PROCESS_HPP
