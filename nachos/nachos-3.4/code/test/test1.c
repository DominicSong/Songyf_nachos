#include "syscall.h"

int
main()
{
    int fd;
    char name[6];
    name[0] = 'a';
    name[1] = '.';
    name[2] = 't';
    name[3] = 'x';
    name[4] = 't';
    Create(name);
    Yield();
    Exit(0);
}
