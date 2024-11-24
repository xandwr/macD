/*  macD.c
    Alexander Pickering (3118504) -- Last Updated 2024/11/22

    Implements a basic process monitor in C.

    The list of processes to be executed can be provided by running macD with the `-i` flag:
    ./macD -i config.conf

    The first line of the config file denotes the maximum time the monitor can run for:
    timelimit 20    // equals 20 seconds

    And the following lines thereafter are a list of executable paths to be ran,
    with arguments provided (separated by spaces) after on the same line:
    /programs/build/pi_n 100
*/

#include "../include/macD.h"

// Define global externs
volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigabrt_received = 0;

ProcessInfo *global_processes = NULL;
int global_process_count = 0;
int global_processes_running = 0;
int total_elapsed_time = 0;

/// @brief Handles the `SIGINT` signal and terminates the program cleanly.
/// @param sig The `SIGINT` signal value.
void handle_sigint(int sig)
{
    if (global_processes == NULL)
    {
        return;
    }

    sigint_received = 1;
}

/// @brief Handles the `SIGABRT` signal and terminates the program cleanly.
/// @param sig The `SIGABRT` signal value.
void handle_sigabrt(int sig)
{
    if (global_processes == NULL)
    {
        return;
    }

    sigabrt_received = 1;
}

/// @brief ### Launches a given program as a child process.
/// @param program_path The path to the program executable.
/// @param timelimit The maximum time limit this process is allowed to run for.
void launch_process(ProcessInfo *process, int process_index)
{
    struct stat buffer;
    if (stat(process->program_name, &buffer) != 0)
    {
        print_start_message(process_index, "failed to start", process);
        process->running = 0;
        return;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    { // Child process logic
      // printf("Launching process %s with args %s\n", process->program_name, *process->args);
        extern char **environ;
        execve(process->program_name, process->args, environ);

        exit(EXIT_FAILURE);
    }
    else
    { // Parent process logic
        global_processes_running++;
        process->pid = pid;
        process->running = 1;

        int fs_buffer_size = snprintf(NULL, 0, "started successfully (pid: %d)", process->pid) + 1;
        char *formatted_string = malloc(fs_buffer_size);
        if (!formatted_string)
        {
            perror("Failed to allocate memory for formatted_string");
            exit(EXIT_FAILURE);
        }

        sprintf(formatted_string, "started successfully (pid: %d)", process->pid);
        print_start_message(process_index, formatted_string, process);
        free(formatted_string);
    }
}

/// @brief Prints a process start message to `stdout`.
/// @param process_index The index of the process in the global processes list.
/// @param message A string to place in the middle of the message.
/// @param process The relevant `ProcessInfo`.
void print_start_message(int process_index, const char *message, ProcessInfo *process)
{
    // Calculates the required buffer size for all arguments
    size_t args_string_length = 0;
    for (int i = 1; process->args[i] != NULL; i++)
    {
        args_string_length += strlen(process->args[i]) + 1; // +1 for spaces/null terminator
    }

    // Allocates buffer for concatenated args
    char *args_string = malloc(args_string_length + 1); // +1 for null terminator
    if (!args_string)
    {
        perror("Failed to allocate memory for args_string");
        exit(EXIT_FAILURE);
    }
    args_string[0] = '\0';

    // Builds the args string
    for (int i = 1; process->args[i] != NULL; i++)
    {
        if (i > 1)
        {
            strcat(args_string, " ");
        }
        strcat(args_string, process->args[i]);
    }

    // Prints the start message
    fprintf(stdout, "[%d] %s %s, %s\n", process_index, process->program_name, args_string, message);

    free(args_string);
}

/// @brief ### Reads the given config file and populates the timelimit and program paths.
/// @param config_file The path to the config file.
/// @param timelimit A pointer to store the time limit in.
/// @param programs An array to store program paths in.
/// @return `0` if successful.
int parse_config(const char *config_file, int *timelimit, ProcessInfo **processes, int *program_count, int *capacity)
{
    FILE *file = fopen(config_file, "r");
    if (!file)
    {
        perror("Failed to open configuration file");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t line_size = 0;

    if (getline(&line, &line_size, file) != -1)
    {
        line[strcspn(line, "\n")] = 0;

        if (validate_timelimit_line(line, timelimit) != 0)
        {
            free(line);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf(stderr, "Error: Failed to read timelimit line or file is empty.\n");
        free(line);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    while (getline(&line, &line_size, file) != -1)
    {
        // Removes newline character
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "timelimit ", 10) == 0)
        {
            sscanf(line, "timelimit %d", timelimit);
        }
        else if (strspn(line, " \t\r\n") != strlen(line)) // <- Checks if the line isn't empty
        {
            char *program_path = strtok(line, " ");
            char *arg;

            // Ensures the processes array has enough capacity
            if (*program_count >= *capacity)
            {
                *capacity *= 2;
                ProcessInfo *new_processes = realloc(*processes, *capacity * sizeof(ProcessInfo));
                if (!new_processes)
                {
                    perror("Failed to realloc memory for processes array");
                    exit(EXIT_FAILURE);
                }
                *processes = new_processes;
            }

            ProcessInfo *current_process = &(*processes)[*program_count];
            current_process->program_name = malloc(strlen(program_path) + 1);
            if (!current_process->program_name)
            {
                perror("Failed to allocate memory for program_name");
                exit(EXIT_FAILURE);
            }

            strcpy(current_process->program_name, program_path);

            int arg_index = 1;
            size_t args_capacity = ARG_MAX;

            // Allocates memory to the args[] array in the ProcessInfo (max args = LINK_MAX)
            current_process->args = malloc(args_capacity * sizeof(char *));
            if (!current_process->args)
            {
                perror("Failed to allocate memory for args array");
                exit(EXIT_FAILURE);
            }

            current_process->args[0] = strdup(current_process->program_name);
            if (!current_process->args[0])
            {
                perror("Failed to allocate memory for program name");
                exit(EXIT_FAILURE);
            }

            while ((arg = strtok(NULL, " ")) != NULL)
            {
                if (arg_index >= args_capacity)
                {
                    args_capacity *= 2;
                    char **new_args = realloc(current_process->args, args_capacity * sizeof(char *));
                    if (!new_args)
                    {
                        perror("Failed to realloc memory for args array");
                        exit(EXIT_FAILURE);
                    }
                    current_process->args = new_args;
                }
                size_t len = strlen(arg);
                while (len > 0 && isspace(arg[len - 1]))
                {
                    arg[--len] = '\0';
                }
                while (isspace(*arg))
                {
                    arg++;
                }
                current_process->args[arg_index] = malloc(strlen(arg) + 1);
                if (!current_process->args[arg_index])
                {
                    perror("Failed to allocate memory for argument");
                    exit(EXIT_FAILURE);
                }
                strcpy(current_process->args[arg_index], arg);
                arg_index++;
            }
            current_process->args[arg_index] = NULL; // Null-terminates args array

            (*program_count)++;

            if (*program_count >= LINK_MAX)
            {
                fprintf(stderr, "Too many programs in configuration file. Max allowed is %d.\n", LINK_MAX);
                break;
            }
        }
    }

    free(line);
    fclose(file);
    return 0;
}

/// @brief Validates the format of the timelimit line in the config.
/// @param line The line to check.
/// @param timelimit A pointer to a timelimit value.
/// @return
int validate_timelimit_line(const char *line, int *timelimit)
{
    if (strncmp(line, "timelimit ", 10) != 0)
    {
        fprintf(stderr, "Error: Invalid timelimit line format. Expected 'timelimit <number>'.\n");
        return -1;
    }

    // Extract the timelimit value
    char *endptr;
    long parsed_timelimit = strtol(line + 10, &endptr, 10);

    // Ensure the value is numeric and valid
    if (*endptr != '\0' || parsed_timelimit <= 0)
    {
        fprintf(stderr, "Error: Invalid or missing timelimit value. Must be a positive integer.\n");
        return -1;
    }

    *timelimit = (int)parsed_timelimit;
    return 0;
}

/// @brief ### Prints a program helper message to stdout.
void print_usage_message()
{
    fprintf(stdout, "Usage: ./macD -i [config file]\n");
}

/// @brief ### Checks if a given file exists in the local path.
/// @param file_name The name of the file to check for.
/// @return `0` if the file is found, `1` if not.
int file_exists(char *file_name)
{
    if (access(file_name, F_OK) == 0)
    {
        return 0;
    }
    return 1;
}

/// @brief Populates `*cpu_usage` and `*memory_usage` pointers with the current resource usage values of process `pid`.
/// @param pid The pid of the process to check.
/// @param cpu_usage A pointer to store the CPU utilization value in.
/// @param memory_usage A pointer to store the memory usage value in.
void get_process_resource_usage(int pid, int *cpu_usage, double *memory_usage)
{
    *cpu_usage = 0;
    *memory_usage = 0.0;

    // Allocates memory dynamically for file paths
    size_t path_size = snprintf(NULL, 0, "/proc/%d/stat", pid) + 1;
    char *stat_path = malloc(path_size);
    if (!stat_path)
    {
        perror("Failed to allocate memory for stat_path");
        return;
    }
    snprintf(stat_path, path_size, "/proc/%d/stat", pid);

    path_size = snprintf(NULL, 0, "/proc/%d/statm", pid) + 1;
    char *statm_path = malloc(path_size);
    if (!statm_path)
    {
        perror("Failed to allocate memory for statm_path");
        free(stat_path);
        return;
    }
    snprintf(statm_path, path_size, "/proc/%d/statm", pid);

    // Checks if the process exists
    if (kill(pid, 0) != 0)
    {
        free(stat_path);
        free(statm_path);
        return;
    }

    // Opens /proc/[pid]/stat for CPU usage
    FILE *stat_file = fopen(stat_path, "r");
    if (!stat_file)
    {
        fprintf(stderr, "Warning: Unable to open %s (process may have terminated).\n", stat_path);
        free(stat_path);
        free(statm_path);
        return;
    }

    size_t line_size = 1024;
    char *line = malloc(line_size);
    if (!line)
    {
        perror("Failed to allocate memory for line buffer");
        fclose(stat_file);
        free(stat_path);
        free(statm_path);
        return;
    }

    if (fgets(line, line_size, stat_file) != NULL)
    {
        // Parses fields using dynamic allocation
        char **fields = malloc(LINK_MAX * sizeof(char *));
        if (!fields)
        {
            perror("Failed to allocate memory for fields array");
            fclose(stat_file);
            free(line);
            free(stat_path);
            free(statm_path);
            return;
        }

        int field_index = 0;
        char *token = strtok(line, " ");
        while (token != NULL)
        {
            fields[field_index] = malloc(strlen(token) + 1);
            if (!fields[field_index])
            {
                perror("Failed to allocate memory for field token");
                for (int i = 0; i < field_index; i++)
                {
                    free(fields[i]);
                }
                free(fields);
                fclose(stat_file);
                free(line);
                free(stat_path);
                free(statm_path);
                return;
            }
            strcpy(fields[field_index++], token);
            token = strtok(NULL, " ");
        }

        unsigned long utime = strtoul(fields[13], NULL, 10);
        unsigned long stime = strtoul(fields[14], NULL, 10);
        unsigned long starttime = strtoul(fields[21], NULL, 10);
        unsigned long total_time = utime + stime;

        long clock_ticks_per_sec = sysconf(_SC_CLK_TCK);

        double uptime;
        FILE *uptime_file = fopen("/proc/uptime", "r");
        if (uptime_file)
        {
            fscanf(uptime_file, "%lf", &uptime);
            fclose(uptime_file);
        }
        else
        {
            fprintf(stderr, "Warning: Unable to read /proc/uptime.\n");
        }

        unsigned long long elapsed_time = (unsigned long long)(uptime * clock_ticks_per_sec) - starttime;

        if (elapsed_time > 0)
        {
            *cpu_usage = (total_time * 100) / elapsed_time;
        }

        // Frees allocated memory for fields
        for (int i = 0; i < field_index; i++)
        {
            free(fields[i]);
        }
        free(fields);
    }

    fclose(stat_file);

    // Opens /proc/[pid]/statm for memory usage
    FILE *statm_file = fopen(statm_path, "r");
    if (!statm_file)
    {
        fprintf(stderr, "Warning: Unable to open %s (process may have terminated).\n", statm_path);
        free(line);
        free(stat_path);
        free(statm_path);
        return;
    }

    if (fgets(line, line_size, statm_file) != NULL)
    {
        char *size_field = strtok(line, " ");
        char *resident_field = strtok(NULL, " ");
        if (size_field && resident_field)
        {
            unsigned long total_pages = strtoul(size_field, NULL, 10);
            unsigned long resident_pages = strtoul(resident_field, NULL, 10);

            int page_size = getpagesize();

            double total_memory_mb = (total_pages * page_size) / (1024.0 * 1024.0);
            double resident_memory_mb = (resident_pages * page_size) / (1024.0 * 1024.0);

            *memory_usage = resident_memory_mb;
        }
    }

    fclose(statm_file);

    // Frees dynamically allocated memory
    free(line);
    free(stat_path);
    free(statm_path);
}

/// @brief Prints a timestamp message to the screen.
/// @param message The beginning of the message.
void print_timestamp(const char *message)
{
    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);

    char formatted_time[64];
    strftime(formatted_time, sizeof(formatted_time), "%a, %b %d, %Y %I:%M:%S %p", local_time);

    fprintf(stdout, "%s, %s\n", message, formatted_time);
}

/// @brief Prints a status report to `stdout`.
/// @param processes The `ProcessInfo` to use.
/// @param process_count The number of processes.
void print_status_report(ProcessInfo *processes, int *process_count, const char *message)
{
    print_timestamp(message);

    for (int i = 0; i < *process_count; i++)
    {
        if (processes[i].running)
        {
            int cpu_usage = 0;
            double memory_usage = 0.0;
            get_process_resource_usage(processes[i].pid, &cpu_usage, &memory_usage);
            fprintf(stdout, "[%d] Running, cpu usage: %d%%, mem usage: %.2f MB\n", i, cpu_usage, memory_usage);
        }
        else if (processes[i].was_terminated)
        {
            fprintf(stdout, "[%d] Terminated\n", i);
        }
        else
        {
            fprintf(stdout, "[%d] Exited\n", i);
        }
    }
}

/// @brief Monitors actively-running processes and occasionally prints a status report on each.
/// @param processes The `ProcessInfo` to use.
/// @param process_count The number of processes.
/// @param timelimit The maximum time limit the running processes have to finish executing.
void monitor_processes(ProcessInfo *processes, int *process_count, int *timelimit)
{
    global_processes = processes;
    global_process_count = *process_count;

    while (1)
    {
        if (sigint_received || sigabrt_received)
        {
            for (int i = 0; i < global_process_count; i++)
            {
                if (global_processes[i].running)
                {
                    kill(global_processes[i].pid, SIGKILL);
                    global_processes[i].running = 0;
                    global_processes[i].was_terminated = 1;
                }
            }
            print_status_report(global_processes, process_count, "Signal Received - Terminating");
            fprintf(stdout, "Exiting (total time: %d seconds)\n", total_elapsed_time);
            break;
        }

        sleep(1);
        total_elapsed_time += 1;

        for (int i = 0; i < *process_count; i++)
        {
            if (processes[i].running)
            {
                int status;
                pid_t result = waitpid(processes[i].pid, &status, WNOHANG);

                if (result > 0) // Process has finished
                {
                    processes[i].running = 0;
                    global_processes_running--;

                    if (WIFEXITED(status)) // Exited normally
                    {
                        processes[i].running = 0;
                        processes[i].was_terminated = 0;
                    }
                    else if (WIFSIGNALED(status)) // Terminated by signal
                    {
                        processes[i].running = 0;
                        processes[i].was_terminated = 1;
                    }
                }
                else if (total_elapsed_time >= *timelimit && processes[i].running)
                {
                    kill(processes[i].pid, SIGKILL);
                    waitpid(processes[i].pid, &status, 0);
                    processes[i].running = 0;
                    processes[i].was_terminated = 1; // Explicitly terminated due to timeout
                    global_processes_running--;
                }
            }
        }

        if (global_processes_running == 0)
        {
            print_status_report(processes, process_count, "Terminating");
            fprintf(stdout, "Exiting (total time: %d seconds)\n", total_elapsed_time);
            break;
        }

        if (total_elapsed_time % 5 == 0)
        {
            print_status_report(processes, process_count, "Normal report");
        }
    }
}

/// @brief Cleans up memory of child processes.
/// @param processes The processes to free.
/// @param process_count The number of processes.
void cleanup_processes(ProcessInfo *processes, int *process_count)
{
    for (int i = 0; i < *process_count; i++)
    {
        if (processes[i].program_name)
        {
            free(processes[i].program_name);
            processes[i].program_name = NULL;
        }

        if (processes[i].args)
        {
            for (int j = 0; processes[i].args[j] != NULL; j++)
            {
                free(processes[i].args[j]);
            }
            free(processes[i].args);
            processes[i].args = NULL;
        }
    }

    // Frees the processes array itself (this was being really annoying lol)
    free(processes);
}

/// @brief ### The main entry point of macD.
/// @param argc The number of provided command line arguments.
/// @param argv An array containing the provided arguments.
/// @return `0` if the program executed successfully, `1` if it failed.
int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);
    signal(SIGABRT, handle_sigabrt);

    // Prints a usage message if we don't receive the proper number of arguments, then terminates.
    if (argc != 3)
    {
        print_usage_message();
        exit(EXIT_FAILURE);
    }

    int user_argument;
    char *input_file = malloc(PATH_MAX);

    // Terminates the program early if we can't allocate memory to `input_file`.
    if (input_file == NULL)
    {
        perror("Failed to allocate memory. Terminating...\n");
        exit(EXIT_FAILURE);
    }

    while ((user_argument = getopt(argc, argv, "i:")) != -1)
    {
        switch (user_argument)
        {
        case 'i':
            // Checks if the provided file name is longer than `PATH_MAX`, and terminates if so.
            if (strlen(optarg) > PATH_MAX)
            {
                fprintf(stderr, "./macD: Provided file name longer than PATH_MAX. Terminating...\n");
                exit(EXIT_FAILURE);
            }

            // Copies the input argument into the allocated memory for `input_file`, then ensures the copied string is null-terminated.
            strncpy(input_file, optarg, PATH_MAX - 1);
            input_file[PATH_MAX - 1] = '\0';
            break;
        default:
            free(input_file);
            exit(EXIT_FAILURE);
        }
    }

    // If the input file doesn't exist, terminate the program early.
    if (file_exists(input_file) != 0)
    {
        fprintf(stderr, "./macD: %s not found\n", input_file);
        free(input_file);
        exit(EXIT_FAILURE);
    }

    int timelimit;
    int program_count = 0;
    int capacity = ARG_MAX;

    ProcessInfo *processes = calloc(capacity, sizeof(ProcessInfo));
    if (!processes)
    {
        perror("Failed to allocate memory for processes array");
        exit(EXIT_FAILURE);
    }

    parse_config(input_file, &timelimit, &processes, &program_count, &capacity);

    print_timestamp("Starting report");

    for (int i = 0; i < program_count; i++)
    {
        launch_process(&processes[i], i);
    }

    monitor_processes(processes, &program_count, &timelimit);

    cleanup_processes(processes, &program_count);

    free(input_file);
    return 0;
}
