#include "libefs.h"
#include <fcntl.h>

// FS Descriptor
TFileSystemStruct *_fs;

// Open File Table
TOpenFile *_oft;

// Open file table counter
int _oftCount=0;

// Mounts a paritition given in fsPartitionName. Must be called before all
// other functions
void initFS(const char *fsPartitionName, const char *fsPassword)
{
    mountFS(fsPartitionName, fsPassword);
    _fs = getFSInfo();
}

// Opens a file in the partition. Depending on mode, a new file may be created
// if it doesn't exist, or we may get FS_FILE_NOT_FOUND in _result. See the enum above for valid modes.
// Return -1 if file open fails for some reason. E.g. file not found when mode is MODE_NORMAL, or
// disk is full when mode is MODE_CREATE, etc.

int openFile(const char *filename, unsigned char mode)
{
    
    char* dataBuffer = makeDataBuffer();
    unsigned long* inodeBuffer = makeInodeBuffer();
 /*
    _oft->openMode = mode;
    _oft->blockSize = _fs->blockSize;
    _oft->inode = index;
    _oft->inodeBuffer = inodeBuffer;
    _oft->buffer = dataBuffer;
    */
    int i;
    switch (mode) {
        case 0:
            i = open(filename, O_RDWR);
            if (_result != FS_FILE_NOT_FOUND && i >= 0) {
                _oftCount++;
            } else {
                return -1;
            }
            break;
        case 1:
            i = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (i >= 0) {
                unsigned long freeBlock = findFreeBlock();
                markBlockBusy(freeBlock);
                inodeBuffer[0] = freeBlock;
                setBlockNumInInode(inodeBuffer, 0, freeBlock);
                updateFreeList();
                makeDirectoryEntry(filename, 0x0, getFileLength(filename));
                _oftCount++;
            } else {
                return -1;
            }
            break;
        case 2:
            i = open(filename, O_RDONLY);
            if (_result != FS_FILE_NOT_FOUND && i >= 0) {
                _oftCount++;
            } else {
                return -1;
            }
        case 3:
            i = open(filename, O_RDWR | O_APPEND);
            if (_result != FS_FILE_NOT_FOUND && i >= 0) {
                _oftCount++;
            } else {
                return -1;
            }
        default:
            break;
    }
    
    unsigned int index = findFile(filename);
    saveInode(inodeBuffer, index);
    updateDirectory();
	return index;
}

// Write data to the file. File must be opened in MODE_NORMAL or MODE_CREATE modes. Does nothing
// if file is opened in MODE_READ_ONLY mode.
void writeFile(int fp, void *buffer, unsigned int dataSize, unsigned int dataCount)
{
    if (_oft->openMode == 2 || _oft->openMode == 3) {
        return;
    }
    unsigned long* inodeBuffer = makeInodeBuffer();
    loadInode(inodeBuffer, fp);
    
    int i = 0;

    unsigned long blockNum = inodeBuffer[i];
    unsigned int totalData = dataSize * dataCount;
    int j = 0;
    
    while (j < ceil(totalData/(_fs->blockSize))) {
        int currentOffset = j * _fs->blockSize;
        writeBlock((char*)buffer + currentOffset, blockNum);
        unsigned long nextBlock = findFreeBlock();
        markBlockBusy(nextBlock);
        inodeBuffer[i++] = nextBlock;
        updateFreeList();
        j++;
    }
    
    saveInode(inodeBuffer, fp);
    updateDirectory();
    free(inodeBuffer);
    free(buffer);
}

// Flush the file data to the disk. Writes all data buffers, updates directory,
// free list and inode for this file.
//??
void flushFile(int fp)
{
    updateFreeList();
    updateDirectory();
}

// Read data from the file.
void readFile(int fp, void *buffer, unsigned int dataSize, unsigned int dataCount)
{
    unsigned long* inodeBuffer = makeInodeBuffer();
    loadInode(inodeBuffer, fp);
    
    int i = 0;
    
    unsigned long blockNum = inodeBuffer[i];
    unsigned int totalData = dataSize * dataCount;
    int j = 0;
    
    while (j < ceil(totalData/(_fs->blockSize))) {
        int currentOffset = j * _fs->blockSize;
        readBlock((char*)buffer + currentOffset, blockNum);
        blockNum = inodeBuffer[i++];
        j++;
    }
    
    saveInode(inodeBuffer, fp);
    free(inodeBuffer);
}

// Delete the file. Read-only flag (bit 2 of the attr field) in directory listing must not be set. 
// See TDirectory structure.
void delFile(const char *filename) {
    unsigned int attr = getAttr(filename);
    unsigned int index = findFile(filename);
    if (_result != FS_FILE_NOT_FOUND) {
        unsigned long *inode;
        loadInode(inode, index);
        if (attr >> 2 == 0) {
            markBlockFree(inode[0]);
            delDirectoryEntry(filename);
        } else {
            printf("This file is READ ONLY");
        }
        updateFreeList();
        updateDirectory();
    }
}

// Close a file. Flushes all data buffers, updates inode, directory, etc.
void closeFile(int fp) {
    
}


// Unmount file system.
void closeFS() {
    unmountFS();
}

