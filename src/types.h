#ifndef TYPES_H
#define TYPES_H

#define MAX_PID_LEN 63

typedef struct {
    char pid[MAX_PID_LEN + 1];
    int arrival_time;
    int burst_time;
    int priority;
} Process;

typedef struct {
    char pid[MAX_PID_LEN + 1];
    int start;
    int end;
} GanttBlock;

typedef struct {
    int completion_time;
    int first_start_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
} ProcessMetrics;

typedef struct {
    Process *processes;
    ProcessMetrics *metrics;
    int count;
    GanttBlock *blocks;
    int block_count;
    int block_capacity;
    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
} ScheduleResult;

#endif
