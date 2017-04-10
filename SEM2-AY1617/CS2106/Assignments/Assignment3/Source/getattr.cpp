#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 2)
	{
		printf("\nUsage: %s <file to check>\n", av[0]);
		printf("Prints: 'R' = Read only, 'W' = Read/Write\n\n");
		return -1;
	}

    initFS("part.dsk", "2106s2");
    unsigned int attr = getAttr(av[1]);
    if (attr >> 2 == 1) {
        printf("R\n");
    } else if (attr >> 2 == 0) {
        printf("W\n");
    } else if (FS_FILE_NOT_FOUND) {
        printf("FILE NOT FOUND\n");
    }
	return 0;
}
