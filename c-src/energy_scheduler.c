#include <stdio.h>
#include <stdlib.h>

// Define a maximum number of processes to keep array handling simple.
#define MAX_PROCESSES 50

// Define a structure to store all process-related information.
typedef struct {
    int id;                  // Process ID.
    double burst;            // CPU burst time required by process.
    int priority;            // Priority of process (1 means highest priority).
    double temperature;      // Initial temperature associated with process/system.
    double basePower;        // Base power usage in watts at baseline settings.
    int scheduled;           // Flag to mark whether process is already scheduled.
} Process;

// Define a structure for each DVFS level.
typedef struct {
    const char *name;        // Level name for display.
    double frequency;        // CPU frequency in GHz.
    double voltage;          // CPU voltage in Volts.
} DvfsLevel;

// Define a structure to record output details for each scheduled process.
typedef struct {
    int processId;           // Process ID in execution order.
    const char *dvfsName;    // Selected DVFS level name.
    double frequency;        // Selected frequency.
    double voltage;          // Selected voltage.
    double execTime;         // Computed execution time at selected frequency.
    double dynPower;         // Computed dynamic power for selected settings.
    double energy;           // Energy consumed by this process.
    double finishTime;       // Finish time in the optimized schedule.
} ScheduleRecord;

// Declare baseline constants used for relative scaling in energy model.
const double BASELINE_FREQ = 2.5;   // Baseline frequency in GHz.
const double BASELINE_VOLT = 1.2;   // Baseline voltage in Volts.

// Create fixed DVFS levels: low, medium, and high.
DvfsLevel DVFS_LEVELS[] = {
    {"LOW", 1.2, 0.90},
    {"MEDIUM", 1.8, 1.00},
    {"HIGH", 2.5, 1.20}
};

// This function chooses DVFS level index using temperature, priority, and burst.
int chooseDvfsLevel(double temperature, int priority, double burst) {
    // If temperature is too high, force low level to cool down.
    if (temperature >= 80.0) {
        return 0;
    }

    // If task is urgent by priority or very short, use high for responsiveness.
    if (priority <= 2 || burst <= 4.0) {
        return 2;
    }

    // If temperature is moderately high, stay in medium.
    if (temperature >= 70.0) {
        return 1;
    }

    // For long tasks, choose low to save energy.
    if (burst > 10.0) {
        return 0;
    }

    // Default case uses medium balance.
    return 1;
}

// This function computes selection score for scheduling decision.
double computeScore(Process p, int orderIndex) {
    // Higher priority value should map to lower score; so invert it.
    double priorityTerm = 50.0 / (double)p.priority;

    // Short jobs get a bonus to improve response time.
    double shortJobTerm = 30.0 / (p.burst + 1.0);

    // Add fairness aging term so waiting jobs gradually gain chance.
    double agingTerm = 2.0 * (double)orderIndex;

    // Apply penalty when temperature is high.
    double thermalPenalty = (p.temperature > 70.0) ? (p.temperature - 70.0) : 0.0;

    // Return final score.
    return priorityTerm + shortJobTerm + agingTerm - thermalPenalty;
}

// This function computes dynamic power using simplified CMOS relation.
double computeDynamicPower(double basePower, double voltage, double frequency) {
    // Scale power by V^2 and f relative to baseline.
    double voltageFactor = (voltage * voltage) / (BASELINE_VOLT * BASELINE_VOLT);
    double frequencyFactor = frequency / BASELINE_FREQ;

    // Return estimated dynamic power.
    return basePower * voltageFactor * frequencyFactor;
}

// This function prints a separator line for better readability.
void printLine() {
    printf("-------------------------------------------------------------------------------\n");
}

int main() {
    // Declare variables for number of processes and loop counters.
    int n, i;

    // Create process array and schedule record array.
    Process processes[MAX_PROCESSES];
    ScheduleRecord records[MAX_PROCESSES];

    // Variables to store total energy and performance metrics.
    double baselineEnergy = 0.0;
    double optimizedEnergy = 0.0;
    double baselineTime = 0.0;
    double optimizedTime = 0.0;

    // Variables for waiting and turnaround time statistics.
    double baselineWaitingSum = 0.0;
    double optimizedWaitingSum = 0.0;
    double baselineTurnaroundSum = 0.0;
    double optimizedTurnaroundSum = 0.0;

    // Print title and purpose to user.
    printf("\nEnergy-Efficient CPU Scheduling Algorithm (DVFS + Thermal-Aware)\n");
    printLine();

    // Ask user for number of processes.
    printf("Enter number of processes (1-%d): ", MAX_PROCESSES);
    scanf("%d", &n);

    // Validate process count.
    if (n < 1 || n > MAX_PROCESSES) {
        printf("Invalid number of processes. Exiting.\n");
        return 1;
    }

    // Read each process details from user.
    for (i = 0; i < n; i++) {
        processes[i].id = i + 1;
        processes[i].scheduled = 0;

        printf("\nEnter details for Process P%d\n", processes[i].id);

        printf("Burst time: ");
        scanf("%lf", &processes[i].burst);

        printf("Priority (1 = highest): ");
        scanf("%d", &processes[i].priority);

        printf("Current temperature (in C): ");
        scanf("%lf", &processes[i].temperature);

        printf("Base power usage (in Watts): ");
        scanf("%lf", &processes[i].basePower);

        // Validate basic constraints for safe computation.
        if (processes[i].burst <= 0 || processes[i].priority <= 0 || processes[i].basePower <= 0) {
            printf("Invalid input detected. Burst, priority, and power must be positive.\n");
            return 1;
        }
    }

    // Compute baseline metrics using FCFS + fixed high frequency.
    double fcfsClock = 0.0;
    for (i = 0; i < n; i++) {
        // Baseline executes at baseline frequency so burst remains unchanged.
        double execTime = processes[i].burst;

        // Baseline power at baseline voltage/frequency equals base power.
        double dynPower = computeDynamicPower(processes[i].basePower, BASELINE_VOLT, BASELINE_FREQ);

        // Energy is power multiplied by execution time.
        double energy = dynPower * execTime;

        // Update baseline totals.
        baselineEnergy += energy;
        baselineTime += execTime;

        // For FCFS waiting time is accumulated previous execution times.
        baselineWaitingSum += fcfsClock;

        // Turnaround time is waiting plus own execution.
        baselineTurnaroundSum += (fcfsClock + execTime);

        // Move FCFS clock forward.
        fcfsClock += execTime;
    }

    // Schedule processes using custom energy-aware strategy.
    int scheduledCount = 0;
    double currentClock = 0.0;

    while (scheduledCount < n) {
        // Track best candidate process index and score.
        int bestIndex = -1;
        double bestScore = -1e18;

        // Find highest score among unscheduled processes.
        for (i = 0; i < n; i++) {
            if (!processes[i].scheduled) {
                double score = computeScore(processes[i], scheduledCount);

                if (score > bestScore) {
                    bestScore = score;
                    bestIndex = i;
                }
            }
        }

        // Safety check for selection.
        if (bestIndex == -1) {
            printf("Unexpected scheduling error.\n");
            return 1;
        }

        // Choose DVFS level for selected process.
        int levelIndex = chooseDvfsLevel(
            processes[bestIndex].temperature,
            processes[bestIndex].priority,
            processes[bestIndex].burst
        );

        DvfsLevel level = DVFS_LEVELS[levelIndex];

        // Execution time scales inversely with frequency.
        double execTime = processes[bestIndex].burst * (BASELINE_FREQ / level.frequency);

        // Compute dynamic power under selected DVFS settings.
        double dynPower = computeDynamicPower(processes[bestIndex].basePower, level.voltage, level.frequency);

        // Compute energy for this process.
        double energy = dynPower * execTime;

        // Fill output record.
        records[scheduledCount].processId = processes[bestIndex].id;
        records[scheduledCount].dvfsName = level.name;
        records[scheduledCount].frequency = level.frequency;
        records[scheduledCount].voltage = level.voltage;
        records[scheduledCount].execTime = execTime;
        records[scheduledCount].dynPower = dynPower;
        records[scheduledCount].energy = energy;
        records[scheduledCount].finishTime = currentClock + execTime;

        // Waiting time is accumulated elapsed clock before process starts.
        optimizedWaitingSum += currentClock;

        // Turnaround time is waiting plus execution.
        optimizedTurnaroundSum += (currentClock + execTime);

        // Update totals and clock.
        optimizedEnergy += energy;
        optimizedTime += execTime;
        currentClock += execTime;

        // Mark process as scheduled.
        processes[bestIndex].scheduled = 1;

        // Update thermal state for remaining processes (simple model).
        // If a high level runs, nearby tasks heat slightly; otherwise cooling effect.
        for (i = 0; i < n; i++) {
            if (!processes[i].scheduled) {
                if (levelIndex == 2) {
                    processes[i].temperature += 0.6;
                } else if (levelIndex == 1) {
                    processes[i].temperature += 0.2;
                } else {
                    processes[i].temperature -= 0.4;
                }

                // Clamp to reasonable simulation bounds.
                if (processes[i].temperature < 25.0) {
                    processes[i].temperature = 25.0;
                }
                if (processes[i].temperature > 95.0) {
                    processes[i].temperature = 95.0;
                }
            }
        }

        // Increment number of scheduled processes.
        scheduledCount++;
    }

    // Print scheduling order and per-process results.
    printf("\nOptimized Scheduling Order and DVFS Decisions\n");
    printLine();
    printf("%-10s %-10s %-12s %-10s %-12s %-12s %-12s\n",
           "Process", "DVFS", "Freq(GHz)", "Volt(V)", "ExecTime", "Power(W)", "Energy(J)");
    printLine();

    for (i = 0; i < n; i++) {
        printf("P%-9d %-10s %-12.2f %-10.2f %-12.2f %-12.2f %-12.2f\n",
               records[i].processId,
               records[i].dvfsName,
               records[i].frequency,
               records[i].voltage,
               records[i].execTime,
               records[i].dynPower,
               records[i].energy);
    }

    printLine();

    // Compute averages for both methods.
    double baselineAvgWaiting = baselineWaitingSum / n;
    double optimizedAvgWaiting = optimizedWaitingSum / n;
    double baselineAvgTurnaround = baselineTurnaroundSum / n;
    double optimizedAvgTurnaround = optimizedTurnaroundSum / n;

    // Compute energy savings percentage.
    double energySaved = baselineEnergy - optimizedEnergy;
    double energySavedPercent = (energySaved / baselineEnergy) * 100.0;

    // Compute performance impact as total-time increase percentage.
    double timeIncrease = optimizedTime - baselineTime;
    double timeIncreasePercent = (timeIncrease / baselineTime) * 100.0;

    // Print comparison summary.
    printf("\nComparison: Baseline (FCFS + Fixed High Freq) vs Optimized\n");
    printLine();
    printf("Baseline Total Energy      : %.2f J\n", baselineEnergy);
    printf("Optimized Total Energy     : %.2f J\n", optimizedEnergy);
    printf("Energy Saved               : %.2f J (%.2f%%)\n", energySaved, energySavedPercent);
    printf("\nBaseline Total Time        : %.2f units\n", baselineTime);
    printf("Optimized Total Time       : %.2f units\n", optimizedTime);
    printf("Performance Impact         : %.2f%% time change\n", timeIncreasePercent);

    printf("\nBaseline Avg Waiting Time  : %.2f\n", baselineAvgWaiting);
    printf("Optimized Avg Waiting Time : %.2f\n", optimizedAvgWaiting);
    printf("Baseline Avg Turnaround    : %.2f\n", baselineAvgTurnaround);
    printf("Optimized Avg Turnaround   : %.2f\n", optimizedAvgTurnaround);

    return 0;
}
