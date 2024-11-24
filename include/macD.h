#ifndef MACD_H
#define MACD_H

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <bits/getopt_core.h>
#include <ctype.h>

/// @brief A struct representing information about a process.
typedef struct
{
    pid_t pid;
    char *program_name;
    int running;
    int was_terminated;
    char **args;
} ProcessInfo;

/// Globals for signal handling
extern volatile sig_atomic_t sigint_received;
extern volatile sig_atomic_t sigabrt_received;

extern ProcessInfo *global_processes;
extern int global_process_count;
extern int global_processes_running;
extern int total_elapsed_time;

/// Function declarations
void handle_sigint(int sig);
void handle_sigabrt(int sig);
void launch_process(ProcessInfo *process, int process_index);
void print_start_message(int process_index, const char *message, ProcessInfo *process);
int parse_config(const char *config_file, int *timelimit, ProcessInfo **processes, int *program_count, int *capacity);
int validate_timelimit_line(const char *line, int *timelimit);
void print_usage_message();
int file_exists(char *file_name);
void get_process_resource_usage(int pid, int *cpu_usage, double *memory_usage);
void print_timestamp(const char *message);
void print_status_report(ProcessInfo *processes, int *process_count, const char *message);
void monitor_processes(ProcessInfo *processes, int *process_count, int *timelimit);
void cleanup_processes(ProcessInfo *processes, int *process_count);

#endif // MACD_H
