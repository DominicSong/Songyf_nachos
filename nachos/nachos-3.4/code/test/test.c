#include "syscall.h"


int
main()
{
    int id;
    char name[6];
    name[0] = 't';
    name[1] = 'e';
    name[2] = 's';
    name[3] = 't';
    name[4] = '1';
    name[5] = '\0';
    id = Exec(name);
    Join(id);
    Exit(0);
}
