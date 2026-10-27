#ifndef METRICS_HPP
#define METRICS_HPP

#include <vector>
#include <string>
#include "process.hpp"

struct SystemMetrics {
    unsigned int totalSimulationTime = 0;
    unsigned int totalActiveCpuTime = 0;
    unsigned int totalIdleCpuTime = 0;
    unsigned int totalIoTime = 0;
    unsigned int totalProcesses = 0;
    unsigned int contextSwitches = 0;
    unsigned int contextSwitchCost = 0; // Cost per switch in time units
    unsigned int totalContextSwitchOverhead = 0;

    double cpuUtilizationPercent = 0.0;
    double throughput = 0.0; // processes per 100 time units or per unit time

    double avgTurnaroundTime = 0.0;
    double avgWaitTime = 0.0;
    double avgResponseTime = 0.0;

    unsigned int maxTurnaroundTime = 0;
    unsigned int minTurnaroundTime = 0;
    unsigned int maxWaitTime = 0;
    unsigned int minWaitTime = 0;
};

class MetricsTracker {
public:
    explicit MetricsTracker(unsigned int contextSwitchCost = 0);

    void recordContextSwitch();
    void finalize(unsigned int totalSimulationTime,
                  unsigned int totalCpuTime,
                  const std::vector<ProcessPtr>& finishedProcesses);

    [[nodiscard]] const SystemMetrics& getMetrics() const;
    void printReport(const std::string& algorithmName) const;

private:
    unsigned int contextSwitchCost_ = 0;
    SystemMetrics metrics_;
};

#endif // METRICS_HPP
