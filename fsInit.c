/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Kyle Petkovic, Matthew Hernandez, Nicholas Pagcanlungan, Neal Olorvida
* Student IDs: 923683186, 920214682, 922298361, 915905996
* GitHub Name: kylepet
* Group Name: Compiler Errors
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Initializes the filesystem if none exists, 
* 			   otherwise it will update the pointers to the
*              free space and FAT table structures.
*
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "initSpace.h" // VCB struct written here
#include "freeSpace.h" // initFAT() and initFreeSpace protoytypes
#include "initDir.h"

char ourMagicNumber[] = "NealKyleMattNicholas"; //TODO: consider changing this
VCB *vcbGlobal;
DE *g_rootDir;
DE *g_cwd;

char currentDirectoryPath[MAX_FILEPATH_SIZE];

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize){
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */

	/* malloc a block of memory as your VCB pointer */
	vcbGlobal = (VCB *)malloc(blockSize);

	int blocksNeeded = (sizeof(DE) * MAX_DIR_ENTRIES + blockSize - 1) / blockSize;
	g_cwd = (DE*) malloc(blocksNeeded * vcbGlobal->blockSize);
	g_rootDir = (DE*) malloc(blocksNeeded * vcbGlobal->blockSize);

	if (vcbGlobal == NULL){ // Error condition if malloc fails
		printf("ERROR: Memory allocation failed for VCB\n");
		return -1;
	}

	/* LBAread block 0 
	   First argument is vcbGlobal, second is for reading (1) block, third is the zeroth (0) position. */
	uint64_t readResult = LBAread(vcbGlobal, 1, 0);

	if (readResult != 1){ // Error condition if failure to read from block 0
		printf("ERROR: Failure to read block 0\n");
		free(vcbGlobal);
		return -1;
	}

	/* Look at the signature (magic number) in structure and see if it matches. */
	if(strcmp(vcbGlobal->signature, ourMagicNumber) == 0){
		//init pointers

		// Free Space bmp
		vcbGlobal->freeSpace = (int *) malloc(vcbGlobal->freeSpaceBitmapLength * vcbGlobal->blockSize);
		int freeSpaceRead = LBAread(vcbGlobal->freeSpace, 
			vcbGlobal->freeSpaceBitmapLength, 1 + vcbGlobal->FATlength);
		if (freeSpaceRead < 0){ 
			printf("ERROR: Failure to read block 0\n");
			free(vcbGlobal);
			return -1;
		}

		

		vcbGlobal->FATtable = (FATentry*) malloc(vcbGlobal->FATlength * blockSize);
		int FATread = LBAread(vcbGlobal->FATtable, vcbGlobal->FATlength, 1);
		if (FATread < 0){ 
			printf("ERROR: Failure to read block 0\n");
			free(vcbGlobal);
			return -1;
		}

		

		// New pointers created so they can be used later. Store them to disk
		uint64_t writeResult = LBAwrite(vcbGlobal, 1, 0);
		if (writeResult < 0){ 
				printf("ERROR: Failed to write block\n");
				free(vcbGlobal);
				return -1;
		}

		

		// Init locations needed for navigation
		// Machine just started, so root and dir will both be the root
		
		readData(vcbGlobal->rootDirLoc, g_rootDir);
		readData(vcbGlobal->rootDirLoc, g_cwd);

		
		strcpy(currentDirectoryPath, "/");

		printf("Volume is already initialized.\n");		
	} else {
		/* If the signature does not match our static number,
		   then we must initialize values in our Volume Control Block */

		printf("Initializing Filesystem\n");

		// Init Free space and FAT
		int fatSize = initFAT(numberOfBlocks, blockSize);
		int startWritePos = initFreeSpace(numberOfBlocks, blockSize, fatSize);

		vcbGlobal->numBlocks = numberOfBlocks;
		vcbGlobal->freeBlocks = numberOfBlocks;
		vcbGlobal->blockSize = blockSize;
		strcpy(vcbGlobal->signature, ourMagicNumber);
		vcbGlobal->nextFreeBlock = startWritePos;
		vcbGlobal->FATlength = fatSize;
		vcbGlobal->freeSpaceBitmapLength = startWritePos - 1 - fatSize;


		vcbGlobal->freeSpace = (int *) malloc(vcbGlobal->freeSpaceBitmapLength * vcbGlobal->blockSize);

		int freeSpaceRead = LBAread(vcbGlobal->freeSpace, 
			vcbGlobal->freeSpaceBitmapLength, 1 + vcbGlobal->FATlength);

		if (freeSpaceRead < 1){ // if there was no blocks read
			printf("ERROR: Failure to read block 0\n");
			free(vcbGlobal);
			return -1;
		}

		vcbGlobal->FATtable = (FATentry*) malloc(fatSize * blockSize);
	
		int FATread = LBAread(vcbGlobal->FATtable, vcbGlobal->FATlength, 1);

		if (FATread < 0){ 
			printf("ERROR: Failure to read block 0\n");
			free(vcbGlobal);
			return -1;
		}

		initDir(MAX_DIR_ENTRIES, NULL, blockSize);
		

		/* LBAwrite the VCB to block 0
		   First argument is vcbGlobal, second is for reading (1) block, third is the zeroth (0) position. */
		uint64_t writeResult = LBAwrite(vcbGlobal, 1, 0); // Q: What do I do with the return value?

		if (writeResult < 0){ 
				printf("ERROR: Failed to write block\n");
				free(vcbGlobal);
				return -1;
		}
	}

	return 0;
}
	
void exitFileSystem (){

	// When initalizing the filesystem g_rootDir and g_cwd are the same
	// Preform pointer equality to see if both need to be freed or just one
	if(g_rootDir != g_cwd) {
        free(g_rootDir);
    }
	free(g_cwd);
    
	free(vcbGlobal->FATtable);
	free(vcbGlobal->freeSpace);
	free(vcbGlobal);

	printf ("System exiting\n");
}