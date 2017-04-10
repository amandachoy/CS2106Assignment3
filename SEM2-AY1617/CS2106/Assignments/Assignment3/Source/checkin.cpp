#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check in> <password>\n\n", av[0]);
		return -1;
	}
    
    initFS("part.dsk", av[2]);
    unsigned int index = findFile(av[1]);

    if (_result == FS_OK) {
        printf("DUPLICATE FILE\n");
    } else if (_result == FS_FILE_NOT_FOUND) {
        openFile(av[1], 1);
        printf("Checked In\n");
    }
	return 0;
}
