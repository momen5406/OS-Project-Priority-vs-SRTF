#include "input.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_int_field(const char *text, int *value_out) {
    if (!text || *text == '\0') {
        return 0;
    }

    for (const char *p = text; *p; p++) {
        if (!(isdigit((unsigned char)*p) || (*p == '-' && p == text))) {
            return 0;
        }
    }

    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (*end != '\0') {
        return 0;
    }

    *value_out = (int)value;
    return 1;
}

int validate_processes(Process *processes, int count, char *error_buffer, int error_size) {
    for (int i = 0; i < count; i++) {
        if (processes[i].pid[0] == '\0') {
            snprintf(error_buffer, (size_t)error_size, "Process %d: PID is required.", i + 1);
            return 0;
        }
        if (processes[i].arrival_time < 0) {
            snprintf(error_buffer, (size_t)error_size, "Process %s: arrival time cannot be negative.", processes[i].pid);
            return 0;
        }
        if (processes[i].burst_time <= 0) {
            snprintf(error_buffer, (size_t)error_size, "Process %s: burst time must be positive.", processes[i].pid);
            return 0;
        }
        if (processes[i].priority <= 0) {
            snprintf(error_buffer, (size_t)error_size, "Process %s: priority must be positive.", processes[i].pid);
            return 0;
        }
    }

    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(processes[i].pid, processes[j].pid) == 0) {
                snprintf(error_buffer, (size_t)error_size, "Duplicate PID detected: %s", processes[i].pid);
                return 0;
            }
        }
    }

    return 1;
}
