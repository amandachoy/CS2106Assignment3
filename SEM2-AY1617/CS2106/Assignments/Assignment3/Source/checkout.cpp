#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check out> <password>\n\n", av[0]);
		return -1;
	}
    
    initFS("part.dsk", av[2]);    
    if (_result == FS_ERROR) {
		printf("Unknown Error\n");
		exit(-1);
	}
	
	findFile(av[1]);

	if (_result == FS_OK) {
		unsigned int fileInEFS = openFile(av[1], MODE_READ_ONLY);
		
		FILE *fp = fopen(av[1], "w");
		if(fp == NULL)
		{
			printf("\nUnable to open source file %s\n\n", av[1]);
			exit(-1);
		}
        
		unsigned long size = getFileLength(av[1]);
		void *buffer = malloc(size);	
		
		readFile(fileInEFS, (void *) buffer, sizeof(char), size);
		closeFile(fileInEFS);

		fwrite(buffer, sizeof(char), size, fp);	
		fclose(fp);

		unmountFS();
		free(buffer);
    } else if (_result == FS_FILE_NOT_FOUND){
        printf("FILE NOT FOUND\n");
        exit(-1);
    } else {
		printf("Unknown Error\n");
		exit(-1);
	}
	return 0;
}
