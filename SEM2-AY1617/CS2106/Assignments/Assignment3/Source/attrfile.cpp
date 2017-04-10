#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check set attrbute> <attribute>\n", av[0]);
		printf("Attribute: 'R' = Read only, 'W' = Read/Write\n\n");
		return -1;
	}

    initFS("part.dsk", "2106s2");
    unsigned int attr = getAttr(av[1]);
    if (strcmp(av[2], "R")) {
        attr &= ~(1 << 2);
    } else if (strcmp(av[2], "W")) {
        attr |= 1 << 2;
    }
    setAttr(av[1], attr);
    updateDirectory();
    printf("New Attribute: %u\n", attr);
	return 0;
}
