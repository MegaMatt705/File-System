/**************************************************************
* Class:  CSC-415-03 Fall 2021
* Names: TODO
* Student IDs: TODO
* GitHub Name: TODO
* Group Name: Compiler Errors
* Project: Basic File System
*
* File: freeSpace.h
*
* Description: Interface for initializing the freespace and the 
*			   bitmap for tracking space.
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"

#ifndef FREE_SPACE_FAT
#define FREE_SPACE_FAT
typedef struct FATentry {
	// on EOF this is 0
	unsigned int nextBlock;
} FATentry;
#endif 

struct VCB;

// Bit operations for freespace
void setBit( int fs[],  int k);
void clearBit( int fs[],  int k);
int getBit( int fs[],  int k );

int getFreeBlock();
int* allocFreeBlocks(int numOfBlocks);
int setChunk(int fs[], int from, int to);
int writeData(int* blocksToWrite, int blocks, void* buffer);
int overwriteData(int start, int blocksNeeded, void* buffer);
int readData(int startBlock, void* buffer);
int unallocateFolder(int startBlock);
int printBitmap( int fs[]);

int initFreeSpace (uint64_t numberOfBlocks, uint64_t blockSize, unsigned int startWriteFrom);
int initFAT(int numberOfBlocks, int blockSize);