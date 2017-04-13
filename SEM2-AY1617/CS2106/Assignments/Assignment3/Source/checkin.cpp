#include "libefs.h"

int main(int ac, char **av)
{
	if(ac != 3)
	{
		printf("\nUsage: %s <file to check in> <password>\n\n", av[0]);
		return -1;
	}
    
    initFS("part.dsk", av[2]);
    if (_result == FS_ERROR) {
		printf("Unknown Error\n");
		exit(-1);
	}
	
    findFile(av[1]);

	if (_result == FS_FILE_NOT_FOUND) {
		unsigned int fileInEFS = openFile(av[1], MODE_CREATE);
		
		FILE *fp = fopen(av[1], "r");
		if(fp == NULL)
		{
			printf("\nUnable to open source file %s\n\n", av[1]);
			exit(-1);
		}
        
		// get the size of the file
        fseek(fp, 0, SEEK_END);
        unsigned long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        
		void *buffer = malloc(size);		
		fread(buffer, sizeof(char), size, fp);
		fclose(fp);
		
		writeFile(fileInEFS, (void *) buffer, sizeof(char), size);
		flushFile(fileInEFS);
		closeFile(fileInEFS);
		
		unmountFS();
		free(buffer);
    } else if (_result == FS_OK){
        printf("DUPLICATE FILE\n");
        exit(-1);
    } else {
		printf("Unknown Error\n");
		exit(-1);
	}
	return 0;
}
