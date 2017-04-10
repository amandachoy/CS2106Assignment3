#include "libefs.h"
#include <fcntl.h>

// FS Descriptor
TFileSystemStruct *_fs;

// Open File Table
TOpenFile *_oft;

// Open file table counter
int _oftCount=0;

const char *filenames [100];

// Mounts a paritition given in fsPartitionName. Must be called before all
// other functions
void initFS(const char *fsPartitionName, const char *fsPassword)
{
    mountFS(fsPartitionName, fsPassword);
    _fs = getFSInfo();
    _oft = (TOpenFile *) calloc(sizeof(TOpenFile), _fs->maxFiles);
    _oftCount = 0;
}

int createOpenFileEntry(const char *filename, int mode, unsigned int inode, unsigned long len) {
	_oft[_oftCount].openMode = mode;
	_oft[_oftCount].blockSize = _fs->blockSize;
	_oft[_oftCount].inode = inode;
	unsigned long* inodeBuffer = makeInodeBuffer();
	loadInode(inodeBuffer, inode);
	_oft[_oftCount].inodeBuffer = inodeBuffer;
	_oft[_oftCount].buffer = makeDataBuffer();
	_oft[_oftCount].writePtr = (mode == MODE_READ_APPEND ? (len % _fs->blockSize) : 0);
	_oft[_oftCount].readPtr = 0;
	_oft[_oftCount].filePtr = (mode == MODE_READ_APPEND ? len : 0);
	filenames[_oftCount] = filename;
	//~ printf("lol %d\n",_oft[_oftCount].filePtr);
	_oftCount++;
	return _oftCount - 1;
}

// Opens a file in the partition. Depending on mode, a new file may be created
// if it doesn't exist, or we may get FS_FILE_NOT_FOUND in _result. See the enum above for valid modes.
// Return -1 if file open fails for some reason. E.g. file not found when mode is MODE_NORMAL, or
// disk is full when mode is MODE_CREATE, etc.

int openFile(const char *filename, unsigned char mode)
{
    unsigned int i = findFile(filename);
    switch (mode) {
        case MODE_NORMAL:
            if (_result == FS_OK) {
				return createOpenFileEntry(filename, mode, i, getFileLength(filename));
            } else {
                return -1;
            }
            break;
        case MODE_CREATE:
            if (_result == FS_OK) {
				return createOpenFileEntry(filename, mode, i, getFileLength(filename));
            } else {
				i = makeDirectoryEntry(filename, 0x80, 0);
				if(_result == FS_DIR_FULL || _result == FS_DIR_FULL) {
					return -1;
				} else {
					updateDirectory();
					return createOpenFileEntry(filename, mode, i, 0); 
				}
            }
            break;
        case MODE_READ_ONLY:
            if (_result == FS_OK) {
				return createOpenFileEntry(filename, mode, i, getFileLength(filename));
            } else {
                return -1;
            }
            break;
        case MODE_READ_APPEND:
            if (_result == FS_OK) {
				int fp = createOpenFileEntry(filename, mode, i, getFileLength(filename));
				readBlock(_oft[fp].buffer, returnBlockNumFromInode(_oft[fp].inodeBuffer, _oft[fp].filePtr));
				return fp;
            } else {
                return -1;
            }
            break;
        default:
            return -1;
    }
}

// Write data to the file. File must be opened in MODE_NORMAL or MODE_CREATE modes. Does nothing
// if file is opened in MODE_READ_ONLY mode.
void writeFile(int fp, void *buffer, unsigned int dataSize, unsigned int dataCount)
{
	TOpenFile f = _oft[fp];
    if (f.openMode == MODE_READ_ONLY || dataSize <= 0) {
        return;
    }
  
    unsigned int remaining = dataSize, lenToWriteIntoThisBlock;
    unsigned long blockNumber;
	
	while(remaining > 0) {
		blockNumber = returnBlockNumFromInode(f.inodeBuffer, f.filePtr);

		if(blockNumber == 0){
			blockNumber = findFreeBlock();
			markBlockBusy(blockNumber);
			setBlockNumInInode(f.inodeBuffer, f.filePtr, blockNumber);
		}

		lenToWriteIntoThisBlock = f.blockSize - f.writePtr < remaining ?
								  (f.blockSize - f.writePtr) : remaining;
		strncpy(f.buffer + f.writePtr, (char *)buffer+(dataSize-remaining), lenToWriteIntoThisBlock);
		f.filePtr = f.filePtr + lenToWriteIntoThisBlock;
		f.writePtr = (f.writePtr + lenToWriteIntoThisBlock) % f.blockSize;
		remaining -= lenToWriteIntoThisBlock;
		if(f.writePtr == 0) {
			writeBlock(f.buffer, blockNumber);
		}
	}
	updateDirectoryFileLength(filenames[fp], f.filePtr);
	_oft[fp] = f;
}

// Flush the file data to the disk. Writes all data buffers, updates directory,
// free list and inode for this file.

void flushFile(int fp)
{
	TOpenFile f = _oft[fp];
    if (f.openMode == MODE_READ_ONLY) {
        return;
    }

	if(f.writePtr != 0) {
		memset(f.buffer + f.writePtr, 0, f.blockSize - f.writePtr);
		writeBlock(f.buffer, returnBlockNumFromInode(f.inodeBuffer, f.filePtr));
	}    
    updateDirectory();
	updateFreeList();
	saveInode(f.inodeBuffer, f.inode);
}

// Read data from the file.
void readFile(int fp, void *buffer, unsigned int dataSize, unsigned int dataCount)
{
	TOpenFile f = _oft[fp];
    if (dataSize <= 0) {
        return;
    }
  
    unsigned int remaining = dataSize, lenToReadFromThisBlock;
    unsigned long blockNumber;
	
	while(remaining > 0) {
		blockNumber = returnBlockNumFromInode(f.inodeBuffer, f.filePtr);
		if(blockNumber == 0){
			memset((char *)buffer + (dataSize - remaining), 0, f.blockSize);
		}else{
			readBlock(f.buffer, blockNumber);
		}
		lenToReadFromThisBlock = f.blockSize - f.readPtr < remaining ?
								  (f.blockSize - f.readPtr) : remaining;
		strncpy((char *)buffer+(dataSize-remaining), f.buffer, lenToReadFromThisBlock);

		f.filePtr = f.filePtr + lenToReadFromThisBlock;
		f.readPtr = (f.readPtr + lenToReadFromThisBlock) % f.blockSize;
		remaining -= lenToReadFromThisBlock;
	}
}

// Delete the file. Read-only flag (bit 2 of the attr field) in directory listing must not be set. 
// See TDirectory structure.
void delFile(const char *filename) {
    unsigned int i = findFile(filename);
    if (_result == FS_OK) {
		unsigned int attr = getAttr(filename);
		bool readonly = attr | (1<<6);
		if(readonly) {
			return;
		} else {
			int numOfBlocks = getFileLength(filename) / _fs->blockSize;
			unsigned long *inodeBuffer = makeInodeBuffer();
			loadInode(inodeBuffer, i);
			for(int i=0;i<numOfBlocks;i++){
				if(inodeBuffer[i] != 0) {
					markBlockFree(inodeBuffer[i]);
					inodeBuffer[i] = 0;
				}
			}
			updateFreeList();
			saveInode(inodeBuffer, i);
			delDirectoryEntry(filename);
			updateDirectory();
		}
	}
	
	return;
}

// Close a file. Flushes all data buffers, updates inode, directory, etc.
void closeFile(int fp) {
	flushFile(fp);
}


// Unmount file system.
void closeFS() {
	for(int i = 0; i < _oftCount; i++){
		closeFile(i);
	}
    unmountFS();
    free(_oft);
}

int main() {
	initFS("part.dsk", "cs2106");
	int a = openFile("hehe.txt", MODE_READ_APPEND);
	printf("a: %d\n",a);
	//~ int b = openFile("hehe1.txt", MODE_CREATE);
	//~ printf("b: %d\n",b);
	char * content = "test heloo word!!!";
	writeFile(a, content, strlen(content), 0);

	flushFile(a);
	closeFile(a);
	
	char * readed = (char *)calloc(sizeof(char), 100);
	a = openFile("hehe.txt", MODE_NORMAL);
	readFile(a, (void *)readed, strlen(content)*3, 0);
	printf("readed: %s\n", readed+19);
	printf("readed: %s\n", readed);
	printf("readed: %s\n", readed+38);

	closeFS();
	return 0;
}

