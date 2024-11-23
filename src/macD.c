#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/getopt_core.h>
#include <signal.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/limits.h>

/// @brief A struct representing information about a process.
typedef struct
{
    pid_t pid;
    char program_name[PATH_MAX];
    int running;
    int was_terminated;
} ProcessInfo;

/// @brief ### Launches a given program as a child process.
/// @param program_path The path to the program executable.
/// @param timelimit The maximum time limit this process is allowed to run for.
void launch_process(const char *absolute_program_path, const char *relative_program_path, int timelimit, int process_index, ProcessInfo *processes)
{
    struct stat buffer;
    if (stat(absolute_program_path, &buffer) != 0)
    {
        fprintf(stderr, "[%d]%s, failed to start\n", process_index, relative_program_path);
        processes[process_index].running = 0;
        return;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    { // Child process logic
        char *args[] = {(char *)absolute_program_path, NULL};
        execvp(args[0], args);

        exit(EXIT_FAILURE);
    }
    else
    { // Parent process logic
        fprintf(stdout, "[%d]%s, started successfully (pid: %d)\n", process_index, relative_program_path, pid);

        processes[process_index].pid = pid;
        processes[process_index].running = 1;
        strncpy(processes[process_index].program_name, relative_program_path, PATH_MAX - 1);
    }
}

/// @brief ### Reads the given config file and populates the timelimit and program paths.
/// @param config_file The path to the config file.
/// @param timelimit A pointer to store the time limit in.
/// @param programs An array to store program paths in.
/// @return The number of programs found.
int parse_config(const char *config_file, int *timelimit, char absolute_programs[LINK_MAX][PATH_MAX], char relative_programs[LINK_MAX][PATH_MAX])
{
    FILE *file = fopen(config_file, "r");
    if (!file)
    {
        perror("Failed to open configuration file");
        exit(EXIT_FAILURE);
    }

    char line[PATH_MAX];
    int program_count = 0;

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    while (fgets(line, sizeof(line), file))
    {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "timelimit ", 10) == 0)
        {
            sscanf(line, "timelimit %d", timelimit);
        }
        else if (strspn(line, " \t\r\n") != strlen(line)) // <- Checks if the line isn't empty
        {
            // Concatenates the current line in the config file to the current working directory
            char absolute_path[PATH_MAX] = {0};
            strcat(absolute_path, cwd);
            strcat(absolute_path, line);
            strncpy(absolute_programs[program_count], absolute_path, PATH_MAX - 1);

            strncpy(relative_programs[program_count], line, PATH_MAX - 1);

            absolute_programs[program_count][PATH_MAX - 1] = '\0';
            program_count++;

            // Reset the absolute path string after each iteration
            strncpy(absolute_path, "", 1);

            if (program_count >= LINK_MAX)
            {
                fprintf(stderr, "Too many programs in configuration file. Max allowed is %d.\n", LINK_MAX);
                break;
            }
        }
    }

    fclose(file);
    return program_count;
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
void get_process_resource_usage(int pid, int *cpu_usage, int *memory_usage)
{
    unsigned long utime, stime, starttime;
    long total_time;
    unsigned long long elapsed_time;

    char process_string[255];
    snprintf(process_string, sizeof(process_string), "/proc/%d/stat", pid);

    FILE *process_info_file = fopen(process_string, "r");
    if (process_info_file == NULL)
    {
        perror("Failed to open /proc/[pid]/stat");
        return;
    }

    char line[1024];
    if (fgets(line, sizeof(line), process_info_file) != NULL)
    {
        char *fields[64];
        int field_index = 0;

        char *token = strtok(line, " ");
        while (token != NULL)
        {
            fields[field_index++] = token;
            token = strtok(NULL, " ");
        }

        // Set the value in the memory_usage pointer to the process memory (converted to MB)
        *memory_usage = atoi(fields[22]) / 1024 / 1024;

        // Start calculating the CPU %
        utime = strtoul(fields[13], NULL, 10);
        stime = strtoul(fields[14], NULL, 10);
        starttime = strtoul(fields[21], NULL, 10);
        total_time = utime + stime;

        long clock_ticks_per_sec = sysconf(_SC_CLK_TCK);

        double uptime;
        FILE *uptime_file = fopen("/proc/uptime", "r");
        if (uptime_file)
        {
            fscanf(uptime_file, "%lf", &uptime);
            fclose(uptime_file);
        }

        elapsed_time = (unsigned long long)(uptime * clock_ticks_per_sec) - starttime;

        // Set the value in the cpu_usage pointer to the process CPU utilization (= total_time * 100 / elapsed_time)
        *cpu_usage = (total_time * 100) / (elapsed_time);
    }

    fclose(process_info_file);
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
void print_status_report(ProcessInfo *processes, int process_count)
{
    print_timestamp("Normal report");

    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].running)
        {
            int cpu_usage, memory_usage;
            get_process_resource_usage(processes[i].pid, &cpu_usage, &memory_usage);

            fprintf(stdout, "[%d] Running, cpu usage: %d%%, mem usage: %d MB\n", i, cpu_usage, memory_usage);
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
    fprintf(stdout, "\n");
}

/// @brief Monitors actively-running processes and occasionally prints a status report on each.
/// @param processes The `ProcessInfo` to use.
/// @param process_count The number of processes.
/// @param timelimit The maximum time limit the running processes have to finish executing.
void monitor_processes(ProcessInfo *processes, int process_count, int timelimit)
{
    int elapsed_time = 0;

    while (1)
    {
        sleep(5);
        elapsed_time += 5;

        // Check process statuses
        for (int i = 0; i < process_count; i++)
        {
            if (processes[i].running)
            {
                int status;
                pid_t result = waitpid(processes[i].pid, &status, WNOHANG);

                if (result > 0)
                { // Process has completed
                    processes[i].running = 0;
                }
                else if (elapsed_time >= timelimit && processes[i].running)
                {
                    kill(processes[i].pid, SIGKILL);
                    waitpid(processes[i].pid, &status, 0);
                    processes[i].running = 0;
                    processes[i].was_terminated = 1;
                }
            }
        }

        // Print the status report
        print_status_report(processes, process_count);

        // Stop monitoring after the time limit
        if (elapsed_time >= timelimit)
        {
            fprintf(stdout, "Exiting (total time: %d seconds)\n", elapsed_time);
            break;
        }
    }
}

/// @brief ### The main entry point of macD.
/// @param argc The number of provided command line arguments.
/// @param argv An array containing the provided arguments.
/// @return `0` if the program executed successfully, `1` if it failed.
int main(int argc, char *argv[])
{
    int user_argument;
    char *input_file = malloc(PATH_MAX);

    // Terminates the program early if we can't allocate memory to `input_file`.
    if (input_file == NULL)
    {
        perror("Failed to allocate memory. Terminating...\n");
        exit(EXIT_FAILURE);
    }

    // Prints a usage message if we don't receive the proper number of arguments, then terminates.
    if (argc != 3)
    {
        print_usage_message();
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
    char absolute_programs[LINK_MAX][PATH_MAX];
    char relative_programs[LINK_MAX][PATH_MAX];
    int program_count = parse_config(input_file, &timelimit, absolute_programs, relative_programs);

    ProcessInfo processes[LINK_MAX] = {0};

    print_timestamp("Starting report");

    // fprintf(stdout, "Timelimit: %d seconds\n", timelimit);

    /*
    fprintf(stdout, "Programs to execute:\n");
    for (int i = 0; i < program_count; i++)
    {
        fprintf(stdout, "   %s\n", programs[i]);
    }
    */

    for (int i = 0; i < program_count; i++)
    {
        launch_process(absolute_programs[i], relative_programs[i], timelimit, i, processes);
    }

    monitor_processes(processes, program_count, timelimit);

    free(input_file);
    return 0;
}
