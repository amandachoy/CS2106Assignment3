#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 2)
	{
		printf("\nUsage: %s <file to delete>\n\n", av[0]);
		return -1;
	}
    initFS("part.dsk", "2106s2");
    delFile(av[1]);
    if (_result == FS_FILE_NOT_FOUND) {
        printf("FILE NOT FOUND\n");
    } else {
        printf("FILE DELETED\n");
    }
    
	return 0;
}
