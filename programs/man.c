/// @file man.c
/// @brief Shows the available commands.
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strerror.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc == 1) {
        int fd = open("/bin", O_RDONLY | O_DIRECTORY, 0);
        if (fd == -1) {
            printf("%s: cannot access '/bin': %s\n", argv[0], strerror(errno));
            return 1;
        }
        dirent_t dent;
        int per_line = 0;
        while (getdents(fd, &dent, sizeof(dirent_t)) == sizeof(dirent_t)) {
            // Shows only regular files
            if (dent.d_type == DT_REG) {
                printf("%10s ", dent.d_name);
                if (++per_line == 6) {
                    per_line = 0;
                    putchar('\n');
                }
            }
        }
        putchar('\n');
        close(fd);
    } else if (argc == 2) {
        char *pager = "more";
        char filepath[PATH_MAX];
        strcpy(filepath, "/usr/share/man/");
        strncat(filepath, argv[1], PATH_MAX);
        strncat(filepath, ".man", PATH_MAX);
        int fd = open(filepath, O_RDONLY, 42);
        if (fd < 0) {
            printf("%s: No manual entry for %s\n", argv[0], argv[1]);
            exit(1);
        }
        close(fd);
        execlp(pager, pager, filepath, NULL);
    }
    return 0;
}
