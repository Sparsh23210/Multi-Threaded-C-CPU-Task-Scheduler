#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>

#include "process.hpp"
#include "scheduler.hpp"
#include "logger.hpp"
#include "metrics.hpp"

static void printHelp(const char* progName) {
    std::cout << "Usage: " << progName << " [OPTIONS] [bursts_file]\n"
              << "Options:\n"
              << "  -s, --algo <fcfs|rr|priority|srtf>   Scheduling algorithm (default: fcfs)\n"
              << "  -q, --quantum <int>                  Time quantum for Round Robin / Priority (default: 2)\n"
              << "  --aging-interval <int>               Ticks before priority is boosted (default: 5)\n"
              << "  --aging-boost <int>                  Amount to increment priority by (default: 1)\n"
              << "  --cs-cost <int>                      Context switch overhead ticks (default: 0)\n"
              << "  --delay <microseconds>               Simulation tick delay (default: 500us)\n"
              << "  --fast                               Run simulation without delays (fast benchmarking)\n"
              << "  -i, --interactive                    Interactive CLI process creator mode\n"
              << "  -h, --help                           Show this help menu\n";
}

int main(int argc, char** argv) {
    SchedulerConfig config;
    std::string filename;
    std::string algoName = "FCFS";
    bool interactiveMode = false;

    // Command-line parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "-s" || arg == "--algo") {
            if (i + 1 < argc) {
                std::string a = argv[++i];
                for (auto& c : a) c = tolower(c);
                if (a == "fcfs") {
                    config.algorithm = SchedulerAlgorithm::FCFS;
                    algoName = "First-Come, First-Served (FCFS)";
                } else if (a == "rr") {
                    config.algorithm = SchedulerAlgorithm::RR;
                    algoName = "Round Robin (RR)";
                } else if (a == "priority") {
                    config.algorithm = SchedulerAlgorithm::PRIORITY_AGING;
                    algoName = "Priority Scheduling with Aging";
                } else if (a == "srtf") {
                    config.algorithm = SchedulerAlgorithm::SRTF;
                    algoName = "Shortest Remaining Time First (SRTF)";
                } else {
                    std::cerr << "Unknown algorithm: " << a << std::endl;
                    return 1;
                }
            }
        } else if (arg == "-q" || arg == "--quantum") {
            if (i + 1 < argc) {
                config.quantum = static_cast<unsigned int>(std::max(1, std::stoi(argv[++i])));
            }
        } else if (arg == "--aging-interval") {
            if (i + 1 < argc) {
                config.agingInterval = static_cast<unsigned int>(std::max(1, std::stoi(argv[++i])));
            }
        } else if (arg == "--aging-boost") {
            if (i + 1 < argc) {
                config.agingBoost = std::stoi(argv[++i]);
            }
        } else if (arg == "--cs-cost") {
            if (i + 1 < argc) {
                config.contextSwitchCost = static_cast<unsigned int>(std::max(0, std::stoi(argv[++i])));
            }
        } else if (arg == "--delay") {
            if (i + 1 < argc) {
                config.tickDelayUs = static_cast<unsigned int>(std::max(0, std::stoi(argv[++i])));
            }
        } else if (arg == "--fast") {
            config.tickDelayUs = 0;
        } else if (arg == "-i" || arg == "--interactive") {
            interactiveMode = true;
        } else if (arg[0] != '-') {
            filename = arg;
        }
    }

    std::vector<ProcessPtr> processes;
    unsigned int pidCounter = 0;

    if (interactiveMode) {
        std::cout << "=== Interactive Process Creation Mode ===" << std::endl;
        std::cout << "Enter bursts for processes (odd number of positive integers: CPU IO CPU ...)" << std::endl;
        std::cout << "Optionally specify priority at prompt. Type 'run' or 'done' when finished.\n" << std::endl;

        while (true) {
            std::cout << "Process P" << pidCounter << " bursts: ";
            std::string line;
            if (!std::getline(std::cin, line) || line == "done" || line == "run" || line.empty()) {
                break;
            }

            std::istringstream iss(line);
            std::vector<unsigned int> bursts;
            int val;
            while (iss >> val) {
                if (val > 0) bursts.push_back(static_cast<unsigned int>(val));
            }

            if (bursts.size() % 2 == 0) {
                std::cout << "  [Error] Must have an odd number of bursts (CPU IO CPU ...). Try again." << std::endl;
                continue;
            }

            int prio = 10;
            if (config.algorithm == SchedulerAlgorithm::PRIORITY_AGING) {
                std::cout << "  Priority for P" << pidCounter << " (higher integer = higher priority, default 10): ";
                std::string prioStr;
                if (std::getline(std::cin, prioStr) && !prioStr.empty()) {
                    prio = std::stoi(prioStr);
                }
            }

            auto proc = std::make_shared<Process>(pidCounter++, bursts, prio);
            processes.push_back(proc);
        }
    } else {
        if (filename.empty()) {
            // Default to bursts_rr_3.txt or tests/sample_bursts.txt if exists
            std::ifstream checkRoot("bursts_rr_3.txt");
            if (checkRoot.is_open()) {
                filename = "bursts_rr_3.txt";
            } else {
                filename = "tests/sample_bursts.txt";
            }
        }

        std::ifstream fin(filename);
        if (!fin.is_open()) {
            std::cerr << "Unable to open input file: " << filename << std::endl;
            return 1;
        }

        std::string line;
        while (std::getline(fin, line)) {
            // Skip empty lines or comment lines
            if (line.find_first_not_of(" \t\r\n") == std::string::npos || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::vector<unsigned int> bursts;
            int val;
            while (iss >> val) {
                if (val > 0) {
                    bursts.push_back(static_cast<unsigned int>(val));
                }
            }

            if (bursts.empty()) continue;

            if (bursts.size() % 2 == 0) {
                std::cerr << "Error: process must have an odd number of bursts (line: " << line << ")" << std::endl;
                continue;
            }

            int prio = 10 + static_cast<int>(pidCounter * 2); // Default varying priority if not provided
            auto proc = std::make_shared<Process>(pidCounter++, bursts, prio);
            processes.push_back(proc);
        }
    }

    if (processes.empty()) {
        std::cerr << "No valid processes to schedule." << std::endl;
        return 1;
    }

    // Print initial bursts
    for (const auto& p : processes) {
        std::vector<unsigned int> temp(p->bursts.begin(), p->bursts.end());
        Logger::instance().logProcessBursts(temp);
    }

    // Initialize and run scheduler
    CPUScheduler scheduler(config);
    for (const auto& p : processes) {
        scheduler.addProcess(p);
    }

    scheduler.run();

    // Print comprehensive metrics report
    scheduler.getMetricsTracker().printReport(algoName);

    return 0;
}
