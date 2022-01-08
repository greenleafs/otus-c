#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("I'm ready!\n");
    printf("Args passed: %d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        printf("Argument %d: %s\n", i, argv[i]);
    }
    return 0;
}
