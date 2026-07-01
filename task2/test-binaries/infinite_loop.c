#include <stdio.h>

int main(void)
{
    printf("[infinite_loop] entering endless loop\n");
    fflush(stdout);

    while (1) {
    }

    return 0;
}
