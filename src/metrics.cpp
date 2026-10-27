#include "metrics.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

MetricsTracker::MetricsTracker(unsigned int contextSwitchCost)
    : contextSwitchCost_(contextSwitchCost) {
    metrics_.contextSwitchCost = contextSwitchCost;
}

void MetricsTracker::recordContextSwitch() {
    metrics_.contextSwitches++;
}

void MetricsTracker::finalize(unsigned int totalSimulationTime,
                              unsigned int totalCpuTime,
                              const std::vector<ProcessPtr>& finishedProcesses) {
    metrics_.totalSimulationTime = totalSimulationTime;
    metrics_.totalActiveCpuTime = totalCpuTime;
    metrics_.totalProcesses = static_cast<unsigned int>(finishedProcesses.size());
    metrics_.totalContextSwitchOverhead = metrics_.contextSwitches * contextSwitchCost_;

    if (metrics_.totalSimulationTime > 0) {
        metrics_.totalIdleCpuTime = (metrics_.totalSimulationTime > metrics_.totalActiveCpuTime)
            ? (metrics_.totalSimulationTime - metrics_.totalActiveCpuTime) : 0;
        metrics_.cpuUtilizationPercent = (static_cast<double>(metrics_.totalActiveCpuTime) / metrics_.totalSimulationTime) * 100.0;
        metrics_.throughput = static_cast<double>(metrics_.totalProcesses) / metrics_.totalSimulationTime;
    }

    if (finishedProcesses.empty()) {
        return;
    }

    unsigned long long sumTurnaround = 0;
    unsigned long long sumWait = 0;
    unsigned long long sumResponse = 0;

    metrics_.minTurnaroundTime = finishedProcesses.front()->turnaroundTime;
    metrics_.maxTurnaroundTime = finishedProcesses.front()->turnaroundTime;
    metrics_.minWaitTime = finishedProcesses.front()->totalWaitTime;
    metrics_.maxWaitTime = finishedProcesses.front()->totalWaitTime;

    for (const auto& p : finishedProcesses) {
        sumTurnaround += p->turnaroundTime;
        sumWait += p->totalWaitTime;
        sumResponse += p->responseTime;

        metrics_.minTurnaroundTime = std::min(metrics_.minTurnaroundTime, p->turnaroundTime);
        metrics_.maxTurnaroundTime = std::max(metrics_.maxTurnaroundTime, p->turnaroundTime);
        metrics_.minWaitTime = std::min(metrics_.minWaitTime, p->totalWaitTime);
        metrics_.maxWaitTime = std::max(metrics_.maxWaitTime, p->totalWaitTime);
    }

    metrics_.avgTurnaroundTime = static_cast<double>(sumTurnaround) / finishedProcesses.size();
    metrics_.avgWaitTime = static_cast<double>(sumWait) / finishedProcesses.size();
    metrics_.avgResponseTime = static_cast<double>(sumResponse) / finishedProcesses.size();
}

const SystemMetrics& MetricsTracker::getMetrics() const {
    return metrics_;
}

void MetricsTracker::printReport(const std::string& algorithmName) const {
    std::cout << "\n================================================================" << std::endl;
    std::cout << "               SCHEDULER PERFORMANCE REPORT                    " << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << " Algorithm Used             : " << algorithmName << std::endl;
    std::cout << " Total Processes Completed  : " << metrics_.totalProcesses << std::endl;
    std::cout << " Total Simulation Time      : " << metrics_.totalSimulationTime << " ticks" << std::endl;
    std::cout << " Total Active CPU Time      : " << metrics_.totalActiveCpuTime << " ticks" << std::endl;
    std::cout << " Total Idle CPU Time        : " << metrics_.totalIdleCpuTime << " ticks" << std::endl;
    std::cout << " CPU Utilization            : " << std::fixed << std::setprecision(2)
              << metrics_.cpuUtilizationPercent << "%" << std::endl;
    std::cout << " Throughput                 : " << std::setprecision(4)
              << metrics_.throughput << " processes/tick" << std::endl;
    std::cout << " Total Context Switches     : " << metrics_.contextSwitches << std::endl;
    std::cout << " Context Switch Cost/Unit   : " << metrics_.contextSwitchCost << " ticks" << std::endl;
    std::cout << " Total Context Switch Delay : " << metrics_.totalContextSwitchOverhead << " ticks" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << " METRIC                AVG        MIN        MAX" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << " Turnaround Time       "
              << std::setw(8) << std::fixed << std::setprecision(2) << metrics_.avgTurnaroundTime
              << std::setw(11) << metrics_.minTurnaroundTime
              << std::setw(11) << metrics_.maxTurnaroundTime << std::endl;
    std::cout << " Waiting Time          "
              << std::setw(8) << std::fixed << std::setprecision(2) << metrics_.avgWaitTime
              << std::setw(11) << metrics_.minWaitTime
              << std::setw(11) << metrics_.maxWaitTime << std::endl;
    std::cout << " Response Time         "
              << std::setw(8) << std::fixed << std::setprecision(2) << metrics_.avgResponseTime
              << std::setw(11) << "-"
              << std::setw(11) << "-" << std::endl;
    std::cout << "================================================================\n" << std::endl;
}
