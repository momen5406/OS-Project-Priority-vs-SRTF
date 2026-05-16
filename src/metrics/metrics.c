#include "metrics.h"

void calculate_metrics(Process *processes, ProcessMetrics *metrics, int count) {
    for (int i = 0; i < count; i++) {
        metrics[i].turnaround_time = metrics[i].completion_time - processes[i].arrival_time;
        metrics[i].waiting_time = metrics[i].turnaround_time - processes[i].burst_time;
        metrics[i].response_time = metrics[i].first_start_time - processes[i].arrival_time;
    }
}

void calculate_averages(ScheduleResult *result) {
    if (result->count == 0) {
        result->avg_waiting_time = 0.0;
        result->avg_turnaround_time = 0.0;
        result->avg_response_time = 0.0;
        return;
    }

    double wt_sum = 0.0;
    double tat_sum = 0.0;
    double rt_sum = 0.0;

    for (int i = 0; i < result->count; i++) {
        wt_sum += result->metrics[i].waiting_time;
        tat_sum += result->metrics[i].turnaround_time;
        rt_sum += result->metrics[i].response_time;
    }

    result->avg_waiting_time = wt_sum / result->count;
    result->avg_turnaround_time = tat_sum / result->count;
    result->avg_response_time = rt_sum / result->count;
}
