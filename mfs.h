/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: Basic File System
*
* File: mfs.h
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "initDir.h"
#include "b_io.h"
#include <dirent.h>
#include <stdbool.h>

#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK
#define MAX_FILEPATH_SIZE 225
#define MAX_PATHNAME_SIZE 256

extern char currentDirectoryPath[MAX_FILEPATH_SIZE];

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

typedef struct ppInfo{
	DE *parent;
	char *lastElement;
	int index;
} ppInfo;

// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
typedef struct fs_diriteminfo{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
} fs_diriteminfo;

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct fdDir
	{
	/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
	unsigned short  d_reclen;		/* length of this record */
	unsigned short	dirEntryPosition;	/* which directory entry position, like file pos */
	DE *	directory;			/* Pointer to the loaded directory you want to iterate */
	struct fs_diriteminfo * di;		/* Pointer to the structure you return from read */
	short MAXentries;
	} fdDir;


// Key directory functions
int parsePath(const char * path, ppInfo *ppi);
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
int isDirectory(DE file);
DE* loadDir(DE *de);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file
bool cleanPath(const char *inputPath, char *outputPath);

// Func to create a file
int fs_create(char *filename, mode_t permissions);

// Func to find unused entry in directory
int findUnusedEntry(DE *directory);

// This is the strucutre that is filled in from a call to fs_stat
struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	
	/* add additional attributes here for your file system */
	unsigned char fileType;		/* FT_REGFILE, FT_REGFILE, etc. */
	};

int fs_stat(const char *path, struct fs_stat *buf);

#endif
