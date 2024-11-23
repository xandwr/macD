#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/getopt_core.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>

/// @brief Launches a test program that runs for 3 seconds and exits.
void launch_process()
{
    fprintf(stdout, "Forking new child process...\n");
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("Fork failed. Exiting...");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    { // -- Child process logic --
        char *args[] = {"./test_program", NULL};
        fprintf(stdout, "Child process %s starting. Now awaiting completion...\n", args[0]);
        execvp(args[0], args);

        fprintf(stderr, "%s execuction failed.\n", args[0]);
        exit(EXIT_FAILURE);
    }
    else
    { // -- Parent process logic --
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            printf("SUCCESS: Child process exited with status %d.\n", WEXITSTATUS(status));
        }
        else
        {
            printf("FAILURE: Child process terminated abnormally.\n");
        }
    }
}

/// @brief Prints a program helper message to stdout.
void print_usage_message()
{
    fprintf(stdout, "Usage: ./macD -i [config file]\n");
}

/// @brief Checks if a given file exists in the local path.
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

/// @brief The main entry point of macD.
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

    // Launches the test program to validate child process logic.
    launch_process();

    free(input_file);
    return 0;
}
