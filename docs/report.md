# Energy-Efficient CPU Scheduling Algorithm

## 1. Theory and Documentation

### 1.1 Introduction to CPU Scheduling
CPU scheduling is an operating system function that decides which process gets CPU time and in what order. Since many processes compete for limited CPU resources, a scheduler aims to improve CPU utilization, throughput, and response time.

### 1.2 Types of CPU Scheduling Algorithms

#### FCFS (First Come First Serve)
- Processes are scheduled in arrival order.
- Very simple and fair by arrival sequence.
- Can cause high waiting time for short jobs if a long job comes first.

#### SJF (Shortest Job First)
- Process with smallest burst time is selected first.
- Gives good average waiting time.
- Needs burst-time estimation and may starve longer processes.

#### Round Robin
- Each process gets a fixed time quantum.
- Good for time-sharing and fairness.
- Context switching overhead increases if quantum is too small.

#### Priority Scheduling
- Higher-priority processes run first.
- Useful for real-time/important tasks.
- May starve low-priority tasks without aging.

### 1.3 What is Energy Efficiency in OS
Energy efficiency in operating systems means completing computing work using less power while maintaining acceptable performance. It is important in mobile phones, IoT devices, laptops, and embedded systems where battery and thermal limits are critical.

### 1.4 DVFS (Dynamic Voltage and Frequency Scaling)
DVFS dynamically changes CPU frequency and voltage based on workload demand.
- Lower frequency and voltage reduce power usage significantly.
- Dynamic power roughly follows relation:
  - Power proportional to V^2 * f
- Reducing voltage gives large energy savings.
- Performance can drop if frequency becomes too low.

### 1.5 Thermal-Aware Scheduling
Thermal-aware scheduling prevents overheating by considering current CPU/process temperature.
- If temperature is high, run at lower frequency to reduce heat.
- If temperature is safe and urgent work exists, allow higher frequency.
- Helps avoid thermal throttling and improves hardware reliability.

### 1.6 Problem Statement
Traditional scheduling algorithms focus mainly on performance and fairness, but they do not directly optimize power and temperature. This can increase battery drain and overheating in mobile/embedded systems.

### 1.7 Objectives of the Project
- Design a custom scheduler that is energy-aware and thermal-aware.
- Use DVFS levels to adjust CPU speed and voltage.
- Balance power efficiency with performance and fairness.
- Compare energy consumption before and after optimization.
- Build a C simulation and a web-based demonstration interface.

### 1.8 Proposed Algorithm (High-Level Logic)
The scheduler uses process attributes and thermal state to decide both:
1. Which process to execute next.
2. Which DVFS level (low/medium/high) to use.

Main idea:
- Urgent or short high-priority tasks may run at high frequency for responsiveness.
- If thermal state is high, scheduler reduces frequency.
- Long or less urgent tasks are shifted to lower/medium frequency to save energy.
- A fairness term (aging) increases score of waiting tasks to prevent starvation.

### 1.9 Advantages and Limitations

Advantages:
- Reduces total energy consumption.
- Controls thermal rise and overheating risk.
- Keeps responsiveness for high-priority short jobs.
- Demonstrates practical OS power-management concept.

Limitations:
- Uses simplified energy and thermal model (simulation-level).
- Real hardware behavior may differ.
- More complex than basic FCFS/RR implementation.

---

## 2. Algorithm Design

### 2.1 Input Parameters
For each process Pi:
- Burst time (CPU time required)
- Priority (1 = highest priority)
- Temperature indicator (current thermal value associated with task/system)
- Base power usage (watts at nominal condition)

Global parameters:
- DVFS levels (frequency, voltage pairs)
- Thermal threshold values
- Aging factor for fairness

### 2.2 Decision Logic for Frequency and Voltage
Three DVFS levels:
- High: f = 2.5 GHz, V = 1.20 V
- Medium: f = 1.8 GHz, V = 1.00 V
- Low: f = 1.2 GHz, V = 0.90 V

Rules:
- If temperature is very high (>= 80 C): choose Low.
- Else if process is urgent (priority <= 2 or burst <= 4): choose High.
- Else if temperature is moderately high (>= 70 C): choose Medium.
- Else if burst is long (> 10): choose Low.
- Otherwise: choose Medium.

### 2.3 Scheduling Strategy
At each selection step:
- Compute score for each unscheduled process:
  - score = priority term + short-job term + aging term - hot penalty
- Select process with maximum score.
- Apply DVFS decision rules.
- Compute execution time, energy usage, and update thermal estimate.

Fairness:
- Aging term increases with waiting time index.
- Prevents low-priority starvation.

### 2.4 Pseudocode

```text
Input N processes with (id, burst, priority, temp, basePower)
Define DVFS levels: HIGH, MEDIUM, LOW with (f, V)
Set baseline_f = 2.5 GHz, baseline_V = 1.2 V

for each process i:
    baselineExec[i] = burst[i] * (baseline_f / baseline_f)
    baselinePower[i] = basePower[i] * (baseline_V^2 / baseline_V^2) * (baseline_f / baseline_f)
    baselineEnergy += baselinePower[i] * baselineExec[i]

unscheduled = all processes
order = 0
while unscheduled not empty:
    best = argmax score(i) over unscheduled
        where score(i) = priorityWeight(i) + shortJobBonus(i) + aging(order) - thermalPenalty(i)

    dvfs = chooseDVFS(process[best].temp, process[best].priority, process[best].burst)
    execTime = burst[best] * (baseline_f / dvfs.f)
    dynPower = basePower[best] * (dvfs.V^2 / baseline_V^2) * (dvfs.f / baseline_f)
    energy = dynPower * execTime

    save schedule record(best, dvfs, execTime, dynPower, energy)
    optimizedEnergy += energy

    update process/system temperature estimate
    mark best as scheduled
    order++

Compute:
- Total execution time baseline and optimized
- Average waiting/turnaround estimates
- Energy saved percentage
Output schedule table and comparison summary
```

---

## 3. C Language Implementation

The complete C implementation is provided in:
- `c-src/energy_scheduler.c`

It supports:
- User input for all process data
- Baseline scheduling and optimized scheduling comparison
- DVFS-level selection logic
- Energy calculation and reporting
- Clear, beginner-friendly comments

---

## 4. Output Explanation

### 4.1 Sample Input

```text
Number of processes: 4
P1: burst=8 priority=3 temp=65 power=22
P2: burst=4 priority=1 temp=60 power=26
P3: burst=12 priority=4 temp=75 power=20
P4: burst=5 priority=2 temp=68 power=24
```

### 4.2 Sample Output (Summary)

```text
Baseline total energy: 640.00 Joules
Optimized total energy: 512.89 Joules
Energy saved: 19.86%
Scheduling order (optimized): P2 -> P4 -> P1 -> P3
```

### 4.3 Explanation
- P2 and P4 are urgent (high priority/short burst), so they get high or medium frequency early.
- P3 has long burst and higher temperature, so lower frequency is used to save energy and reduce heat.
- As a result, optimized scheduling gives meaningful energy savings with manageable performance impact.

### 4.4 Comparison with Normal Scheduling
Normal scheduling (baseline FCFS, fixed high frequency):
- Better raw speed in some cases.
- Higher energy usage.
- More heat generation.

Proposed energy-aware scheduling:
- Slight performance trade-off for non-urgent tasks.
- Better battery life and thermal control.
- More suitable for portable and embedded devices.

---

## 5. Website UI (Frontend)

The web demo is in:
- `web/index.html`
- `web/styles.css`
- `web/script.js`

Features:
- Input form for process details
- Add/remove process rows
- Run scheduling button
- Result table for order, DVFS level, execution time, and energy
- Comparison chart (Energy and Performance)

---

## 6. Project Integration (C + UI)

### 6.1 Integration Approaches

1. Node.js API + C executable:
- Build C code to executable.
- Node backend receives JSON input from UI.
- Backend runs executable (child process), passes input, reads output, returns JSON.

2. CGI approach:
- Web server executes compiled C CGI binary.
- Form input mapped to request parameters.
- C generates HTML/JSON response.

3. Re-implementation API approach:
- Keep C version for system simulation.
- Build same logic in backend language (Node/Python) for easy API maintenance.

### 6.2 Recommended for Mini Project
Use Node.js + Express + child_process for easiest beginner-friendly integration:
- Clean frontend/backend separation.
- Easy deployment and debugging.
- Keeps original C logic usable.

---

## 7. Bonus Content

### 7.1 Comparison Graph (Energy vs Performance)
The web UI includes a bar chart showing:
- Baseline vs optimized energy
- Baseline vs optimized execution time

### 7.2 Future Improvements
- Add machine-learning based DVFS prediction.
- Include real CPU temperature sensors (hardware integration).
- Add multi-core scheduling and task migration.
- Add battery state-of-charge aware scheduling.
- Evaluate with real benchmarks and trace-driven simulation.

---

## Conclusion
This project presents a practical and beginner-friendly energy-efficient scheduler design combining DVFS and thermal-aware scheduling. It demonstrates how operating systems can reduce energy consumption while maintaining fairness and acceptable performance.
