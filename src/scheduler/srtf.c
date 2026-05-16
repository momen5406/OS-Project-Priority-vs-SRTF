#include "srtf.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "../metrics/metrics.h"

typedef struct {
    Process process;
    int original_index;
} IndexedProcess;

static int parse_pid_suffix(const char *pid, int *ok) {
    int len = (int)strlen(pid);
    int i = len - 1;
    while (i >= 0 && isdigit((unsigned char)pid[i])) {
        i--;
    }
    if (i == len - 1) {
        *ok = 0;
        return 0;
    }
    *ok = 1;
    return atoi(pid + i + 1);
}

static int compare_pid(const char *a, const char *b) {
    int a_ok = 0;
    int b_ok = 0;
    int a_num = parse_pid_suffix(a, &a_ok);
    int b_num = parse_pid_suffix(b, &b_ok);
    if (a_ok && b_ok && a_num != b_num) {
        return a_num - b_num;
    }
    return strcmp(a, b);
}

static int sort_by_arrival_then_pid(const void *left, const void *right) {
    const IndexedProcess *a = (const IndexedProcess *)left;
    const IndexedProcess *b = (const IndexedProcess *)right;
    if (a->process.arrival_time != b->process.arrival_time) {
        return a->process.arrival_time - b->process.arrival_time;
    }
    return compare_pid(a->process.pid, b->process.pid);
}

static void add_block(ScheduleResult *result, const char *pid, int time) {
    if (result->block_count > 0) {
        GanttBlock *last = &result->blocks[result->block_count - 1];
        if (strcmp(last->pid, pid) == 0 && last->end == time) {
            last->end++;
            return;
        }
    }

    if (result->block_count >= result->block_capacity) {
        int new_capacity = result->block_capacity == 0 ? 16 : result->block_capacity * 2;
        GanttBlock *new_blocks = realloc(result->blocks, sizeof(GanttBlock) * new_capacity);
        if (!new_blocks) {
            return;
        }
        result->blocks = new_blocks;
        result->block_capacity = new_capacity;
    }

    GanttBlock *block = &result->blocks[result->block_count++];
    strncpy(block->pid, pid, MAX_PID_LEN);
    block->pid[MAX_PID_LEN] = '\0';
    block->start = time;
    block->end = time + 1;
}

ScheduleResult run_srtf(Process *processes, int count) {
    ScheduleResult result = {0};
    result.count = count;
    result.processes = calloc((size_t)count, sizeof(Process));
    result.metrics = calloc((size_t)count, sizeof(ProcessMetrics));
    IndexedProcess *ordered = calloc((size_t)count, sizeof(IndexedProcess));
    int *remaining = calloc((size_t)count, sizeof(int));

    for (int i = 0; i < count; i++) {
        result.processes[i] = processes[i];
        result.metrics[i].first_start_time = -1;
        ordered[i].process = processes[i];
        ordered[i].original_index = i;
        remaining[i] = processes[i].burst_time;
    }

    qsort(ordered, (size_t)count, sizeof(IndexedProcess), sort_by_arrival_then_pid);

    int done = 0;
    int time = 0;
    int current = -1;

    while (done < count) {
        int chosen = -1;
        for (int i = 0; i < count; i++) {
            int idx = ordered[i].original_index;
            if (result.processes[idx].arrival_time <= time && remaining[idx] > 0) {
                if (chosen == -1) {
                    chosen = idx;
                    continue;
                }
                if (remaining[idx] < remaining[chosen]) {
                    chosen = idx;
                } else if (remaining[idx] == remaining[chosen]) {
                    /* Keep current process on exact ties to avoid needless context switches. */
                    if (chosen == current) {
                        continue;
                    }
                    if (idx == current) {
                        chosen = idx;
                    } else if (result.processes[idx].arrival_time < result.processes[chosen].arrival_time) {
                        chosen = idx;
                    } else if (result.processes[idx].arrival_time == result.processes[chosen].arrival_time &&
                               compare_pid(result.processes[idx].pid, result.processes[chosen].pid) < 0) {
                        chosen = idx;
                    }
                }
            }
        }

        if (chosen == -1) {
            time++;
            continue;
        }

        if (result.metrics[chosen].first_start_time == -1) {
            result.metrics[chosen].first_start_time = time;
        }

        add_block(&result, result.processes[chosen].pid, time);
        remaining[chosen]--;
        current = chosen;
        time++;

        if (remaining[chosen] == 0) {
            result.metrics[chosen].completion_time = time;
            done++;
        }
    }

    calculate_metrics(result.processes, result.metrics, count);
    calculate_averages(&result);

    free(ordered);
    free(remaining);
    return result;
}
