/// @file poweroff.c
/// @brief
/// @copyright (c) 2014-2024 This file is distributed under the MIT License.
/// See LICENSE.md for details.

#include <stdio.h>
#include <sys/reboot.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    printf("Executing power-off...\n");
    reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, NULL);
    return 0;
}
