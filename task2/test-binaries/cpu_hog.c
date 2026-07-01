#include <stdint.h>
#include <stdio.h>

int main(void)
{
    volatile uint64_t value = 0;

    printf("[cpu_hog] consuming CPU\n");
    fflush(stdout);

    while (1) {
        value++;
        value *= 3;
        value ^= 0x5a5a5a5a;
    }

    return 0;
}
