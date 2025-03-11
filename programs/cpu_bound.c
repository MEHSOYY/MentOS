#include <stdio.h>
#include "syscall.h" 

int main() {
    unsigned long start = get_ticks();  // Custom system call for time in ticks
    unsigned long now;

    do {
        now = get_ticks();
    } while ((now - start) < 5000);  // Assuming 1 tick = 1 millisecond

    printf("CPU-bound task completed\n");
    return 0;
}
