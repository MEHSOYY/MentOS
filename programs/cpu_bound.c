#include <stdio.h>

int main() {
    volatile int i, j;
    for (i = 0; i < 100000; i++) {
        for (j = 0; j < 100000; j++) {
            // Busy-wait loop
        }
    }

    printf("CPU-bound task completed\n");
    return 0;
}
