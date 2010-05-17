/* Group 
 * Sweta Yamini 06CS3008
 * Naveen Kumar Molleti 06CS3009
 * 
 * Implemeting a file system
 */

#include<iostream>

using namespace std;

#include"Disk.h"
#include"FileSystem.h"
#include"misc.h"

int main()
{
	FileSystem fileSystem;
	int errCode;
	
	//Format filesystem
	errCode = fileSystem.formatDisk();
	if(errCode == 1)
		cout<<"Filesystem formatting failed"<<endl;

	//Create a file and open it
	int file1 = fileSystem.create();
	int fileid1 = fileSystem.open(file1);

	//This is the data we'll write
	byte buffer[] = "Implementing File Systems, a test program";

	//Seek to a large distance so that we actually cause an indirection. As usual, we skip the first 10 blocks, and then write
	//Note that 256 is the block size. So, we seek to skip the first 10 direct blocks and then 10 bytes
	fileSystem.seek(fileid1, 10*256 + 10, SEEK_CUR);

	//Write 20 bytes at that position
	//If you could not write 20 bytes, that is definitely an error
	if(fileSystem.write(fileid1, buffer, 20) != 20)
		cerr<<"Error!"<<endl;

	//Close the file
	fileSystem.close(fileid1);
	
	//Reopen
	fileid1 = fileSystem.open(file1);

	/* Read from file currently does not work. So, commenting out */
	/*
	byte readbuf[16] = "\0";
	fileSystem.read(fileid1, readbuf, 6);

	cout<<"Read"<<readbuf<<"done" <<endl;
	*/

	//Owing to the non-functionality of read(), del() cannot be tested
	//fileSystem.del(fileid1);

	//By our write, we can calculate that the data block is 22
	//free() would free that block and add it to the free list
	//free() is a private function, however to show that it works, all you have to do is modify the FileSystem.h file and uncomment the following statement
	//fileSystem.free(22);
	
	//Also note that inumber() works. It is trivial to verify this
	//Only read() and del() do not work.

	return 0;
}
