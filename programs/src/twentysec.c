#include <stdio.h>
#include <unistd.h>

/// @brief Executes a test program that runs for 21 seconds and exits (should halt execution with a 20 second time limit).
int main() {
    sleep(21);
    return 0;
}
