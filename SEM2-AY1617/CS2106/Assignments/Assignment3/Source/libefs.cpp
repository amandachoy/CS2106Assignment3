#include "libefs.h"
#include <fcntl.h>

// FS Descriptor
TFileSystemStruct *_fs;

// Open File Table
TOpenFile *_oft;

// Open file table counter
int _oftCount=0;

char **filenames;

// Mounts a paritition given in fsPartitionName. Must be called before all
// other functions
void initFS(const char *fsPartitionName, const char *fsPassword)
{
    if(strlen(fsPassword) > MAX_PWD_LEN) {
		_result = FS_ERROR;
		return;
	}
	
    mountFS(fsPartitionName, fsPassword);
    _fs = getFSInfo();
    _oft = (TOpenFile *) calloc(sizeof(TOpenFile), _fs->maxFiles);
    filenames = (char **) calloc(sizeof(char *), _fs->maxFiles);
    for(int i = 0; i < _fs->maxFiles; i++){
		filenames[i] = (char *) calloc(sizeof(char), MAX_FNAME_LEN);
	}
    _oftCount = 0;
}

int createOpenFileEntry(const char *filename, int mode, unsigned int inode, unsigned long len) {
	if (_oftCount >= _fs->maxFiles) {
		_result = FS_ERROR;
		return -1;
	}
	
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
	memcpy(filenames[_oftCount], filename, strlen(filename));
	_oftCount++;
	_result = FS_OK;
	return _oftCount - 1;
}

// Opens a file in the partition. Depending on mode, a new file may be created
// if it doesn't exist, or we may get FS_FILE_NOT_FOUND in _result. See the enum above for valid modes.
// Return -1 if file open fails for some reason. E.g. file not found when mode is MODE_NORMAL, or
// disk is full when mode is MODE_CREATE, etc.

int openFile(const char *filename, unsigned char mode)
{
	if (strlen(filename) > MAX_FNAME_LEN) {
		_result = FS_ERROR;
		return -1;
	}
	
    unsigned int i = findFile(filename);
    switch (mode) {
        case MODE_NORMAL:
            if (_result == FS_OK) {
				// cannot open in this mode if the file is read only
				unsigned int attr = getAttr(filename);
				bool isReadOnly = attr & 0x04;
				if(isReadOnly) {
					_result = FS_ERROR;
					return -1;
				}
				return createOpenFileEntry(filename, mode, i, getFileLength(filename));
            } else {
                return -1;
            }
            break;
        case MODE_CREATE:
            if (_result == FS_OK) {
				// cannot open in this mode if the file is read only
				unsigned int attr = getAttr(filename);
				bool isReadOnly = attr & 0x04;
				if(isReadOnly) {
					_result = FS_ERROR;
					return -1;
				}
				return createOpenFileEntry(filename, mode, i, getFileLength(filename));
            } else {
				// test whether the disk is full
				findFreeBlock();
				if(_result == FS_FULL) {
					return -1;
				}
				
				// test whether there is a free directory entry
				i = makeDirectoryEntry(filename, 0x01, 0);
				if(_result == FS_DIR_FULL) {
					return -1;
				}
				
				updateDirectory();
				return createOpenFileEntry(filename, mode, i, 0); 
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
				// cannot open in this mode if the file is read only
				unsigned int attr = getAttr(filename);
				bool isReadOnly = attr & 0x04;
				if(isReadOnly) {
					_result = FS_ERROR;
					return -1;
				}
				return createOpenFileEntry(filename, mode, i, getFileLength(filename));
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
    if (f.openMode == MODE_READ_ONLY || f.inode == -1 || dataSize <= 0 || dataCount <= 0) {
		_result = FS_ERROR;
        return;
    }
  
    unsigned int total = dataSize * dataCount;
    unsigned int remaining = total, lenToWriteIntoThisBlock;
    unsigned long blockNumber;
	
	while(remaining > 0) {
		blockNumber = returnBlockNumFromInode(f.inodeBuffer, f.filePtr);

		if(blockNumber == 0){
			blockNumber = findFreeBlock();
			
			if(_result == FS_FULL) {
				// stop when there is no space in the disk
				return;
			}
			
			markBlockBusy(blockNumber);
			setBlockNumInInode(f.inodeBuffer, f.filePtr, blockNumber);
		} else {
			readBlock(f.buffer, blockNumber);
		}

		lenToWriteIntoThisBlock = (f.blockSize - f.writePtr) < remaining ?
								  (f.blockSize - f.writePtr) : remaining;
		memcpy(f.buffer + f.writePtr, 
		       (char *)buffer + total - remaining, 
		       lenToWriteIntoThisBlock);
		f.filePtr = f.filePtr + lenToWriteIntoThisBlock;
		f.writePtr = (f.writePtr + lenToWriteIntoThisBlock) % f.blockSize;
		remaining -= lenToWriteIntoThisBlock;
		writeBlock(f.buffer, blockNumber);
		if (f.filePtr > getFileLength(filenames[fp])) {
			printf("aaa");
			updateDirectoryFileLength(filenames[fp], f.filePtr);
		}
	}
	
	_oft[fp] = f;
}

// Flush the file data to the disk. Writes all data buffers, updates directory,
// free list and inode for this file.
void flushFile(int fp)
{
	TOpenFile f = _oft[fp];
    if (f.openMode == MODE_READ_ONLY || f.inode == -1) {
		_result = FS_ERROR;
        return;
    }
	
    updateDirectory();
	updateFreeList();
	saveInode(f.inodeBuffer, f.inode);
}

// Read data from the file.
void readFile(int fp, void *buffer, unsigned int dataSize, unsigned int dataCount)
{
	TOpenFile f = _oft[fp];
    if (dataSize <= 0 || f.inode == -1) {
		_result = FS_ERROR;
        return;
    }
  
    unsigned int total = dataSize * dataCount;
    unsigned int remaining = total, lenToReadFromThisBlock;
    unsigned long blockNumber;
	
	while(remaining > 0) {
		blockNumber = returnBlockNumFromInode(f.inodeBuffer, f.filePtr);
		if(blockNumber == 0){
			memset((char *)buffer + total - remaining, 0, f.blockSize);
		}else{
			readBlock(f.buffer, blockNumber);
		}
		
		lenToReadFromThisBlock = f.blockSize - f.readPtr < remaining ?
								  (f.blockSize - f.readPtr) : remaining;
		memcpy((char *)buffer + total - remaining, 
		       f.buffer, 
		       lenToReadFromThisBlock);

		f.filePtr = f.filePtr + lenToReadFromThisBlock;
		f.readPtr = (f.readPtr + lenToReadFromThisBlock) % f.blockSize;
		remaining -= lenToReadFromThisBlock;
	}
}

// Delete the file. Read-only flag (bit 2 of the attr field) in directory listing must not be set. 
// See TDirectory structure.
void delFile(const char *filename) {
	if (strlen(filename) > MAX_FNAME_LEN) {
		_result = FS_ERROR;
		return;
	}
	
    unsigned int index = findFile(filename);
    if (_result == FS_OK) {
		unsigned int attr = getAttr(filename);
		bool isReadOnly = attr & 0x04;
		if(isReadOnly) {
			_result = FS_ERROR;
			return;
		} else {
			unsigned long *inodeBuffer = makeInodeBuffer();
			// make an empty buffer
			char * dataBuffer = makeDataBuffer();
			loadInode(inodeBuffer, index);
			for(int i = 0;i < _fs->numInodeEntries; i++){
				if(inodeBuffer[i] != 0) {			
					// clear this block
					writeBlock(dataBuffer, inodeBuffer[i]);
					markBlockFree(inodeBuffer[i]);
					inodeBuffer[i] = 0;
				}
			}
			updateFreeList();
			saveInode(inodeBuffer, index);
			delDirectoryEntry(filename);
			updateDirectory();
		}
	} 
	
	return;
}

// Close a file. Flushes all data buffers, updates inode, directory, etc.
void closeFile(int fp) {
	flushFile(fp);
	
	// mark as closed
	_oft[fp].inode = -1;
}


// Unmount file system.
void closeFS() {
	for(int i = 0; i < _oftCount; i++){
		if (_oft[i].inode != -1) {
			closeFile(i);
		}
	}
	
    for(int i = 0; i < _fs->maxFiles; i++){
		free(filenames[i]);
	}
	free(filenames);
	
    free(_oft);
    
    unmountFS();
}
