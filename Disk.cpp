#include<iostream>
#include<fstream>

#include"Disk.h"
#include"Inode.h"
#include"SuperBlock.h"
#include"misc.h"

using namespace std;

Disk::Disk()
{
	disk_int = new fstream();
	//disk_int->open("DISK", fstream::out|fstream::in|fstream::binary);
	disk_int->open("DISK", fstream::out|fstream::in);

	if(!disk_int->is_open())
	{
		disk_int->clear();
		//disk_int->open("DISK",fstream::out|fstream::binary);
		disk_int->open("DISK",fstream::out);
		for(int i=0; i<BLOCK_SIZE; ++i)
			buf_int[i]=0;

		for(int i = 0; i < NUM_BLOCKS; i++)
			write(i, buf_int);
		disk_int->close();
		//disk_int->open("DISK", fstream::out|fstream::in|fstream::binary);
		disk_int->open("DISK", fstream::out|fstream::in);
	}
	
	/*
	if(disk_int->is_open())
		cout<<"open"<<endl;
	else cout << "not open" << endl;
	*/

}

void Disk::test()
{
	byte buffer[] = "Blah";

	write(5, buffer);
	cout<<"Written"<<endl;

	byte inbuf[20];
	read(5, inbuf);
	cout<<"Read  "<<inbuf<<endl;
}

void Disk::read(int blockNo, byte* buffer)
{
	disk_int->seekg(blockNo * BLOCK_SIZE);
	disk_int->read(buffer, BLOCK_SIZE);
}

void Disk::read(SuperBlock* block)
{
	int temp[3];
	disk_int->seekg(0);
	disk_int->read((char*)temp, 3*sizeof(int));
	
	block->size = temp[0];
	block->isize = temp[1];
	block->freeList = temp[2];
}

void Disk::read(int blockNo, InodeBlock* inodeBlock)
{
	disk_int->seekg(blockNo * Disk::BLOCK_SIZE);

	Inode* curInode;
	for(int i=0; i<InodeBlock::SIZE; ++i){
		curInode = &(inodeBlock->node[i]);
		disk_int->read((char *)(&curInode->flags), sizeof(int));
		disk_int->read((char *)(&curInode->owner), sizeof(int));
		disk_int->read((char *)(&curInode->size), sizeof(int));
		for(int j=0; j<13; ++j)
			disk_int->read((char *)(&(curInode->ptr[j])), sizeof(int));
	}
}

void Disk::read(int blockNo, IndirectBlock* indirectBlock)
{
	disk_int->seekg(blockNo * Disk::BLOCK_SIZE);

	for(int i=0;i<IndirectBlock::SIZE; ++i){
		disk_int->read((char *)(&(indirectBlock->ptr[i])), sizeof(int));
	}
}

void Disk::write(int blockNo, byte* buffer)
{
	disk_int->seekp(blockNo * BLOCK_SIZE);
	disk_int->write(buffer, BLOCK_SIZE);
}

void Disk::write(SuperBlock* block)
{
	disk_int->seekp(0);
	disk_int->write((char*)(&(block->size)), sizeof(int));
	disk_int->write((char*)(&(block->isize)), sizeof(int));
	disk_int->write((char*)(&(block->freeList)), sizeof(int));
}

void Disk::write(int blockNo, InodeBlock* inodeBlock)
{
	disk_int->seekp(blockNo * Disk::BLOCK_SIZE);

	Inode* curInode;
	for(int i=0; i<InodeBlock::SIZE; ++i){
		curInode = &(inodeBlock->node[i]);
		disk_int->write((char *)(&curInode->flags), sizeof(int));
		disk_int->write((char *)(&curInode->owner), sizeof(int));
		disk_int->write((char *)(&curInode->size), sizeof(int));
		for(int j=0; j<13; ++j)
			disk_int->write((char *)(&(curInode->ptr[j])), sizeof(int));
	}
}

void Disk::write(int blockNo, IndirectBlock* indirectBlock)
{
	disk_int->seekp(blockNo * Disk::BLOCK_SIZE);

	for(int i=0;i<IndirectBlock::SIZE; ++i){
		disk_int->write((char *)(&(indirectBlock->ptr[i])), sizeof(int));
	}
}
void Disk::stop(bool removeDisk)
{
	disk_int->close();

	if(removeDisk)
		remove("DISK");
}
