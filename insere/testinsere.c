#include<stdio.h>
#include "testinsere.h"

long hello_syscall(void)
{
    return syscall(__NR_insere, 5);
}

int main(int argc, char *argv[])
{
    long int a = hello_syscall();
    printf("System call returned %ld\n", a);
    return 0;
}
// int main(void) {
//   printf("%d\n", insere(15));
// }
