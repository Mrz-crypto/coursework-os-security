#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("[slow_program] sleeping longer than sandbox allows\n");
    fflush(stdout);
    sleep(30);
    return 0;
}
