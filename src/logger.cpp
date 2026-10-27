#include "logger.hpp"
#include <cstdio>

static const char* stopReasonNames[] = {
    "enter io",
    "quantum expired",
    "preempted (srtf)",
    "completed"
};

Logger& Logger::instance() {
    static Logger loggerInstance;
    return loggerInstance;
}

void Logger::logProcessBursts(const std::vector<unsigned int>& bursts) {
    std::lock_guard<std::mutex> lock(printMutex_);
    for (size_t i = 0; i < bursts.size(); ++i) {
        std::cout << bursts[i] << (i + 1 < bursts.size() ? " " : "");
    }
    std::cout << std::endl;
}

void Logger::logProcessBursts(const unsigned int bursts[], size_t count) {
    std::lock_guard<std::mutex> lock(printMutex_);
    for (size_t i = 0; i < count; ++i) {
        std::cout << bursts[i] << (i + 1 < count ? " " : "");
    }
    std::cout << std::endl;
}

void Logger::logCpuBurstExecution(unsigned int procId,
                                  unsigned int cpuExecutedTime,
                                  unsigned int ioExecutedTime,
                                  unsigned int totalElapsedTime,
                                  ExecutionStopReason stopReason) {
    std::lock_guard<std::mutex> lock(printMutex_);
    std::cout << "P" << procId
              << ": executed cpu bursts = " << cpuExecutedTime
              << ", executed io bursts = " << ioExecutedTime
              << ", time elapsed = " << totalElapsedTime
              << ", " << stopReasonNames[static_cast<int>(stopReason)]
              << std::endl;
}

void Logger::logProcessCompletion(unsigned int procId,
                                  unsigned int completionTime,
                                  unsigned int totalWaitTime) {
    std::lock_guard<std::mutex> lock(printMutex_);
    std::cout << "P" << procId
              << ": turnaround time = " << completionTime
              << ", wait time = " << totalWaitTime
              << std::endl;
}

void Logger::logAgingEvent(unsigned int procId, int oldPriority, int newPriority, unsigned int globalTime) {
    std::lock_guard<std::mutex> lock(printMutex_);
    std::cout << "[AGING] T=" << globalTime << " | Process P" << procId
              << " aged: priority boosted from " << oldPriority << " to " << newPriority
              << std::endl;
}

void Logger::logInfo(const std::string& message) {
    std::lock_guard<std::mutex> lock(printMutex_);
    std::cout << message << std::endl;
}

void log_cpuburst_execution(unsigned int procID,
                            unsigned int cpuExecutedTime,
                            unsigned int ioExecutedTime,
                            unsigned int totalElapsedTime,
                            ExecutionStopReason stopReason) {
    Logger::instance().logCpuBurstExecution(procID, cpuExecutedTime, ioExecutedTime, totalElapsedTime, stopReason);
}

void log_process_bursts(unsigned int bursts[], size_t numOfBursts) {
    Logger::instance().logProcessBursts(bursts, numOfBursts);
}

void log_process_completion(unsigned int procID,
                            unsigned int completionTime,
                            unsigned int totalWaitTime) {
    Logger::instance().logProcessCompletion(procID, completionTime, totalWaitTime);
}
