/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Kyle Petkovic, Matthew Hernandez, Nicholas Pagcanlungan, Neal Olorvida
* Student IDs: 923683186, 920214682, 922298361, 915905996
* GitHub Name: kylepet
* Group Name: Compiler Errors
* Project: Basic File System
*
* File: freeSpace.c
*
* Description: Initializes free space for the filesystem.
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freeSpace.h"
#include "fsLow.h"
#include "initSpace.h"

void  setBit( int fs[],  int k ){
    fs[k/32] |= 1 << (k%32); 
}

void  clearBit( int fs[],  int k ){
    fs[k/32] &= ~(1 << (k%32));
}

int getBit( int fs[],  int k ){
    return ( (fs[k/32] & (1 << (k%32) )) != 0 ) ;     
}

int printBitmap( int fs[]){
    for(int i = 0; i < 300; i++){
        printf("\n bit %d is %d", i + 1, getBit(fs, i));
    }
    printf("\n");
}

// Returns the first available block. If no blocks available it will
// return -1
int getFreeBlock(){
    for(int i = 0; i < vcbGlobal->numBlocks; i++){
        if (getBit(vcbGlobal->freeSpace, i) == 0){
            setBit(vcbGlobal->freeSpace, i);
            return i;
        }
    }
    return -1;
}

int* allocFreeBlocks(int numOfBlocks){

    int* freeBlocks = (int*) malloc(sizeof(int) * numOfBlocks);
    int acqBlocks = 0;
    unsigned int prevBit = -1;
    
    for(int i = 1; i < vcbGlobal->numBlocks; i++){
        
        if (getBit(vcbGlobal->freeSpace, i) == 0){
            freeBlocks[acqBlocks] = i;
            setBit(vcbGlobal->freeSpace, i);
            if(prevBit != -1){
                vcbGlobal->FATtable[prevBit].nextBlock = (unsigned int) i;
            }
            prevBit = i;
            acqBlocks += 1;
        }
        if (acqBlocks == numOfBlocks){
            // Commit new freespace to disk
            int save = LBAwrite(vcbGlobal->freeSpace, vcbGlobal->freeSpaceBitmapLength, 1 + vcbGlobal->FATlength);
            if (save < 0){
                perror("Failed to save to disk");
            }
            
            // Update last bit to be EOF
            vcbGlobal->FATtable[i].nextBlock = 0;

            // Commit FAT to disk
            save = LBAwrite(vcbGlobal->FATtable, vcbGlobal->FATlength, 1);
            if (save < 0){
                perror("Failed to save to disk");
            }

            return freeBlocks;
        }
    }
    return NULL;
}

// Array is the blocks to write the data to, returned from
// allocFreeBlocks
int writeData(int* blocksToWrite, int blocks, void* buffer){
    void* movedBuffer = buffer;
    for(int i = 0; i < blocks; i++){
        int res = LBAwrite(movedBuffer, 1, blocksToWrite[i]);

        if (res != 1){
            perror("Error writing to volume");
        }
        movedBuffer += vcbGlobal->blockSize;
    }
}

// Overwrite following the FAT table and getting more data if needed
int overwriteData(int start, int blocksNeeded, void* buffer){
    void* movedBuffer = buffer;
    int pos = start;
    int lastPos;
    int blocksWritten = 0;
    while(pos != 0){
        int res = LBAwrite(movedBuffer, 1, pos);
        if (res != 1){
            perror("Error writing to volume");
        }

        movedBuffer += vcbGlobal->blockSize;

        if(vcbGlobal->FATtable[pos].nextBlock == 0){
            lastPos = pos;
        }

        pos = vcbGlobal->FATtable[pos].nextBlock;
        blocksNeeded -= 1;
        blocksWritten += 1;
    }
    // Needs more data
    if(blocksNeeded != 0){
        int * blocks = allocFreeBlocks(blocksNeeded);
        // Links previous blocks to the new blocks I've just allocted
        // since extra space is required
        vcbGlobal->FATtable[lastPos].nextBlock = blocks[0];
        blocksWritten += writeData(blocks, blocksNeeded, movedBuffer);
        free(blocks);
    }
    return blocksWritten;
}

// Abstraction for LBAread to read in data using the FAT table
int readData(int startBlock, void* buffer){
    void* movedBuffer = buffer;
    int pos = startBlock;
    int totalRead = 0;
    while(pos != 0){
        
        int res = LBAread(movedBuffer, 1, pos);
        if (res != 1){
            perror("Error reading from volume");
        }

        pos = vcbGlobal->FATtable[pos].nextBlock;
    
        movedBuffer += vcbGlobal->blockSize;
        totalRead += 1;
    }
    return totalRead;
}

// Unallocate a folder
int unallocateFolder(int startBlock){
    int pos = startBlock;
    int temp = startBlock;
    int totalCleared = 0;
    while(pos != 0){
        clearBit(vcbGlobal->freeSpace, pos);

        pos = vcbGlobal->FATtable[pos].nextBlock;
        vcbGlobal->FATtable[temp].nextBlock = 0;
        temp = pos;
    
        totalCleared += 1;
    }

    // Commit freespace to disk
    int save = LBAwrite(vcbGlobal->freeSpace, vcbGlobal->freeSpaceBitmapLength, 1 + vcbGlobal->FATlength);
    if (save < 0){
        perror("Failed to save to disk");
    }

    // Commit FAT to disk
    save = LBAwrite(vcbGlobal->FATtable, vcbGlobal->FATlength, 1);
    if (save < 0){
        perror("Failed to save to disk");
    }

    return totalCleared;
}

int setChunk(int fs[], int from, int to){
	int set = 0;
	for(int i = from; i < to; i++){
		setBit(fs, i);
		set++;
	}
	return set;
}

int initFreeSpace (uint64_t numberOfBlocks, uint64_t blockSize, unsigned int FATsize){
    // 1 int is 4 bytes -> 32 bits
    int intsNeeded = (numberOfBlocks + (sizeof(int) - 1)*8) / (sizeof(int)*8);
    int blocksNeeded = ((intsNeeded * sizeof(int)) + blockSize - 1) / blockSize;

    int* freeSpace = (int*) malloc(blocksNeeded * blockSize);
    setChunk(freeSpace, 0, 1 + blocksNeeded + FATsize);
    LBAwrite(freeSpace, blocksNeeded, 1 + FATsize);

    return FATsize + blocksNeeded + 1;    
}

int initFAT(int numberOfBlocks, int blockSize){
	int FATsize = sizeof(FATentry) * numberOfBlocks;
	int blocksNeeded = (FATsize + blockSize - 1) / blockSize;
	
	FATentry *FATtable = (FATentry*) malloc(blocksNeeded * blockSize);
	
    // vcbGlobal. Never need to read the FAT to get this info though
    FATtable[0].nextBlock = 0;
    // Space for FAT
    int i = 1;
    for (i; i < blocksNeeded + 1; i++){
        FATtable[i].nextBlock = i + 1;
    }
    FATtable[i - 1].nextBlock = 0;
    
    // Space for bitmap
    int intsNeededBMP = (numberOfBlocks + (sizeof(int) - 1)*8) / (sizeof(int)*8);
    int blocksNeededBMP = ((intsNeededBMP * sizeof(int)) + blockSize - 1) / blockSize;
    for (i; i < blocksNeeded + blocksNeededBMP + 1; i++){
        FATtable[i].nextBlock = i + 1;
    }
    FATtable[i - 1].nextBlock = 0;

    // Rest of the space is unused
    for(int i = blocksNeeded + 1 + blocksNeeded; i < numberOfBlocks; i++){
        FATtable[i].nextBlock = 0;
    }

	LBAwrite(FATtable, blocksNeeded, 1);
	return blocksNeeded;
}
