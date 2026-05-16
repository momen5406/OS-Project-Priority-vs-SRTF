# CPU Scheduler Simulator

A C-based project on Cpu Scheduling Algorithms: Priority - SRTF

## Project Overview

This project implements multiple CPU scheduling algorithms commonly studied in Operating Systems courses. It includes a GUI interface to visualize process scheduling, generate Gantt charts, and calculate key performance metrics.

## Features

- **Multiple Scheduling Algorithms**
  - Shortest Remaining Time First (SRTF)
  - Priority-based Scheduling

- **GUI Interface**
  - Interactive process input
  - Real-time visualization
  - Gantt chart generation

- **Performance Metrics**
  - Waiting Time
  - Turnaround Time
  - Response Time
  - Average calculations for all metrics

## Project Structure

```text
OS-Project/
├── src/
│   ├── main.c                 # Entry point
│   ├── types.h                # Core data structures
│   ├── Makefile               # Build configuration
│   ├── scheduler/             # Scheduling algorithms
│   │   ├── srtf.c            # Shortest Remaining Time First
│   │   └── priority.c        # Priority-based scheduling
│   ├── metrics/               # Performance metrics calculation
│   │   └── metrics.c
│   ├── gui/                   # User interface
│   │   ├── interface.c
│   │   └── gantt.c           # Gantt chart visualization
│   ├── util/                  # Utility functions
│   │   └── input.c           # Input handling
│   ├── cpu_scheduler_sim      # Compiled executable
│   └── ...
├── Documentation - Test Cases # Test documentation
├── Screenshots/               # Project screenshots
└── README.md                  # Original documentation
```

## Data Structures

### Process

```c
typedef struct {
    char pid[MAX_PID_LEN + 1];
    int arrival_time;
    int burst_time;
    int priority;
} Process;
```

### ProcessMetrics

```c
typedef struct {
    int completion_time;
    int first_start_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
} ProcessMetrics;
```

### ScheduleResult

Aggregates processes, their metrics, Gantt blocks, and average performance metrics.

## Scheduling Algorithms

### Shortest Remaining Time First (SRTF)

Preemptive scheduling algorithm that selects the process with the shortest remaining burst time. This minimizes average waiting time.

**File:** `src/scheduler/srtf.c`

### Priority Scheduling

Scheduling based on process priority values. Higher or lower priority values determine execution order depending on implementation.

**File:** `src/scheduler/priority.c`

## Building the Project

### Prerequisites

- GCC compiler
- Make utility
- Standard C library
- Code must run on Linux not Windows

### Compilation

```bash
cd src
make
```

This will compile all source files and generate the `cpu_scheduler_sim` executable.

### Running the Application

```bash
./cpu_scheduler_sim
```

The GUI interface will launch, allowing you to:

1. Input process information (PID, Arrival Time, Burst Time, Priority)
2. Select a scheduling algorithm
3. View the Gantt chart
4. Display performance metrics

## Usage Guide

1. **Launch the Application**

   ```bash
   ./cpu_scheduler_sim
   ```

2. **Input Processes**
   - Enter process details when prompted
   - Specify arrival time, burst time, and priority

3. **View Results**
   - Gantt chart shows process execution timeline
   - Metrics display waiting time, turnaround time, and response time
   - Average metrics calculated across all processes

## Performance Metrics Explained

- **Waiting Time:** Total time a process spends waiting in the queue
- **Turnaround Time:** Total time from arrival to completion (Waiting Time + Burst Time)
- **Response Time:** Time from arrival to first execution
- **Completion Time:** Absolute time when process finishes execution

## Testing

Test cases and documentation are available in the `Documentation - Test Cases` folder, which includes:

- Test scenarios for each algorithm
- Expected outputs
- Edge cases and validation

## Screenshots

Screenshots demonstrating the GUI and results are available in the `Screenshots/` folder.

## Technical Details

- **Language:** C
- **Platform:** Linux
- **Build System:** Make
- **GUI Framework:** gtk
