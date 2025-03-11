#include <stdio.h>
#include <time.h>

int main() {
    clock_t start = clock();
    while ((clock() - start) / CLOCKS_PER_SEC < 5) {
        // Busy wait
    }
    printf("CPU-bound task completed\n");
    return 0;
}
