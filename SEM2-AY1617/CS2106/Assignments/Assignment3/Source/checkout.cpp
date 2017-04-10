#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check out> <password>\n\n", av[0]);
		return -1;
	}
    
    initFS("part.dsk", av[2]);
    unsigned int index = findFile(av[1]);
    if (_result == FS_FILE_NOT_FOUND) {
        printf("FILE NOT FOUND\n");
    } else {
        flushFile(index);
        closeFile(index);
        closeFS();
        printf("Checked Out\n");
    }
    
	return 0;
}
