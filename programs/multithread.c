#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void print_message(const char* message) {
    printf("%s\n", message);
}

int main() {
    pid_t pid1, pid2;

    pid1 = fork();
    if (pid1 == 0) {
        // First "thread"
        print_message("Thread 1");
        return 0;
    }

    pid2 = fork();
    if (pid2 == 0) {
        // Second "thread"
        print_message("Thread 2");
        return 0;
    }

    // Parent waits for both child processes to finish
    wait(NULL);
    wait(NULL);

    printf("Both threads have finished.\n");
    return 0;
}
