/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Kyle Petkovic, Matthew Hernandez, Nicholas Pagcanlungan, Neal Olorvida
* Student IDs: 923683186, 920214682, 922298361, 915905996
* GitHub Name: kylepet
* Group Name: Compiler Errors
* Project: Basic File System
*
* File: initSpace.h
*
* Description: This is where we define the volume control block struct
*			   Also decares the global vcb variable.
* 
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freeSpace.h"
#include "fsLow.h"

typedef struct VCB{
	char signature[21]; // Know if you need to format

	unsigned int numBlocks;
    unsigned int freeBlocks; // numBlocks - freeBlocks can be used to get the location of the next free block.
    unsigned int blockSize;
	unsigned int rootDirLoc;

	int freeSpaceBitmapLength;
	int FATlength;

	// FAT and freeSpace bitmap pointer created at runtime
	int* freeSpace;
	FATentry *FATtable;

	int nextFreeBlock;
}VCB;

extern VCB *vcbGlobal;
