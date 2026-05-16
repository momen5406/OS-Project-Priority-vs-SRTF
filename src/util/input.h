#ifndef INPUT_H
#define INPUT_H

#include "../types.h"

int parse_int_field(const char *text, int *value_out);
int validate_processes(Process *processes, int count, char *error_buffer, int error_size);

#endif
