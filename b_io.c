/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Kyle Petkovic, Matthew Hernandez, Nicholas Pagcanlungan, Neal Olorvida
* Student IDs: 923683186, 920214682, 922298361, 915905996
* GitHub Name: kylepet
* Group Name: Compiler Errors
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stdbool.h"		// for write mode indicator
#include "b_io.h"
#include "initDir.h"
#include "mfs.h"
#include "fsLow.h"
#include "freeSpace.h"		// access to abstract write functions
#include "initSpace.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int flags;
	fdDir * dir;
	bool inWriteMode;	//set to true when file is open for write
	int blockIndex;		//holds current position of the block
	int eof;			// end of file  
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		fcbArray[i].inWriteMode = false; //by default, not open for write
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}

// Helper function to truncate a file
int truncateFile(fdDir *dir, const char *filename) {
    if (!dir || !filename) {
        return -1;  // Invalid input
    }

    struct fs_stat fileStat;
    int result = fs_stat(filename, &fileStat);
    if (result != 0) {
        return -1;  // Failed to get file stat
    }

    // Remove all entries from the directory
    while (dir->MAXentries > 2) {
        struct fs_diriteminfo *item = fs_readdir(dir);
        if (!item) {
            break;
        }

        // Ignore "." and ".." entries
        if (strcmp(item->d_name, ".") != 0 && strcmp(item->d_name, "..") != 0) {
            char entryPath[MAX_PATHNAME_SIZE];
            printf(entryPath, sizeof(entryPath), "%s/%s", filename, item->d_name);
            if (item->fileType == FT_DIRECTORY) {
                result = fs_rmdir(entryPath);
            } else {
                result = fs_delete(entryPath);
            }

            if (result != 0) {
                return -1;  // Failed to remove entry
            }
        }
    }

    // Reset the directory structure
    dir->dirEntryPosition = 0;
    dir->MAXentries = 2;  // . and .. entries

    // Reset file stat
    fileStat.st_size = 0;
    fileStat.st_blocks = 0;
    fileStat.st_accesstime = time(NULL);
    fileStat.st_modtime = time(NULL);
    fileStat.st_createtime = time(NULL);

    result = fs_stat(filename, &fileStat);
    return (result == 0) ? 0 : -1;
}

int fileExists(char *filename, fdDir *dir) {
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (strcmp(dir->directory[i].name, filename) == 0) {
            return 1;
        }
    }
    return 0;
}

int dirExists(char *dirPath) {
    struct stat st;

    if(stat(dirPath, &st) == 0) {
        if(S_ISDIR(st.st_mode)) {
            return 1;
        }
    }
    return 0;
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char *filename, int flags) {	
	
	mode_t mode;

  // Identify the parent directory of the new file
  char *dirPath = strdup(filename);
  char *lastSlash = strrchr(dirPath, '/');
  if (lastSlash != NULL) {
      *lastSlash = '\0'; // Split the filename into directory path and file name
  } else {
      // If the filename does not include a parent directory path, use the current directory as the parent directory
      dirPath = strdup(".");
  }

  // Check if the directory exists
  if (!dirExists(dirPath)) {
      printf("Directory does not exist: %s\n", dirPath);
      return -1;
  }

  // Open the parent directory
  fdDir *dir = fs_opendir(dirPath);
  if (dir == NULL) {
      printf("Failed to open parent directory: %s\n", dirPath);
      return -1;
  }

  if (fileExists(basename(filename), dir)) {
    printf("A file with the same name already exists: %s\n", filename);
    return -1;
  }
  
  // Find an unused entry in the parent directory
  int index = findUnusedEntry(dir->directory);
  if (index == -1) {
      printf("Failed to find an unused entry in the parent directory\n");
      return -1;
  }
  
  // If the file does not exist and the O_CREAT flag is set, create the file
  if (checkCreate(flags)) {
      int blocksNeeded = (512 + vcbGlobal->blockSize - 1) / vcbGlobal->blockSize;
      int *freeBlocks = allocFreeBlocks(blocksNeeded);

      // Check if the blocks were allocated successfully   
      if (freeBlocks == NULL) {
        printf("Failed to allocate blocks for the file\n");
        return -1;
      }


      // Update the unused entry in the parent directory
      DE *newEntry = &(dir->directory[index]);
      strcpy(newEntry->name, basename(filename)); // Set the name of the new file
      newEntry->loc = freeBlocks[0]; // Saving starting block to read back later
      newEntry->size = 0; // Set the size of the new file
      newEntry->blocks = blocksNeeded; // Set the number of blocks of the new file
      newEntry->isDir = 0; // Set the type of the new file as file

      // Write the updated parent directory back to the disk
      int blocksNeededDir = ((sizeof(DE) * MAX_DIR_ENTRIES) + vcbGlobal->blockSize - 1) / vcbGlobal->blockSize;
      int written = overwriteData(dir->directory[0].loc, blocksNeededDir, dir->directory);
      if (blocksNeededDir != written) {
          printf("Failed to write the updated parent directory back to the disk\n");
          return -1;
      }
  }

  if (checkTrunc(flags)) {
    int result = truncateFile(dir, filename);
    if (result != 0) {
        printf("Failed to truncate the file: %s\n", filename);
        return -1;
    }
  }

  if (checkAppend(flags)) {
    b_io_fd fd;
    // check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS)) {
		return (-1); 					//invalid file descriptor
	}
    int result = b_seek(fd, 0, SEEK_END);
    if (result != 0) {
        printf("Failed to set the file position to the end of the file: %s\n", filename);
        return -1;
    }
  }

  // Free the memory allocated for the directory path
  free(dirPath);

  return index;
}


/*  Interface to seek function	
	Utilizes preset numbers provided by <unistd.h> .
	Determines file offset, depending on value of whence.
 	Returns the resulting offset value. */
int b_seek (b_io_fd fd, off_t offset, int whence){
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	int resultingOffset = 0;

	/* Each case compares whence to preset numbers provided by <unistd.h> */
	if (whence == SEEK_SET){			// the offset value itself
		resultingOffset = (int)offset; 

	} else if (whence == SEEK_CUR){		// current location + offset
		resultingOffset = fcbArray[fd].index + (int)offset; 

	} else if (whence == SEEK_END){ 	// size of file + offset
		resultingOffset = fcbArray[fd].buflen + (int)offset;

	} else {							// error case
		printf("b_seek failed. Invalid value for whence");
		return -1;
	}
		
	return resultingOffset; // Return resulting offset
}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
			printf("invalid fd");
			return (-1); 					//invalid file descriptor
		}

	fcbArray[fd].inWriteMode = true; 
	
	//remainder of bytes in block
	int remainingBytes = fcbArray[fd].dir->directory->size % fcbArray[fd].dir->directory->blocks;

	//validation checks
	//validate file
	if(fcbArray[fd].dir == NULL)
		return -1;
	//validate count
	if(count < 0 || count > fcbArray[fd].buflen)
		return -1;
	//if the count is greater than remaining bytes in block
	//
		if(fcbArray[fd].dir->directory->size - remainingBytes <= 0)
			return -1;

	//Flag checks
	if(checkROnly(fcbArray[fd].flags))
		return -1;
	if(checkTrunc(fcbArray[fd].flags))
	{
		truncateFile(fcbArray[fd].dir,fcbArray[fd].dir->directory->name);
	}
	if (checkAppend(fcbArray[fd].flags)) 
	{
		fcbArray[fd].index = fcbArray[fd].dir->MAXentries;		
    }

	//memcpy what is in the user buffer into the fcb buffer 						
	memcpy(fcbArray[fd].buf, buffer, count);
	fcbArray[fd].index += count;
	
	
	//LBAwrite fcb buffer
	//cases for when if it excedes the current block
	if(remainingBytes > 0 && count > remainingBytes)
	{	
		//writes the to remaining bytes in block
		LBAwrite(fcbArray[fd].buf,1, remainingBytes);
		memcpy(fcbArray[fd].buf, buffer + remainingBytes, count - remainingBytes);
		fcbArray[fd].index = count - remainingBytes;
		LBAwrite(fcbArray[fd].buf,fcbArray[fd].dir->directory->blocks - 1, fcbArray[fd].buflen + fcbArray[fd].dir->directory->loc);
		

	}
	else
		LBAwrite(fcbArray[fd].buf,fcbArray[fd].dir->directory->blocks, fcbArray[fd].buflen + fcbArray[fd].dir->directory->loc);
	
	//returning the amount of bytes being written
	return (count); //Change this
	}
	



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char *buffer, int count) {
    if (startup == 0) b_init();  // Initialize our system

    // Check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return -1;  // Invalid file descriptor
    }

    b_fcb *fcb = &fcbArray[fd];

    int bytesRead = 0;

    if (fcb == NULL) {
        return -1;
    }

    if (count < 0) {
        return -1;
    }

    if (buffer == NULL) {
        return -1;
    }

    if (fcb->dir == NULL || fcb->dir->directory == NULL) {
        return 0; // No more data to read
    }

    while (count > 0) {
        if (fcb->index == fcb->buflen) {
            // Read more data into the buffer
            if (fcb->blockIndex >= fcb->dir->directory->blocks || fcb->eof) {
                // No more data to read
                break;
            }

            int blockNumber = fcb->dir->directory->loc +fcb->blockIndex;
            LBAread(fcb->buf, 1, blockNumber);
            fcb->blockIndex++;

            fcb->index = 0;
            fcb->buflen = strlen(fcb->buf);

            if (fcb->buflen == 0) {
                // No more data to read
                break;
            }
        }

        int bytesToCopy = (count < fcb->buflen - fcb->index) ? count : (fcb->buflen - fcb->index);

        memcpy(buffer + bytesRead, fcb->buf + fcb->index, bytesToCopy);

        fcb->index += bytesToCopy;
        bytesRead += bytesToCopy;
        count -= bytesToCopy;

        if (fcb->index == fcb->buflen) {
            // Check if we reached the end of the buffer, set EOF if we read less than expected
            if (bytesToCopy < count) {
                fcb->eof = 1;
                break;
            }
        }
    }

    return bytesRead;
}
	
/*  Interface to Close the file 
	write any dirty buffers to disk
	update DE
	free buffer, free FCB */	
int b_close (b_io_fd fd){

	if (fd < 0 || fd >= MAXFCBS){ // Out of bounds fd error case
		printf("b_close Error: Out of bounds file descriptor value\n");
		return -1;
	}

    printf("Closing file %d\n", fd);

	b_fcb * fcb = &fcbArray[fd];
	time_t currentTime = time(NULL);

	/* Check if buffer is modified and not written to disk */
	if (fcb->inWriteMode == true && fcb->index > 0){ // dirty buffer case
		int indexOfFreeBlock = b_getFCB();

		if (indexOfFreeBlock == -1){ // insufficient free space case
			printf("b_close Error: Not enough free space\n");
			return -1;

		/* Write dirty buffer to disk */	
		} else {
			overwriteData(fcb->dir->directory->loc, fcb->dir->directory->blocks, fcb->buf);

			/* Update DE */
			fcb->dir->directory->accessed = currentTime;
			fcb->dir->directory->modified = currentTime;
			fcb->dir->directory->size = fcb->buflen; // TODO: consider changing this value
		}
	}

	/* Free buffer and reset FCB */
	free(fcbArray[fd].buf);
	fcbArray[fd].buf = NULL;
    fcbArray[fd].index = 0;
    fcbArray[fd].buflen = 0;
    fcbArray[fd].flags = 0;
    fcbArray[fd].dir = NULL;
    fcbArray[fd].inWriteMode = false;
    fcbArray[fd].blockIndex = 0;
    fcbArray[fd].eof = 0;
    close(fd);
    fcb = NULL;

	return 0;
}

int checkCreate(int flagBits) {
    return (flagBits & O_CREAT) != 0;
}

int checkTrunc(int flagBits) {
    return (flagBits & O_TRUNC) != 0;
}

int checkAppend(int flagBits) {
    return (flagBits & O_APPEND) != 0;
}

int checkROnly(int flagBits) {
    return (flagBits & O_RDONLY) != 0;
}

int checkWOnly(int flagBits) {
    return (flagBits & O_WRONLY) != 0;
}

int checkRW(int flagBits) {
    return (flagBits & O_RDWR) != 0;
}