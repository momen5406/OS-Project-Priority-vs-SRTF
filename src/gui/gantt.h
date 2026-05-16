#ifndef GANTT_H
#define GANTT_H

#include <gtk/gtk.h>

#include "../types.h"

GtkWidget *gantt_create_panel(const char *title, GtkWidget **drawing_area_out);
void gantt_set_schedule(GtkWidget *drawing_area, const ScheduleResult *result);

#endif
