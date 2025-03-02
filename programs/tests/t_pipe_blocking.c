/// @file t_pipe_blocking.c
/// @brief Test blocking pipe operations between parent and child process.
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int main(void)
{
    int fds[2];
    char write_msg[]                 = "Blocking test message";
    char read_msg[sizeof(write_msg)] = {0};
    ssize_t bytes_written, bytes_read;
    int error_code = 0;

    // Create a pipe.
    if (pipe(fds) == -1) {
        fprintf(stderr, "Failed to create pipe\n");
        return 1;
    }

    // Fork a child process.
    pid_t pid = fork();

    if (pid == -1) {
        fprintf(stderr, "Failed to fork process\n");
        close(fds[0]);
        close(fds[1]);
        return 1;
    } else if (pid == 0) {
        // Child process: reads from the pipe.
        close(fds[1]); // Close unused write end

        printf("Child waiting to read from pipe...\n");
        do {
            bytes_read = read(fds[0], read_msg, sizeof(read_msg));
            if (bytes_read > 0) {
                printf("Child read message: '%s' (%ld bytes)\n", read_msg, bytes_read);
            } else if ((bytes_read == -1) && (errno != EAGAIN)) {
                fprintf(stderr, "Error occurred during read in child process\n");
                error_code = 1;
                break;
            }
        } while (bytes_read != 0);

        close(fds[0]); // Close read end
        return error_code;

    } else {
        // Parent process: writes to the pipe.
        close(fds[0]); // Close unused read end.

        // Sleep for 200 ms.
        timespec_t req = {0, 200000000};
        nanosleep(&req, NULL);

        printf("Parent writing to pipe...\n");
        bytes_written = write(fds[1], write_msg, sizeof(write_msg));

        if (bytes_written > 0) {
            printf("Parent wrote message: '%s' (%ld bytes)\n", write_msg, bytes_written);
        } else if (bytes_written == -1) {
            fprintf(stderr, "Error occurred during write in parent process\n");
            error_code = 1;
        }

        // Sleep for 200 ms.
        nanosleep(&req, NULL);

        close(fds[1]); // Close write end.
        wait(NULL);    // Wait for child to finish
        return error_code;
    }
}
