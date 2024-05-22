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
* Description: Inititializes directories
*
* This file is where you will start and initialize your system
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "initDir.h"
#include "fsLow.h"
#include "freeSpace.h"
#include "initSpace.h"
#include "mfs.h"

int initDir(int defaultEntries, DE * parent, int blockSize) { // pass null if root
	// malloc memory for an array of directory entries
	//DE * directoryEntry[50];
	//int blockSize = 6;
	//int bytesNeeded = defaultEntries * sizeof(directoryEntry); // directoryEntry = DE
	//int blocksNeeded = (bytesNeeded + (blockSize - 1)) / blockSize;
	//bytesNeeded = blocksNeeded * blockSize;

	int blocksNeeded = (defaultEntries * sizeof(DE) + blockSize - 1)/ blockSize;

	DE *dir = malloc(blockSize * blocksNeeded);

	// error check code inserted here
	if (dir == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		return -1;
		
	}
		
	time_t currentTime = time(NULL);

	for (int i = 0; i < defaultEntries; i++) {
		strcpy(dir[i].name, "\0");
		dir[i].size = 0;
		dir[i].loc = -1;
		dir[i].isDir = -1;
		dir[i].accessed = currentTime;
		dir[i].modified = currentTime;
		dir[i].create = currentTime;

	}
	
	int* nextFreeBlocks = allocFreeBlocks(blocksNeeded);

	strcpy(dir[0].name, ".");
	dir[0].size = defaultEntries * sizeof(DE);
	dir[0].loc = nextFreeBlocks[0];
	dir[0].isDir = 1;
	dir[0].blocks = blocksNeeded;

	DE * p = &dir[0];

	if(parent != NULL) {
		p = parent;
	} else {
		// In root dir, so save loc to VCB
		vcbGlobal->rootDirLoc = dir[0].loc;

		dir[1].blocks = blocksNeeded;

		g_rootDir = dir;
		g_cwd = dir;
		strcpy(currentDirectoryPath, "/");

		int write = LBAwrite(vcbGlobal, 1, 0);

		if (write < 0){ 
				printf("ERROR: Failed to write block\n");
				return -1;
		}

		p = &dir[0];
	}

	strcpy(dir[1].name, "..");
	dir[1].size = p -> size;
	dir[1].loc = p -> loc;
	dir[1].isDir = p -> isDir;
	dir[1].create = p -> create;
	dir[1].modified = p -> modified;
	dir[1].accessed = p -> accessed;
	

	// Write to disk
	writeData(nextFreeBlocks, blocksNeeded, dir);

	return nextFreeBlocks[0];
}
