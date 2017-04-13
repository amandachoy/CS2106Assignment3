#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 2)
	{
		printf("\nUsage: %s <file to check>\n", av[0]);
		printf("Prints: 'R' = Read only, 'W' = Read/Write\n\n");
		return -1;
	}

    initFS("part.dsk", "cs2106");
    if (_result == FS_ERROR) {
		printf("Unknown Error\n");
		exit(-1);
	}
	    
	unsigned int attr = getAttr(av[1]);
    if (_result == FS_OK) {
		if(attr & 0x04) {
			printf("R\n");
		} else {
			printf("W\n");
		}
	    closeFS();
	} else if (_result == FS_FILE_NOT_FOUND) {
		printf("FILE NOT FOUND\n");
		closeFS();
        exit(-1);
	} else {
		printf("Unknown Error\n");
		closeFS();
		exit(-1);
	}
	
	return 0;
}
