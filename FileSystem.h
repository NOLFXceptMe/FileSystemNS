#include"Disk.h"
#include"misc.h"

#ifndef _FILE_SYSTEM_H_
#define _FILE_SYSTEM_H_

class  FileSystem{
	public:
		/* Constructor */
		FileSystem();

		int formatDisk();
		int shutdown();
		int create();
		int inumber(int fd);
		int open(int inumber);
		int read(int fd, byte* buffer, int length);
		int write(int fd, byte* buffer, int length);
		int seek(int fd, int offset, int whence);
		int close(int fd);
		int del(int inumber);


	private:
		int allocate();
		int free(int blockNo);

		Disk* disk;
		static const int ISIZE = 4;
		static const int MAX_FILES = 16;

		/* All data structures that follow are in-memory */
		SuperBlock superBlock;
		InodeBlock inodeBlockTable[ISIZE];
		int fileDescTable[MAX_FILES];
		int seekPointer[MAX_FILES];
};

#endif /* _FILE_SYSTEM_H_ */
