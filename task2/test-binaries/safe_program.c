#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("[safe_program] starting harmless work\n");
    sleep(1);
    printf("[safe_program] finished normally\n");
    return 0;
}
