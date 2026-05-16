#ifndef METRICS_H
#define METRICS_H

#include "../types.h"

void calculate_metrics(Process *processes, ProcessMetrics *metrics, int count);
void calculate_averages(ScheduleResult *result);

#endif
