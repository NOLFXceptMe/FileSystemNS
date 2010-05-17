#include<iostream>
#include<cstring>

#include"Disk.h"
#include"FileSystem.h"

FileSystem::FileSystem()
{
	disk = new Disk();

	disk->read(&superBlock);
	
	for(int i = 0; i < ISIZE; i++)
	{
		disk->read(i+1,&inodeBlockTable[i]);
	}	
	//disk->test();
}

int FileSystem::formatDisk()
{
	superBlock.size = Disk::NUM_BLOCKS;
	superBlock.isize = ISIZE;
	superBlock.freeList = ISIZE + 1;

	disk->write(&superBlock);

	/* Start allocating the free list
	 * We really don't need numBlocksNeeded but then, it would cost us atmost 1 block extra, so we don't care
	 */
	int numBlocksNeeded = Disk::NUM_BLOCKS/Disk::POINTERS_PER_BLOCK;
	int freeBlockIndex = ISIZE + 1;
	int freeBlockCounter = ISIZE + 1 + numBlocksNeeded;
	
	IndirectBlock inode;
	for(int i = 0; i < Disk::POINTERS_PER_BLOCK; i++)
		inode.ptr[i] = 0;

	for(int i= 0; i < ISIZE; i++)
	{
		disk->write(i+1, &inode);
		disk->read(i+1, &inodeBlockTable[i]);
	}
	
	IndirectBlock freeBlock;
	while(freeBlockIndex<Disk::NUM_BLOCKS){
		for(int i=0; i<Disk::POINTERS_PER_BLOCK ; ++i){
			if(freeBlockCounter<Disk::NUM_BLOCKS){
				freeBlock.ptr[i] = freeBlockCounter;
				//cerr<<"For block index "<<freeBlockIndex<<"writing "<<freeBlockCounter<<endl;
			}
			else
				freeBlock.ptr[i] = 0;
			freeBlockCounter++;
		}
		disk->write(freeBlockIndex, &freeBlock);
		freeBlockIndex++;
	}

	/* Now initialize File Descriptor Table */
	for(int i=0; i<MAX_FILES; ++i)
		fileDescTable[i] = 0;

	return 0;
}

int FileSystem::inumber(int fd)
{
	if(fileDescTable[fd] != 0)
		return fileDescTable[fd];
	else
		return -1;
}

int FileSystem::open(int inumber)
{
	/* Scan the File Descriptor Table for an empty entry and return the file descriptor */
	for(int i=0; i<MAX_FILES; ++i){
		if(fileDescTable[i] == 0){
			fileDescTable[i] = inumber;
			seekPointer[i] = 0;

			/* Locate which Inode Block this Inode can be found in */
			int inodeBlockNumber = ((inumber-1)/4) + 1;
			/* Write into memory */
			disk->read(inodeBlockNumber, &(inodeBlockTable[inodeBlockNumber-1]));
		
			return i;
		}
	}

	/* Failed */
	return -1;
}

int FileSystem::close(int fd)
{
	int inumber = fileDescTable[fd];

	/* Locate which Inode Block this Inode can be found in */
	int inodeBlockNumber = ((inumber-1)/4) + 1;

	/* Write back to disk*/
	disk->write(inodeBlockNumber, &(inodeBlockTable[inodeBlockNumber]));

	fileDescTable[fd] = 0;
	return 0;
}

int FileSystem::seek(int fd, int offset, int whence)
{
	switch(whence){
		case SEEK_SET:
			seekPointer[fd] = offset;
			return seekPointer[fd];
		case SEEK_CUR:
			seekPointer[fd] += offset;
			return seekPointer[fd];
		case SEEK_END:
			//seekPointer[fd] = file_size + offset;
			break;
	}
	return -1;
}

int FileSystem::read(int fd, byte* buffer, int length)
{
	int inumber = fileDescTable[fd];
	int inodeBlockNumber = ((inumber-1)/4);

	InodeBlock inodeBlock;
	Inode inode = inodeBlockTable[inodeBlockNumber].node[(inumber-1)%4];

	cout<<inode.ptr[0];
	cerr<<"Found inode in block number "<<inodeBlockNumber<< " length "<< length<<" for file "<< fd << " seek " <<seekPointer[fd] <<endl;
	int seekPtr = seekPointer[fd];
	if(seekPtr<0)
		return -1;
//	else if(seekPtr > inode.size)
//		return 0;

	int origLength = length;
//	if(length>inode.size-seekPtr){
//		length = inode.size-seekPtr;
//	}

	int blockNo;
	IndirectBlock indirectBlock;
	char buf_filesys[Disk::BLOCK_SIZE];
	int pos = 0;
	
	while(length > 0){
		/* Locate the block on the disk */
		if(seekPtr<10*Disk::BLOCK_SIZE)
			blockNo = inode.ptr[seekPtr/Disk::BLOCK_SIZE];
		cout<<blockNo<<endl;
		seekPtr -= 10*Disk::BLOCK_SIZE;

		if(seekPtr>0 && seekPtr<64*Disk::BLOCK_SIZE){
			disk->read(inode.ptr[10], &indirectBlock);
			blockNo = indirectBlock.ptr[seekPtr/Disk::BLOCK_SIZE]; 
		}
		seekPtr -= 64*Disk::BLOCK_SIZE;

		if(seekPtr>0 && seekPtr<64*64*Disk::BLOCK_SIZE){
			disk->read(inode.ptr[11], &indirectBlock);
			disk->read(indirectBlock.ptr[seekPtr/(64*Disk::BLOCK_SIZE)], &indirectBlock);
			blockNo = indirectBlock.ptr[(seekPtr%(64*Disk::BLOCK_SIZE))/Disk::BLOCK_SIZE];
		}
		seekPtr -= 64*64*Disk::BLOCK_SIZE;

		if(seekPtr>0){
			disk->read(inode.ptr[12], &indirectBlock);
			disk->read(indirectBlock.ptr[(seekPtr/(Disk::BLOCK_SIZE*64*64))], &indirectBlock);
			disk->read(indirectBlock.ptr[(seekPtr%(Disk::BLOCK_SIZE*64*64))/(64*Disk::BLOCK_SIZE)], &indirectBlock);
			blockNo = indirectBlock.ptr[(seekPtr%(64*Disk::BLOCK_SIZE))/Disk::BLOCK_SIZE];
		}
		cout << "Block no found " << blockNo << endl;
		if(blockNo < 0)
			return -1;

		disk->read(blockNo, buf_filesys);

		int offset = seekPointer[fd]%Disk::BLOCK_SIZE;
		cout << "data read" << &buf_filesys[offset] << "done" << endl;
		if(length<Disk::BLOCK_SIZE){
			memcpy(&buffer[pos], &buf_filesys[offset], length);
			seekPointer[fd] += length;
		}
		else{
			memcpy(&buffer[pos], &buf_filesys[offset], Disk::BLOCK_SIZE - offset);
			seekPointer[fd] += Disk::BLOCK_SIZE - offset;
		}

		pos += Disk::BLOCK_SIZE;
		seekPtr = seekPointer[fd];
		length -= Disk::BLOCK_SIZE;
	}

	return origLength;
}

int FileSystem::write(int fd, byte* buffer, int length)
{
	cout<<endl<<"Writing into file with fd "<<fd<<", data is \""<<buffer<<"\" upto length "<<length<<endl;
	int inumber = fileDescTable[fd];
	int inodeBlockNumber = ((inumber-1)/4);
	Inode inode = inodeBlockTable[inodeBlockNumber].node[(inumber-1)%4];

	cout<<endl<<"Looking for inode"<<endl;
	cout<<"Found inode in inodeBlockNumber "<<inodeBlockNumber<<" for file "<<fd<<endl;
	int seekPtr = seekPointer[fd];
	cout<<"Value of seekPtr is "<<seekPtr<<endl<<endl;
	if(seekPtr<0)
		return -1;
//	else if(seekPtr > inode.size)
//		return 0;

	int origLength = length;
	/*
	if(length>inode.size-seekPtr){
		length = inode.size-seekPtr;
	}
	*/

	int blockNo;
	IndirectBlock indirectBlock;
	char buf_filesys[Disk::BLOCK_SIZE];
	int pos = 0;
	int offset;
	
	while(length > 0){
		/* Locate the block on the disk */
		if(seekPtr<10*Disk::BLOCK_SIZE){
			blockNo = inode.ptr[seekPtr/Disk::BLOCK_SIZE];
			if(blockNo <= 0){
				cout<<"Found no data block to write into, allocating new block"<<endl;
				blockNo = allocate();
				cout<<"Allocated block "<<blockNo<<" as a data block"<<endl;
				inode.ptr[seekPtr/Disk::BLOCK_SIZE] = blockNo;
				inodeBlockTable[inodeBlockNumber].node[(inumber -1)%4] = inode;
				disk->write(inodeBlockNumber+1, &inodeBlockTable[inodeBlockNumber]);

			}
		}
		seekPtr -= 10*Disk::BLOCK_SIZE;

		if(seekPtr>0 && seekPtr<64*Disk::BLOCK_SIZE){
			if(inode.ptr[10] == 0){
				cout<<"Found no data block to write into, allocating new block. This is a single indirect block"<<endl;
				inode.ptr[10] = allocate();
				cout<<"Allocated block "<<inode.ptr[10]<<" as an indirection block"<<endl;
			}
			disk->read(inode.ptr[10], &indirectBlock);
			blockNo = indirectBlock.ptr[seekPtr/Disk::BLOCK_SIZE]; 
			if(blockNo <= 0){
				cout<<"Found no data block to write into, allocating new block. This is a data block"<<endl;
				blockNo = allocate();
				cout<<"Allocated block "<<blockNo<<" as a data block"<<endl;
				indirectBlock.ptr[seekPtr/Disk::BLOCK_SIZE] = blockNo;
				disk->write(inode.ptr[10], &indirectBlock);
			}

		}
		seekPtr -= 64*Disk::BLOCK_SIZE;

		if(seekPtr>0 && seekPtr<64*64*Disk::BLOCK_SIZE){
			if(inode.ptr[11] == 0){
				inode.ptr[11] = allocate();
			}
			disk->read(inode.ptr[11], &indirectBlock);
			if(indirectBlock.ptr[seekPtr/(64*Disk::BLOCK_SIZE)] == 0){
				indirectBlock.ptr[seekPtr/(64*Disk::BLOCK_SIZE)] = allocate();
				disk->write(inode.ptr[11], &indirectBlock);
			}
			disk->read(indirectBlock.ptr[seekPtr/(64*Disk::BLOCK_SIZE)], &indirectBlock);
			blockNo = indirectBlock.ptr[(seekPtr%(64*Disk::BLOCK_SIZE))/Disk::BLOCK_SIZE];
			if(blockNo <= 0){
				blockNo = allocate();
				indirectBlock.ptr[(seekPtr%(64*Disk::BLOCK_SIZE))/Disk::BLOCK_SIZE] = blockNo;
				disk->write(indirectBlock.ptr[seekPtr/(64*Disk::BLOCK_SIZE)], &indirectBlock);
			}
		}
		seekPtr -= 64*64*Disk::BLOCK_SIZE;

		if(seekPtr>0){
			if(inode.ptr[12] == 0){
				inode.ptr[12] = allocate();
			}
			disk->read(inode.ptr[12], &indirectBlock);
			if(indirectBlock.ptr[(seekPtr/(Disk::BLOCK_SIZE*64*64))] == 0){
				indirectBlock.ptr[(seekPtr/(Disk::BLOCK_SIZE*64*64))] = allocate();
				disk->write(inode.ptr[12], &indirectBlock);
			}
			disk->read(indirectBlock.ptr[(seekPtr/(Disk::BLOCK_SIZE*64*64))], &indirectBlock);
			if(indirectBlock.ptr[(seekPtr%(Disk::BLOCK_SIZE*64*64))/(64*Disk::BLOCK_SIZE)] == 0){
				indirectBlock.ptr[(seekPtr%(Disk::BLOCK_SIZE*64*64))/(64*Disk::BLOCK_SIZE)] = allocate();
				disk->write(indirectBlock.ptr[(seekPtr/(Disk::BLOCK_SIZE*64*64))], &indirectBlock);
			}
			disk->read(indirectBlock.ptr[(seekPtr%(Disk::BLOCK_SIZE*64*64))/(64*Disk::BLOCK_SIZE)], &indirectBlock);
			blockNo = indirectBlock.ptr[(seekPtr%(64*Disk::BLOCK_SIZE))/Disk::BLOCK_SIZE];
			if(blockNo <= 0){
				blockNo = allocate();
				indirectBlock.ptr[(seekPtr%(64*Disk::BLOCK_SIZE))/Disk::BLOCK_SIZE] = blockNo;
				disk->write(indirectBlock.ptr[(seekPtr%(Disk::BLOCK_SIZE*64*64))/(64*Disk::BLOCK_SIZE)], &indirectBlock);
			}
		}

		disk->read(blockNo, buf_filesys);

		offset = seekPointer[fd]%Disk::BLOCK_SIZE;
		if(length<Disk::BLOCK_SIZE){
			memcpy((char *)&buf_filesys[offset], (char *)&buffer[pos], length);
			cout<<buf_filesys<<endl;
			seekPointer[fd] += length;
		}
		else{
			memcpy(&buf_filesys[offset], &buffer[pos], Disk::BLOCK_SIZE - offset);
			seekPointer[fd] += Disk::BLOCK_SIZE - offset;
		}

		cerr<<"Writing \""<<&buf_filesys[offset]<<"\" at block number "<<blockNo<<endl;
		disk->write(blockNo, buf_filesys);

		pos += Disk::BLOCK_SIZE;
		seekPtr = seekPointer[fd];
		length -= Disk::BLOCK_SIZE;
	}

	inodeBlockTable[inodeBlockNumber].node[(inumber-1)%4] = inode;
	disk->write(inodeBlockNumber+1, &inodeBlockTable[inodeBlockNumber]);

	byte buf_test[256];
	disk->read(blockNo, buf_test);
	cout<<"Verifying written data"<<endl;
	cout<<"Read back \""<<&buf_test[offset]<<"\" from block "<<blockNo<<endl<<endl;

	//Dump disk blocks just for verification
	cout<<endl<<endl<<"Dumping first 30 blocks on the disk, each on a separate line"<<endl<<
		"Note that first line would signify SuperBlock"<<endl<<
		"The next four lines denote the InodeBlocks"<<endl<<
		"The following lines (upto 16 lines) show the free list blocks"<<endl<<
		"Rest are either indirect or data blocks. In our test case, note that we show one indirect block and one data block"<<endl<<endl;
	cout<<"--- Dump begins ---"<<endl<<endl;
	IndirectBlock test;

	for(int i = 0; i < 30; i++){
		disk->read(i, &test);
		for(int j = 0; j < Disk::POINTERS_PER_BLOCK; j++)
			cout << test.ptr[j] << " ";
		cout << endl;
	}
	cout<<endl<<"--- Dump ends ---"<<endl;
	cout<<endl<<"The line beginning with 22 is the indirect block which is referred to by the inode contained in the inode block shown in the second line of the dump. Note that this is the 21st block on the disk"<<endl;
       	cout<<"The next line, the 22nd block, is the actual data block. The huge numbers shown are due to treating 4 characters as an int during the dump."<<endl;
	//end test 


	return origLength;
}

int FileSystem::allocate()
{
	int freeList = superBlock.freeList;
	cout<<"Freelist begins at "<<freeList<<endl;
	IndirectBlock freeListBlock;
	disk->read(freeList, &freeListBlock);

	int freeBlock = 0;
	int counter = 0;
	while(freeBlock == 0){
		if(freeListBlock.ptr[counter] != 0){
			freeBlock = freeListBlock.ptr[counter];
			cout<<"Found a free block "<<freeBlock<<" in block "<<counter<<" of free list"<<endl;
			freeListBlock.ptr[counter] = 0;
			disk->write(freeList, &freeListBlock);
		}
		counter+=1;
		if(counter == Disk::POINTERS_PER_BLOCK){
			freeList+=1;
			disk->read(freeList, &freeListBlock);
		}
		if(freeList == 1 + ISIZE + Disk::NUM_BLOCKS/Disk::POINTERS_PER_BLOCK)
			return -1;

		counter=counter%Disk::POINTERS_PER_BLOCK;
	}

	char buffer[Disk::BLOCK_SIZE] = {0};
	disk->write(freeBlock, buffer);

	return freeBlock;
}

int FileSystem::free(int blockNo)
{
	int freeBlockIndex = (blockNo - (1+ISIZE+Disk::NUM_BLOCKS/Disk::POINTERS_PER_BLOCK))/Disk::POINTERS_PER_BLOCK;
	int freeList = superBlock.freeList;
	IndirectBlock freeListBlock;
	disk->read(freeList+freeBlockIndex, &freeListBlock);

	freeListBlock.ptr[(blockNo - (1+ISIZE+Disk::NUM_BLOCKS/Disk::POINTERS_PER_BLOCK))%Disk::POINTERS_PER_BLOCK] = blockNo;
	disk->write(freeList+freeBlockIndex, &freeListBlock);

	return 0;
}

int FileSystem::del(int fd)
{
	int inumber = fileDescTable[fd];
	int inodeBlockNumber = ((inumber-1)/4);
	Inode inode = inodeBlockTable[inodeBlockNumber].node[(inumber-1)%4];
	IndirectBlock indirectBlock;
	IndirectBlock secIndirectBlock;
	IndirectBlock tripleIndirectBlock;

	for(int i = 0; i<10; ++i){
		if(inode.ptr[i] != 0)
			free(inode.ptr[i]);
	}

	if(inode.ptr[10] != 0){
		disk->read(inode.ptr[10], &indirectBlock);
		for(int i = 0; i<IndirectBlock::SIZE; ++i){
			if(indirectBlock.ptr[i]!=0)
				free(indirectBlock.ptr[i]);
		}
		free(inode.ptr[10]);
	}

	if(inode.ptr[11] != 0){
		disk->read(inode.ptr[11], &indirectBlock);
		for(int i = 0; i<IndirectBlock::SIZE; ++i)
			if(indirectBlock.ptr[i] != 0){
				disk->read(indirectBlock.ptr[i], &secIndirectBlock);
				for(int j = 0; j<IndirectBlock::SIZE; ++j)
					if(secIndirectBlock.ptr[j] != 0)
						free(secIndirectBlock.ptr[j]);	
				free(indirectBlock.ptr[i]);
			}
		free(inode.ptr[11]);
	}

	if(inode.ptr[12] != 0){
		disk->read(inode.ptr[12], &indirectBlock);
		for(int i = 0; i<IndirectBlock::SIZE; ++i)
			if(indirectBlock.ptr[i] != 0){
				disk->read(indirectBlock.ptr[i], &secIndirectBlock);
				for(int j = 0; j<IndirectBlock::SIZE; ++j)
					if(secIndirectBlock.ptr[j]!=0){
						disk->read(secIndirectBlock.ptr[j], &tripleIndirectBlock);
						for(int k=0; k<IndirectBlock::SIZE; ++k)
							if(tripleIndirectBlock.ptr[k]!=0)
								free(tripleIndirectBlock.ptr[k]);
						free(secIndirectBlock.ptr[j]);
					}
				free(indirectBlock.ptr[i]);
			}
		free(inode.ptr[12]);
	}
	inodeBlockTable[inodeBlockNumber].node[(inumber-1)%4] = inode;
	return 0;
}

int FileSystem::create()
{
	int inode;
	InodeBlock inodeBlock;

	/* Search existing Inode Block Table for free entries */
	for(int i=0; i<ISIZE; ++i){
		inodeBlock = inodeBlockTable[i];
		for(int j=0; j<4; ++j)
			if(inodeBlock.node[j].flags ==0){
				inodeBlock.node[j].flags = 1;
				inodeBlock.node[j].owner = 1;
				inodeBlock.node[j].size = 0;

				inode = i*4+j+1;
				cout<<"Creating file with inode "<<inode<<endl;
				cout<<"Inserting in inodeBlock "<<i<<endl;
				inodeBlockTable[i] = inodeBlock;
				disk->write(i+1, &inodeBlockTable[i]);

				return inode;
			}
	}

	return -1;

	/*
	// test 		
	IndirectBlock test;

	for(int i = 0; i < 35; i++)
	{
	//cout << "est :P " << inodeBlockTable[i].node[j].flags<<endl;
	disk->read(i, &test);
	cout << "another one "<< test.ptr[1]<<endl;
	for(int j = 0; j < Disk::POINTERS_PER_BLOCK; j++)
	cout << test.ptr[j] << " ";
	cout << endl;
	}
	//end test */

}

int FileSystem::shutdown()
{
	disk->write(&superBlock);
	
	for(int i=0; i<ISIZE; ++i){
		disk->write(i+1, &inodeBlockTable[i]);
	}

	disk->stop(false);
	return 0;
}
