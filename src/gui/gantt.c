#include "gantt.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    const ScheduleResult *result;
} GanttContext;

static double hue_to_rgb(double p, double q, double t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

static void pid_to_color(const char *pid, double *r, double *g, double *b) {
    unsigned int hash = 2166136261u;
    for (const unsigned char *p = (const unsigned char *)pid; *p; p++) {
        hash ^= *p;
        hash *= 16777619u;
    }
    double h = (hash % 360u) / 360.0;
    double s = 0.65;
    double l = 0.58;
    double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;
    double p = 2.0 * l - q;
    *r = hue_to_rgb(p, q, h + 1.0 / 3.0);
    *g = hue_to_rgb(p, q, h);
    *b = hue_to_rgb(p, q, h - 1.0 / 3.0);
}

static gboolean on_gantt_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)user_data;
    GanttContext *ctx = g_object_get_data(G_OBJECT(widget), "gantt_ctx");
    if (!ctx || !ctx->result || ctx->result->block_count == 0) {
        cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 14.0);
        cairo_move_to(cr, 20.0, 40.0);
        cairo_show_text(cr, "No data yet. Run simulation.");
        return FALSE;
    }

    const int margin_left = 30;
    const int margin_top = 20;
    const int row_height = 50;
    const int block_unit_width = 40;

    for (int i = 0; i < ctx->result->block_count; i++) {
        GanttBlock block = ctx->result->blocks[i];
        double r = 0.2;
        double g = 0.5;
        double b = 0.8;
        pid_to_color(block.pid, &r, &g, &b);

        int x = margin_left + block.start * block_unit_width;
        int y = margin_top;
        int width = (block.end - block.start) * block_unit_width;

        cairo_set_source_rgb(cr, r, g, b);
        cairo_rectangle(cr, x, y, width, row_height);
        cairo_fill_preserve(cr);

        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_set_line_width(cr, 1.2);
        cairo_stroke(cr);

        cairo_set_source_rgb(cr, 0.05, 0.05, 0.05);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 13.0);
        cairo_move_to(cr, x + 8.0, y + 30.0);
        cairo_show_text(cr, block.pid);

        cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        char start_label[16];
        snprintf(start_label, sizeof(start_label), "%d", block.start);
        cairo_move_to(cr, x - 3.0, y + row_height + 18.0);
        cairo_show_text(cr, start_label);

        char end_label[16];
        snprintf(end_label, sizeof(end_label), "%d", block.end);
        cairo_move_to(cr, x + width - 3.0, y + row_height + 18.0);
        cairo_show_text(cr, end_label);
    }

    return FALSE;
}

GtkWidget *gantt_create_panel(const char *title, GtkWidget **drawing_area_out) {
    GtkWidget *frame = gtk_frame_new(title);
    GtkWidget *area = gtk_drawing_area_new();
    gtk_widget_set_size_request(area, 900, 110);
    gtk_container_add(GTK_CONTAINER(frame), area);

    GanttContext *ctx = g_new0(GanttContext, 1);
    g_object_set_data_full(G_OBJECT(area), "gantt_ctx", ctx, g_free);
    g_signal_connect(area, "draw", G_CALLBACK(on_gantt_draw), NULL);

    if (drawing_area_out) {
        *drawing_area_out = area;
    }
    return frame;
}

void gantt_set_schedule(GtkWidget *drawing_area, const ScheduleResult *result) {
    GanttContext *ctx = g_object_get_data(G_OBJECT(drawing_area), "gantt_ctx");
    if (!ctx) {
        return;
    }
    ctx->result = result;
    int max_time = 10;
    if (result && result->block_count > 0) {
        max_time = result->blocks[result->block_count - 1].end + 1;
    }
    gtk_widget_set_size_request(drawing_area, max_time * 42 + 80, 110);
    gtk_widget_queue_draw(drawing_area);
}
