/// @file getpgid.c
/// @brief
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include "errno.h"
#include "system/syscall_types.h"
#include "unistd.h"

// _syscall1(pid_t, getpgid, pid_t, pid)
pid_t getpgid(pid_t pid)
{
    long __res;
    __inline_syscall_1(__res, getpgid, pid);
    __syscall_return(pid_t, __res);
}
