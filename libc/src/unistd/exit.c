/// @file exit.c
/// @brief
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include "system/syscall_types.h"
#include "unistd.h"

void exit(int status)
{
    long __res;
    __inline_syscall_1(__res, exit, status);
    // The process never returns from this system call!
}
