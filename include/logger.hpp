#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include "process.hpp"

enum class ExecutionStopReason {
    ENTER_IO,
    QUANTUM_EXPIRED,
    PREEMPTED_SRTF,
    COMPLETED
};

class Logger {
public:
    static Logger& instance();

    void logProcessBursts(const std::vector<unsigned int>& bursts);
    void logProcessBursts(const unsigned int bursts[], size_t count);

    void logCpuBurstExecution(unsigned int procId,
                              unsigned int cpuExecutedTime,
                              unsigned int ioExecutedTime,
                              unsigned int totalElapsedTime,
                              ExecutionStopReason stopReason);

    void logProcessCompletion(unsigned int procId,
                              unsigned int completionTime,
                              unsigned int totalWaitTime);

    void logAgingEvent(unsigned int procId, int oldPriority, int newPriority, unsigned int globalTime);

    void logInfo(const std::string& message);

private:
    Logger() = default;
    std::mutex printMutex_;
};

// C-compatible wrapper functions for backward compatibility
void log_cpuburst_execution(unsigned int procID,
                            unsigned int cpuExecutedTime,
                            unsigned int ioExecutedTime,
                            unsigned int totalElapsedTime,
                            ExecutionStopReason stopReason);

void log_process_bursts(unsigned int bursts[], size_t numOfBursts);

void log_process_completion(unsigned int procID,
                            unsigned int completionTime,
                            unsigned int totalWaitTime);

#endif // LOGGER_HPP
