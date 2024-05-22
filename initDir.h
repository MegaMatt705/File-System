/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Kyle Petkovic, Matthew Hernandez, Nicholas Pagcanlungan, Neal Olorvida
* Student IDs: 923683186, 920214682, 922298361, 915905996
* GitHub Name: kylepet
* Group Name: Compiler Errors
* Project: Basic File System
*
* File: fsInit.h
*
* Description: Interface for initializing the directory
*
* 
*
**************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mfs.h"

#define FileNameLimit 40
#define MAX_DIR_ENTRIES 50


#ifndef STRUCT_DE
#define STRUCT_DE
typedef struct DE{

    char name[FileNameLimit];
    int blocks;
    // Exact size in bytes
    int size;
    // First block this directory entry is located at
    int loc;
    // 0 is file, 1 is dir
    short isDir;
    time_t accessed;
    time_t modified;
    time_t create;

} DE;
#endif 

int initDir(int defaultEntries, DE * parent, int blockSize);
extern DE *g_rootDir;
extern DE *g_cwd;