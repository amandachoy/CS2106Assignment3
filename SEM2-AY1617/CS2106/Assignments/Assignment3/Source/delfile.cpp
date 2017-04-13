#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 2)
	{
		printf("\nUsage: %s <file to delete>\n\n", av[0]);
		return -1;
	}
	
    initFS("part.dsk", "cs2106");
    delFile(av[1]);
    if (_result == FS_FILE_NOT_FOUND) {
        printf("FILE NOT FOUND\n");
		exit(-1);
    } else if (_result != FS_OK) {
		printf("Unknown Error\n");
		exit(-1);
	}
    
	return 0;
}
