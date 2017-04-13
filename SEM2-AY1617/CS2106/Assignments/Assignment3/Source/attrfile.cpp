#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check set attrbute> <attribute>\n", av[0]);
		printf("Attribute: 'R' = Read only, 'W' = Read/Write\n\n");
		return -1;
	}

    initFS("part.dsk", "cs2106");
    if (_result == FS_ERROR) {
		printf("Unknown Error\n");
		exit(-1);
	}
	    
	unsigned int attr = getAttr(av[1]);
    if (_result == FS_OK) {
		if (strlen(av[2])==1 && (av[2][0]=='r' || av[2][0]=='R')) {
			attr = attr | 0x04;
		} else if (strlen(av[2])==1 && (av[2][0]=='w' || av[2][0]=='W')) {
			attr = attr & 0xFB;
		}
		setAttr(av[1], attr);
		
		updateDirectory();
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
