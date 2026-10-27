#include "scheduler.hpp"
#include <chrono>
#include <algorithm>
#include <iostream>

CPUScheduler::CPUScheduler(const SchedulerConfig& config)
    : config_(config), metricsTracker_(config.contextSwitchCost) {}

CPUScheduler::~CPUScheduler() {
    isRunning_ = false;
    cvReady_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void CPUScheduler::addProcess(const ProcessPtr& process) {
    std::lock_guard<std::mutex> lock(readyMutex_);
    process->readyQueueEntryTime = globalTime_;
    readyQueue_.push_back(process);
    cvReady_.notify_one();
}

ProcessPtr CPUScheduler::selectNextProcess() {
    std::lock_guard<std::mutex> lock(readyMutex_);
    if (readyQueue_.empty()) {
        return nullptr;
    }

    if (config_.algorithm == SchedulerAlgorithm::FCFS || config_.algorithm == SchedulerAlgorithm::RR) {
        ProcessPtr selected = readyQueue_.front();
        readyQueue_.pop_front();
        return selected;
    }

    if (config_.algorithm == SchedulerAlgorithm::PRIORITY_AGING) {
        auto highestIt = std::max_element(readyQueue_.begin(), readyQueue_.end(),
            [](const ProcessPtr& a, const ProcessPtr& b) {
                if (a->currentPriority == b->currentPriority) {
                    return a->readyQueueEntryTime > b->readyQueueEntryTime;
                }
                return a->currentPriority < b->currentPriority;
            });

        ProcessPtr selected = *highestIt;
        readyQueue_.erase(highestIt);
        return selected;
    }

    if (config_.algorithm == SchedulerAlgorithm::SRTF) {
        auto shortestIt = std::min_element(readyQueue_.begin(), readyQueue_.end(),
            [](const ProcessPtr& a, const ProcessPtr& b) {
                unsigned int remA = a->bursts.empty() ? 0 : a->bursts.front();
                unsigned int remB = b->bursts.empty() ? 0 : b->bursts.front();
                if (remA == remB) {
                    return a->readyQueueEntryTime > b->readyQueueEntryTime;
                }
                return remA < remB;
            });

        ProcessPtr selected = *shortestIt;
        readyQueue_.erase(shortestIt);
        return selected;
    }

    ProcessPtr selected = readyQueue_.front();
    readyQueue_.pop_front();
    return selected;
}

void CPUScheduler::applyAgingMechanism() {
    if (config_.algorithm != SchedulerAlgorithm::PRIORITY_AGING) {
        return;
    }

    std::lock_guard<std::mutex> lock(readyMutex_);
    for (auto& p : readyQueue_) {
        if (globalTime_ > p->readyQueueEntryTime &&
            (globalTime_ - p->readyQueueEntryTime) >= config_.agingInterval) {
            int oldPriority = p->currentPriority;
            p->currentPriority += config_.agingBoost;
            p->readyQueueEntryTime = globalTime_;
            Logger::instance().logAgingEvent(p->id, oldPriority, p->currentPriority, globalTime_);
        }
    }
}

void CPUScheduler::handleBlockedProcesses() {
    std::lock_guard<std::mutex> lockBlocked(blockedMutex_);
    std::deque<ProcessPtr> stillBlocked;

    while (!blockedQueue_.empty()) {
        ProcessPtr p = blockedQueue_.front();
        blockedQueue_.pop_front();

        // Process just entered blocked queue in this exact tick, wait until next tick
        if (p->blockedEntryTime == globalTime_) {
            stillBlocked.push_back(p);
            continue;
        }

        if (p->remainingIoBurst <= 1) {
            p->remainingIoBurst = 0;
            p->readyQueueEntryTime = globalTime_;

            std::lock_guard<std::mutex> lockReady(readyMutex_);
            readyQueue_.push_back(p);
            cvReady_.notify_one();
        } else {
            p->remainingIoBurst -= 1;
            stillBlocked.push_back(p);
        }
    }

    blockedQueue_ = std::move(stillBlocked);
}

void CPUScheduler::workerRoutine() {
    while (isRunning_) {
        ProcessPtr p = selectNextProcess();

        if (!p) {
            isWorkerActive_ = false;
            std::unique_lock<std::mutex> lock(readyMutex_);
            cvReady_.wait_for(lock, std::chrono::milliseconds(2), [this] {
                return !readyQueue_.empty() || !isRunning_;
            });
            continue;
        }

        isWorkerActive_ = true;

        // Context switch tracking
        if (lastExecutedPid_ != -1 && lastExecutedPid_ != static_cast<int>(p->id)) {
            metricsTracker_.recordContextSwitch();
            if (config_.contextSwitchCost > 0) {
                for (unsigned int cs = 0; cs < config_.contextSwitchCost; ++cs) {
                    globalTime_++;
                    if (config_.tickDelayUs > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(config_.tickDelayUs));
                    }
                }
            }
        }
        lastExecutedPid_ = static_cast<int>(p->id);

        if (!p->hasStarted) {
            p->hasStarted = true;
            p->firstCpuTime = globalTime_;
            p->responseTime = (globalTime_ >= p->arrivalTime) ? (globalTime_ - p->arrivalTime) : 0;
        }

        unsigned int cpuBurst = p->bursts.front();
        bool preempted = false;

        if (config_.algorithm == SchedulerAlgorithm::FCFS) {
            // Run entire CPU burst
            while (cpuBurst > 0 && isRunning_) {
                if (config_.tickDelayUs > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(config_.tickDelayUs));
                }
                globalTime_++;
                totalActiveCpuTime_++;
                p->totalCpuExecuted++;
                cpuBurst--;
            }
        } else if (config_.algorithm == SchedulerAlgorithm::RR) {
            // Run up to quantum
            unsigned int ticks = 0;
            while (ticks < config_.quantum && cpuBurst > 0 && isRunning_) {
                if (config_.tickDelayUs > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(config_.tickDelayUs));
                }
                globalTime_++;
                totalActiveCpuTime_++;
                p->totalCpuExecuted++;
                cpuBurst--;
                ticks++;
            }
            if (cpuBurst > 0) {
                preempted = true;
            }
        } else if (config_.algorithm == SchedulerAlgorithm::PRIORITY_AGING) {
            // Run up to quantum ticks or until a higher priority process enters ready queue
            unsigned int ticks = 0;
            while (ticks < config_.quantum && cpuBurst > 0 && isRunning_) {
                if (config_.tickDelayUs > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(config_.tickDelayUs));
                }
                globalTime_++;
                totalActiveCpuTime_++;
                p->totalCpuExecuted++;
                cpuBurst--;
                ticks++;

                // Check if higher priority task arrived
                std::lock_guard<std::mutex> lock(readyMutex_);
                if (!readyQueue_.empty()) {
                    for (const auto& other : readyQueue_) {
                        if (other->currentPriority > p->currentPriority) {
                            preempted = true;
                            break;
                        }
                    }
                    if (preempted) break;
                }
            }
            if (cpuBurst > 0) {
                preempted = true;
            }
        } else if (config_.algorithm == SchedulerAlgorithm::SRTF) {
            // Run tick by tick, check if newly arrived ready process has strictly shorter remaining burst
            while (cpuBurst > 0 && isRunning_) {
                if (config_.tickDelayUs > 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(config_.tickDelayUs));
                }
                globalTime_++;
                totalActiveCpuTime_++;
                p->totalCpuExecuted++;
                cpuBurst--;

                if (cpuBurst > 0) {
                    std::lock_guard<std::mutex> lock(readyMutex_);
                    for (const auto& other : readyQueue_) {
                        unsigned int otherRem = other->bursts.empty() ? 0 : other->bursts.front();
                        if (otherRem < cpuBurst) {
                            preempted = true;
                            break;
                        }
                    }
                    if (preempted) break;
                }
            }
        }

        if (cpuBurst == 0) {
            p->bursts.pop_front();

            if (p->bursts.empty()) {
                p->turnaroundTime = globalTime_;
                unsigned int totalBusy = p->totalCpuExecuted + p->totalIoExecuted;
                p->totalWaitTime = (p->turnaroundTime >= totalBusy) ? (p->turnaroundTime - totalBusy) : 0;

                {
                    std::lock_guard<std::mutex> lockFinished(finishedMutex_);
                    finishedQueue_.push_back(p);
                }

                Logger::instance().logCpuBurstExecution(p->id, p->totalCpuExecuted, p->totalIoExecuted,
                                                        globalTime_, ExecutionStopReason::COMPLETED);
            } else {
                unsigned int ioBurst = p->bursts.front();
                p->bursts.pop_front();
                p->remainingIoBurst = ioBurst;
                p->blockedEntryTime = globalTime_;
                p->totalIoExecuted += ioBurst;

                {
                    std::lock_guard<std::mutex> lockBlocked(blockedMutex_);
                    blockedQueue_.push_back(p);
                }

                Logger::instance().logCpuBurstExecution(p->id, p->totalCpuExecuted, p->totalIoExecuted,
                                                        globalTime_, ExecutionStopReason::ENTER_IO);
            }
        } else {
            // Task was preempted by quantum expiration or shorter/higher priority process
            p->bursts.front() = cpuBurst;
            p->readyQueueEntryTime = globalTime_;

            {
                std::lock_guard<std::mutex> lockReady(readyMutex_);
                readyQueue_.push_back(p);
                cvReady_.notify_one();
            }

            ExecutionStopReason reason = (config_.algorithm == SchedulerAlgorithm::SRTF)
                                            ? ExecutionStopReason::PREEMPTED_SRTF
                                            : ExecutionStopReason::QUANTUM_EXPIRED;

            Logger::instance().logCpuBurstExecution(p->id, p->totalCpuExecuted, p->totalIoExecuted,
                                                    globalTime_, reason);
        }

        isWorkerActive_ = false;
    }
}

void CPUScheduler::run() {
    isRunning_ = true;
    workerThread_ = std::thread(&CPUScheduler::workerRoutine, this);

    while (isRunning_) {
        // Advance CPU clock when CPU is idle (all unfinished processes in IO)
        bool readyEmpty = false;
        {
            std::lock_guard<std::mutex> lock(readyMutex_);
            readyEmpty = readyQueue_.empty();
        }

        bool blockedNotEmpty = false;
        {
            std::lock_guard<std::mutex> lock(blockedMutex_);
            blockedNotEmpty = !blockedQueue_.empty();
        }

        if (readyEmpty && blockedNotEmpty && !isWorkerActive_) {
            // CPU is idle, advance time tick
            if (config_.tickDelayUs > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(config_.tickDelayUs));
            }
            globalTime_++;
        }

        handleBlockedProcesses();
        applyAgingMechanism();

        if (config_.tickDelayUs > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(std::min(500u, config_.tickDelayUs)));
        }

        // Termination check
        bool readyEmptyFinal = false;
        {
            std::lock_guard<std::mutex> lock(readyMutex_);
            readyEmptyFinal = readyQueue_.empty();
        }

        bool blockedEmptyFinal = false;
        {
            std::lock_guard<std::mutex> lock(blockedMutex_);
            blockedEmptyFinal = blockedQueue_.empty();
        }

        if (readyEmptyFinal && blockedEmptyFinal && !isWorkerActive_) {
            isRunning_ = false;
            cvReady_.notify_all();
            break;
        }
    }

    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    // Print individual process completion logs
    {
        std::lock_guard<std::mutex> lock(finishedMutex_);
        std::sort(finishedQueue_.begin(), finishedQueue_.end(),
                  [](const ProcessPtr& a, const ProcessPtr& b) { return a->id < b->id; });

        for (const auto& p : finishedQueue_) {
            Logger::instance().logProcessCompletion(p->id, p->turnaroundTime, p->totalWaitTime);
        }
    }

    // Finalize metrics
    metricsTracker_.finalize(globalTime_, totalActiveCpuTime_, finishedQueue_);
}

const MetricsTracker& CPUScheduler::getMetricsTracker() const {
    return metricsTracker_;
}

const std::vector<ProcessPtr>& CPUScheduler::getFinishedProcesses() const {
    return finishedQueue_;
}
