/// @file ls.c
/// @brief Command 'ls'.
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include <dirent.h>
#include <fcntl.h>
#include <io/ansi_colors.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <strerror.h>
#include <string.h>
#include <sys/bitops.h>
#include <sys/stat.h>
#include <unistd.h>

#define FLAG_L (1U << 0U)
#define FLAG_A (1U << 1U)
#define FLAG_I (1U << 2U)
#define FLAG_1 (1U << 3U)

#define DENTS_NUM 12

static inline const char *to_human_size(unsigned long bytes)
{
    static char output[200];
    const char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
    char length          = sizeof(suffix) / sizeof(suffix[0]);
    int i                = 0;
    double dblBytes      = bytes;
    if (bytes > 1024) {
        for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024) {
            dblBytes = bytes / 1024.0;
        }
    }
    sprintf(output, "%.02lf %2s", dblBytes, suffix[i]);
    return output;
}

static inline void print_dir_entry_name(const char *name, mode_t st_mode)
{
    if (S_ISSOCK(st_mode)) {
        printf(FG_YELLOW_BRIGHT "%s" FG_RESET, name);
    }
    if (S_ISLNK(st_mode)) {
        printf(FG_CYAN_BRIGHT "%s" FG_RESET, name);
    }
    if (S_ISREG(st_mode)) {
        printf(FG_WHITE_BRIGHT "%s" FG_RESET, name);
    }
    if (S_ISBLK(st_mode)) {
        printf(FG_GREEN_BRIGHT "%s" FG_RESET, name);
    }
    if (S_ISDIR(st_mode)) {
        printf(FG_BLUE_BRIGHT "%s" FG_RESET, name);
    }
    if (S_ISCHR(st_mode)) {
        printf(FG_YELLOW_BRIGHT "%s" FG_RESET, name);
    }
    if (S_ISFIFO(st_mode)) {
        printf(FG_YELLOW_BRIGHT "%s" FG_RESET, name);
    }
}

static inline void print_dir_entry(dirent_t *dirent, const char *path, unsigned int flags, size_t *total_size)
{
    static char relative_path[PATH_MAX];
    tm_t *timeinfo;
    stat_t dstat;

    // Check if the file starts with a dot (hidden), and we did not receive
    // the `a` flag.
    if ((dirent->d_name[0] == '.') && !bitmask_check(flags, FLAG_A)) {
        return;
    }

    // Prepare the relative path.
    strcpy(relative_path, path);
    if (path[strnlen(path, PATH_MAX) - 1] != '/') {
        strcat(relative_path, "/");
    }
    strcat(relative_path, dirent->d_name);

    // Stat the file.
    if (stat(relative_path, &dstat) < 0) {
        printf("ls: failed to stat `%s`\n", relative_path);
        return;
    }

    // Deal with the -l.
    if (bitmask_check(flags, FLAG_L)) {
        // Get the broken down time from the creation time of the file.
        timeinfo = localtime(&dstat.st_ctime);
        // Print the inode if required.
        if (bitmask_check(flags, FLAG_I)) {
            printf("%6d ", dirent->d_ino);
        }
        // Print the file type.
        putchar(dt_char_array[dirent->d_type]);
        // Print the access permissions.
        putchar(bitmask_check(dstat.st_mode, S_IRUSR) ? 'r' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IWUSR) ? 'w' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IXUSR) ? 'x' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IRGRP) ? 'r' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IWGRP) ? 'w' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IXGRP) ? 'x' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IROTH) ? 'r' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IWOTH) ? 'w' : '-');
        putchar(bitmask_check(dstat.st_mode, S_IXOTH) ? 'x' : '-');
        // Add a space.
        putchar(' ');
        // Print the rest.
        printf(
            "%4d %4d %11s %02d/%02d %02d:%02d ", dstat.st_uid, dstat.st_gid, to_human_size(dstat.st_size),
            timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);

        print_dir_entry_name(dirent->d_name, dstat.st_mode);

        if (S_ISLNK(dstat.st_mode)) {
            char link_buffer[PATH_MAX];
            ssize_t len = readlink(relative_path, link_buffer, sizeof(link_buffer));
            if (len > 0) {
                link_buffer[len] = '\0';
                printf(" -> %s", link_buffer);
            }
        }
        putchar('\n');
        (*total_size) += dstat.st_size;
    } else {
        // Print the inode if required.
        if (bitmask_check(flags, FLAG_I)) {
            printf("%d ", dirent->d_ino);
        }
        print_dir_entry_name(dirent->d_name, dstat.st_mode);
        // Print in 1 column if requested.
        if (bitmask_check(flags, FLAG_1)) {
            putchar('\n');
        } else {
            putchar(' ');
        }
    }
}

static void print_ls(const char *path, unsigned int flags)
{
    // Open the directory.
    int fd = open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd == -1) {
        printf("ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }

    // Clear the directory entry buffer.
    dirent_t dents[DENTS_NUM];
    memset(&dents, 0, DENTS_NUM * sizeof(dirent_t));
    size_t total_size  = 0;
    ssize_t bytes_read = 0;
    while ((bytes_read = getdents(fd, dents, sizeof(dents))) > 0) {
        for (size_t i = 0; i < bytes_read / sizeof(dirent_t); ++i) {
            print_dir_entry(&dents[i], path, flags, &total_size);
        }
    }
    if (bytes_read < 0) {
        perror("getdents failed");
    }
    if (bitmask_check(flags, FLAG_L)) {
        printf("Total: %s\n", to_human_size(total_size));
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    // Create a variable to store flags.
    uint32_t flags = 0;
    // Check the number of arguments.
    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0)) {
            printf("List information about files inside a given directory.\n");
            printf("Usage:\n");
            printf("    ls [options] [directory]\n");
            return 0;
        } else if (argv[i][0] == '-') {
            for (int j = 1; j < strlen(argv[i]); ++j) {
                if (argv[i][j] == 'l') {
                    bitmask_set_assign(flags, FLAG_L);
                } else if (argv[i][j] == 'a') {
                    bitmask_set_assign(flags, FLAG_A);
                } else if (argv[i][j] == 'i') {
                    bitmask_set_assign(flags, FLAG_I);
                } else if (argv[i][j] == '1') {
                    bitmask_set_assign(flags, FLAG_1);
                }
            }
        } else if (!strcmp(argv[i], "--long")) {
            bitmask_set_assign(flags, FLAG_L);
        } else if (!strcmp(argv[i], "--all")) {
            bitmask_set_assign(flags, FLAG_A);
        } else if (!strcmp(argv[i], "--inode")) {
            bitmask_set_assign(flags, FLAG_I);
        }
    }
    bool_t no_directory = true;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            no_directory = false;
            printf("%s:\n", argv[i]);
            print_ls(argv[i], flags);
            printf("\n");
        }
    }
    if (no_directory) {
        char cwd[PATH_MAX];
        getcwd(cwd, PATH_MAX);
        print_ls(cwd, flags);
        printf("\n");
    }
    return 0;
}
