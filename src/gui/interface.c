#include "interface.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../scheduler/priority.h"
#include "../scheduler/srtf.h"
#include "../util/input.h"
#include "gantt.h"

typedef struct {
    GtkWidget *pid_entry;
    GtkWidget *arrival_entry;
    GtkWidget *burst_entry;
    GtkWidget *priority_entry;
} ProcessRowWidgets;

typedef struct {
    GtkWidget *window;
    GtkWidget *count_entry;
    GtkWidget *form_grid;
    GtkWidget *priority_gantt_area;
    GtkWidget *srtf_gantt_area;
    GtkWidget *priority_table;
    GtkWidget *srtf_table;
    GtkWidget *comparison_label;
    GtkWidget *conclusion_label;
    ProcessRowWidgets *rows;
    int process_count;
    ScheduleResult priority_result;
    ScheduleResult srtf_result;
    int has_results;
} AppState;

static void free_schedule_result(ScheduleResult *result) {
    free(result->processes);
    free(result->metrics);
    free(result->blocks);
    memset(result, 0, sizeof(*result));
}

static void show_error_dialog(GtkWindow *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static GtkListStore *create_results_store(void) {
    return gtk_list_store_new(7, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
}

static GtkWidget *create_results_table(void) {
    GtkListStore *store = create_results_store();
    GtkWidget *view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    const char *columns[] = {"PID", "AT", "BT", "Priority", "WT", "TAT", "RT"};
    for (int i = 0; i < 7; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(columns[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
    }
    return view;
}

static void clear_form_grid(AppState *app) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->form_grid));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    free(app->rows);
    app->rows = NULL;
    app->process_count = 0;
}

static void build_form_rows(AppState *app, int count) {
    clear_form_grid(app);

    app->rows = calloc((size_t)count, sizeof(ProcessRowWidgets));
    app->process_count = count;

    const char *headers[] = {"PID", "Arrival Time", "Burst Time", "Priority"};
    for (int col = 0; col < 4; col++) {
        GtkWidget *label = gtk_label_new(headers[col]);
        gtk_grid_attach(GTK_GRID(app->form_grid), label, col, 0, 1, 1);
    }

    for (int i = 0; i < count; i++) {
        char placeholder[16];
        snprintf(placeholder, sizeof(placeholder), "P%d", i + 1);

        app->rows[i].pid_entry = gtk_entry_new();
        app->rows[i].arrival_entry = gtk_entry_new();
        app->rows[i].burst_entry = gtk_entry_new();
        app->rows[i].priority_entry = gtk_entry_new();

        gtk_entry_set_placeholder_text(GTK_ENTRY(app->rows[i].pid_entry), placeholder);
        gtk_entry_set_placeholder_text(GTK_ENTRY(app->rows[i].arrival_entry), "0");
        gtk_entry_set_placeholder_text(GTK_ENTRY(app->rows[i].burst_entry), "1");
        gtk_entry_set_placeholder_text(GTK_ENTRY(app->rows[i].priority_entry), "1");

        gtk_grid_attach(GTK_GRID(app->form_grid), app->rows[i].pid_entry, 0, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(app->form_grid), app->rows[i].arrival_entry, 1, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(app->form_grid), app->rows[i].burst_entry, 2, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(app->form_grid), app->rows[i].priority_entry, 3, i + 1, 1, 1);
    }

    gtk_widget_show_all(app->form_grid);
}

static int collect_processes(AppState *app, Process **out_processes) {
    if (app->process_count <= 0) {
        show_error_dialog(GTK_WINDOW(app->window), "Create process rows first.");
        return 0;
    }

    Process *processes = calloc((size_t)app->process_count, sizeof(Process));
    char error[256];

    for (int i = 0; i < app->process_count; i++) {
        const char *pid = gtk_entry_get_text(GTK_ENTRY(app->rows[i].pid_entry));
        const char *arrival = gtk_entry_get_text(GTK_ENTRY(app->rows[i].arrival_entry));
        const char *burst = gtk_entry_get_text(GTK_ENTRY(app->rows[i].burst_entry));
        const char *priority = gtk_entry_get_text(GTK_ENTRY(app->rows[i].priority_entry));

        if (!pid || pid[0] == '\0' || !arrival || arrival[0] == '\0' || !burst || burst[0] == '\0' || !priority || priority[0] == '\0') {
            show_error_dialog(GTK_WINDOW(app->window), "All fields are required.");
            free(processes);
            return 0;
        }

        strncpy(processes[i].pid, pid, MAX_PID_LEN);
        processes[i].pid[MAX_PID_LEN] = '\0';

        if (!parse_int_field(arrival, &processes[i].arrival_time) ||
            !parse_int_field(burst, &processes[i].burst_time) ||
            !parse_int_field(priority, &processes[i].priority)) {
            show_error_dialog(GTK_WINDOW(app->window), "Numeric fields must contain valid integers.");
            free(processes);
            return 0;
        }
    }

    if (!validate_processes(processes, app->process_count, error, (int)sizeof(error))) {
        show_error_dialog(GTK_WINDOW(app->window), error);
        free(processes);
        return 0;
    }

    *out_processes = processes;
    return 1;
}

static void fill_results_table(GtkWidget *table, ScheduleResult *result) {
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(table)));
    gtk_list_store_clear(store);

    GtkTreeIter iter;
    for (int i = 0; i < result->count; i++) {
        char at[16], bt[16], pr[16], wt[16], tat[16], rt[16];
        snprintf(at, sizeof(at), "%d", result->processes[i].arrival_time);
        snprintf(bt, sizeof(bt), "%d", result->processes[i].burst_time);
        snprintf(pr, sizeof(pr), "%d", result->processes[i].priority);
        snprintf(wt, sizeof(wt), "%d", result->metrics[i].waiting_time);
        snprintf(tat, sizeof(tat), "%d", result->metrics[i].turnaround_time);
        snprintf(rt, sizeof(rt), "%d", result->metrics[i].response_time);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, result->processes[i].pid,
                           1, at,
                           2, bt,
                           3, pr,
                           4, wt,
                           5, tat,
                           6, rt,
                           -1);
    }

    char avg_wt[32], avg_tat[32], avg_rt[32];
    snprintf(avg_wt, sizeof(avg_wt), "%.2f", result->avg_waiting_time);
    snprintf(avg_tat, sizeof(avg_tat), "%.2f", result->avg_turnaround_time);
    snprintf(avg_rt, sizeof(avg_rt), "%.2f", result->avg_response_time);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "AVERAGE", 1, "-", 2, "-", 3, "-", 4, avg_wt, 5, avg_tat, 6, avg_rt, -1);
}

static const char *winner_text(double a, double b) {
    if (a < b) return "Priority";
    if (b < a) return "SRTF";
    return "Tie";
}

static void refresh_comparison_and_conclusion(AppState *app) {
    char comparison[512];
    snprintf(comparison, sizeof(comparison),
             "Average Waiting Time: Priority=%.2f | SRTF=%.2f | Winner: %s\n"
             "Average Turnaround Time: Priority=%.2f | SRTF=%.2f | Winner: %s\n"
             "Average Response Time: Priority=%.2f | SRTF=%.2f | Winner: %s",
             app->priority_result.avg_waiting_time, app->srtf_result.avg_waiting_time, winner_text(app->priority_result.avg_waiting_time, app->srtf_result.avg_waiting_time),
             app->priority_result.avg_turnaround_time, app->srtf_result.avg_turnaround_time, winner_text(app->priority_result.avg_turnaround_time, app->srtf_result.avg_turnaround_time),
             app->priority_result.avg_response_time, app->srtf_result.avg_response_time, winner_text(app->priority_result.avg_response_time, app->srtf_result.avg_response_time));
    gtk_label_set_text(GTK_LABEL(app->comparison_label), comparison);

    const char *overall = "Priority";
    int priority_wins = 0;
    int srtf_wins = 0;
    if (app->priority_result.avg_waiting_time < app->srtf_result.avg_waiting_time) priority_wins++; else if (app->srtf_result.avg_waiting_time < app->priority_result.avg_waiting_time) srtf_wins++;
    if (app->priority_result.avg_turnaround_time < app->srtf_result.avg_turnaround_time) priority_wins++; else if (app->srtf_result.avg_turnaround_time < app->priority_result.avg_turnaround_time) srtf_wins++;
    if (app->priority_result.avg_response_time < app->srtf_result.avg_response_time) priority_wins++; else if (app->srtf_result.avg_response_time < app->priority_result.avg_response_time) srtf_wins++;
    if (srtf_wins > priority_wins) overall = "SRTF";
    if (srtf_wins == priority_wins) overall = "Both";

    char conclusion[768];
    snprintf(conclusion, sizeof(conclusion),
             "For this workload, %s provides better overall average performance. "
             "SRTF is usually fairer for short jobs but can delay long processes under sustained short arrivals. "
             "Preemptive Priority gives control to critical tasks but has higher starvation risk for low-priority processes unless aging is added. "
             "Recommended choice for this tested input: %s.",
             overall, overall);
    gtk_label_set_text(GTK_LABEL(app->conclusion_label), conclusion);
}

static void run_simulation_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    AppState *app = (AppState *)user_data;
    Process *processes = NULL;
    if (!collect_processes(app, &processes)) {
        return;
    }

    if (app->has_results) {
        free_schedule_result(&app->priority_result);
        free_schedule_result(&app->srtf_result);
    }

    app->priority_result = run_preemptive_priority(processes, app->process_count);
    app->srtf_result = run_srtf(processes, app->process_count);
    app->has_results = 1;
    free(processes);

    fill_results_table(app->priority_table, &app->priority_result);
    fill_results_table(app->srtf_table, &app->srtf_result);
    gantt_set_schedule(app->priority_gantt_area, &app->priority_result);
    gantt_set_schedule(app->srtf_gantt_area, &app->srtf_result);
    refresh_comparison_and_conclusion(app);
}

static void create_rows_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    AppState *app = (AppState *)user_data;
    const char *count_text = gtk_entry_get_text(GTK_ENTRY(app->count_entry));
    int count = 0;
    if (!parse_int_field(count_text, &count) || count <= 0) {
        show_error_dialog(GTK_WINDOW(app->window), "Number of processes must be a positive integer.");
        return;
    }
    build_form_rows(app, count);
}

static void set_row(ProcessRowWidgets *row, const char *pid, const char *at, const char *bt, const char *pr) {
    gtk_entry_set_text(GTK_ENTRY(row->pid_entry), pid);
    gtk_entry_set_text(GTK_ENTRY(row->arrival_entry), at);
    gtk_entry_set_text(GTK_ENTRY(row->burst_entry), bt);
    gtk_entry_set_text(GTK_ENTRY(row->priority_entry), pr);
}

static void load_scenario(AppState *app, int scenario) {
    if (scenario == 1) {
        gtk_entry_set_text(GTK_ENTRY(app->count_entry), "4");
        build_form_rows(app, 4);
        set_row(&app->rows[0], "P1", "0", "5", "3");
        set_row(&app->rows[1], "P2", "2", "3", "1");
        set_row(&app->rows[2], "P3", "4", "6", "4");
        set_row(&app->rows[3], "P4", "5", "2", "2");
    } else if (scenario == 2) {
        gtk_entry_set_text(GTK_ENTRY(app->count_entry), "2");
        build_form_rows(app, 2);
        set_row(&app->rows[0], "P1", "0", "15", "1");
        set_row(&app->rows[1], "P2", "2", "2", "4");
    } else if (scenario == 3) {
        gtk_entry_set_text(GTK_ENTRY(app->count_entry), "4");
        build_form_rows(app, 4);
        set_row(&app->rows[0], "P1", "0", "2", "4");
        set_row(&app->rows[1], "P2", "1", "8", "1");
        set_row(&app->rows[2], "P3", "2", "8", "1");
        set_row(&app->rows[3], "P4", "3", "8", "1");
    } else if (scenario == 4) {
        gtk_entry_set_text(GTK_ENTRY(app->count_entry), "4");
        build_form_rows(app, 4);
        set_row(&app->rows[0], "P1", "-2", "5", "2");
        set_row(&app->rows[1], "P2", "0", "0", "1");
        set_row(&app->rows[2], "P2", "3", "4", "3");
        set_row(&app->rows[3], "P4", "5", "xyz", "1");
    }
}

static void scenario_a_clicked(GtkButton *button, gpointer user_data) { (void)button; load_scenario((AppState *)user_data, 1); }
static void scenario_b_clicked(GtkButton *button, gpointer user_data) { (void)button; load_scenario((AppState *)user_data, 2); }
static void scenario_c_clicked(GtkButton *button, gpointer user_data) { (void)button; load_scenario((AppState *)user_data, 3); }
static void scenario_d_clicked(GtkButton *button, gpointer user_data) { (void)button; load_scenario((AppState *)user_data, 4); }

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AppState *app = (AppState *)user_data;
    if (app->has_results) {
        free_schedule_result(&app->priority_result);
        free_schedule_result(&app->srtf_result);
    }
    free(app->rows);
    free(app);
    gtk_main_quit();
}

void launch_interface(int argc, char **argv) {
    gtk_init(&argc, &argv);

    AppState *app = calloc(1, sizeof(AppState));
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "CPU Scheduling Simulator - Priority vs SRTF");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1200, 900);

    GtkWidget *root_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(app->window), root_scroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(root_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(root), 12);
    gtk_container_add(GTK_CONTAINER(root_scroll), root);

    GtkWidget *input_frame = gtk_frame_new("Input Panel");
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(input_frame), input_box);
    gtk_box_pack_start(GTK_BOX(root), input_frame, FALSE, FALSE, 0);

    GtkWidget *count_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(input_box), count_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(count_row), gtk_label_new("Number of Processes"), FALSE, FALSE, 0);
    app->count_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->count_entry), "e.g., 4");
    gtk_widget_set_size_request(app->count_entry, 120, -1);
    gtk_box_pack_start(GTK_BOX(count_row), app->count_entry, FALSE, FALSE, 0);

    GtkWidget *create_button = gtk_button_new_with_label("Create Input Rows");
    gtk_box_pack_start(GTK_BOX(count_row), create_button, FALSE, FALSE, 0);

    GtkWidget *preset_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(input_box), preset_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(preset_row), gtk_label_new("Load Presets"), FALSE, FALSE, 0);
    GtkWidget *btn_a = gtk_button_new_with_label("Scenario A");
    GtkWidget *btn_b = gtk_button_new_with_label("Scenario B");
    GtkWidget *btn_c = gtk_button_new_with_label("Scenario C");
    GtkWidget *btn_d = gtk_button_new_with_label("Scenario D (Invalid)");
    gtk_box_pack_start(GTK_BOX(preset_row), btn_a, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(preset_row), btn_b, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(preset_row), btn_c, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(preset_row), btn_d, FALSE, FALSE, 0);

    app->form_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(app->form_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(app->form_grid), 10);

    GtkWidget *form_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(form_scroll, -1, 220);
    gtk_container_add(GTK_CONTAINER(form_scroll), app->form_grid);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(form_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(input_box), form_scroll, FALSE, FALSE, 0);

    GtkWidget *run_button = gtk_button_new_with_label("Run Simulation");
    gtk_box_pack_start(GTK_BOX(input_box), run_button, FALSE, FALSE, 0);

    GtkWidget *priority_gantt_frame = gantt_create_panel("Gantt Chart - Preemptive Priority", &app->priority_gantt_area);
    GtkWidget *srtf_gantt_frame = gantt_create_panel("Gantt Chart - SRTF", &app->srtf_gantt_area);
    gtk_box_pack_start(GTK_BOX(root), priority_gantt_frame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), srtf_gantt_frame, FALSE, FALSE, 0);

    GtkWidget *priority_table_frame = gtk_frame_new("Results Table - Priority");
    GtkWidget *priority_table_scroll = gtk_scrolled_window_new(NULL, NULL);
    app->priority_table = create_results_table();
    gtk_container_add(GTK_CONTAINER(priority_table_scroll), app->priority_table);
    gtk_container_add(GTK_CONTAINER(priority_table_frame), priority_table_scroll);
    gtk_box_pack_start(GTK_BOX(root), priority_table_frame, FALSE, FALSE, 0);

    GtkWidget *srtf_table_frame = gtk_frame_new("Results Table - SRTF");
    GtkWidget *srtf_table_scroll = gtk_scrolled_window_new(NULL, NULL);
    app->srtf_table = create_results_table();
    gtk_container_add(GTK_CONTAINER(srtf_table_scroll), app->srtf_table);
    gtk_container_add(GTK_CONTAINER(srtf_table_frame), srtf_table_scroll);
    gtk_box_pack_start(GTK_BOX(root), srtf_table_frame, FALSE, FALSE, 0);

    GtkWidget *comparison_frame = gtk_frame_new("Comparison Summary");
    app->comparison_label = gtk_label_new("Run simulation to compare average metrics.");
    gtk_label_set_xalign(GTK_LABEL(app->comparison_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(app->comparison_label), TRUE);
    gtk_container_add(GTK_CONTAINER(comparison_frame), app->comparison_label);
    gtk_box_pack_start(GTK_BOX(root), comparison_frame, FALSE, FALSE, 0);

    GtkWidget *conclusion_frame = gtk_frame_new("Conclusion");
    app->conclusion_label = gtk_label_new("Conclusion will be generated after simulation.");
    gtk_label_set_xalign(GTK_LABEL(app->conclusion_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(app->conclusion_label), TRUE);
    gtk_container_add(GTK_CONTAINER(conclusion_frame), app->conclusion_label);
    gtk_box_pack_start(GTK_BOX(root), conclusion_frame, FALSE, FALSE, 0);

    g_signal_connect(app->window, "destroy", G_CALLBACK(on_window_destroy), app);
    g_signal_connect(create_button, "clicked", G_CALLBACK(create_rows_clicked), app);
    g_signal_connect(run_button, "clicked", G_CALLBACK(run_simulation_clicked), app);
    g_signal_connect(btn_a, "clicked", G_CALLBACK(scenario_a_clicked), app);
    g_signal_connect(btn_b, "clicked", G_CALLBACK(scenario_b_clicked), app);
    g_signal_connect(btn_c, "clicked", G_CALLBACK(scenario_c_clicked), app);
    g_signal_connect(btn_d, "clicked", G_CALLBACK(scenario_d_clicked), app);

    gtk_widget_show_all(app->window);
    gtk_main();
}
