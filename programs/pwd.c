/// @file pwd.c
/// @brief
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include <limits.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char cwd[PATH_MAX];
    getcwd(cwd, PATH_MAX);
    printf("%s\n", cwd);
    return 0;
}
