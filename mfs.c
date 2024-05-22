/**************************************************************
* Class:  CSC-415-03 Fall 2023
* Names: Kyle Petkovic, Matthew Hernandez, Nicholas Pagcanlungan, Neal Olorvida
* Student IDs: 923683186, 920214682, 922298361, 915905996
* GitHub Name: kylepet
* Group Name: Compiler Errors
* Project: Basic File System
*
* File: mfs.c
*
* Description: File System inode initialization, tracking, and updating.
*
**************************************************************/

#include "mfs.h"
#include "initDir.h"
#include "initSpace.h"
#include "freeSpace.h"
#include <stdbool.h>
#include <time.h>
#include <string.h>

int isUsed(DE directory)
{
	//returns 1 if it is Used else 0
	if(directory.name[0] == '\0'){
		// unsued
		return 0;
	}
	else {
		// used
		return 1;
	}
}


int getType(DE* entry)
{
	return entry->isDir;
}

int findUnusedEntry(DE* entry)
{
	for(int i = 0; i < MAX_DIR_ENTRIES -1; i++)
	{
		if(isUsed(entry[i]) == 0)
			return i;
	}
		
	return -1;
}

int fs_isFile(char * filename) // 0 is file 1 is not
{
	ppInfo *pp;
	pp = (ppInfo *) malloc(sizeof(ppInfo));

	char outputPath[MAX_FILEPATH_SIZE];
	cleanPath(filename, outputPath);

	int ret = parsePath(outputPath, pp);

	if(ret == -1)
		return -1;
	else
		return getType(&pp->parent[pp->index]) == 0;
}

int fs_isDir(char * pathname)// 1 is dir 0 is not
{
	ppInfo *pp;
	pp = (ppInfo *) malloc(sizeof(ppInfo));

	char outputPath[MAX_FILEPATH_SIZE];
	cleanPath(pathname, outputPath);

	int ret = parsePath(outputPath, pp) == -1;

	if(ret == -1)
		return -1;
	if(pp == NULL){
		return -1;
	}
	else{
		return getType(&pp->parent[pp->index]);
	}
		
}

int fs_mkdir(const char *pathname, mode_t mode)
{


	ppInfo *pp = (ppInfo*) malloc(sizeof(ppInfo));
	DE * newDir = (DE*) malloc(sizeof(DE));

	char outputPath[MAX_FILEPATH_SIZE];

	cleanPath(pathname, outputPath);

	int ret = parsePath(outputPath, pp);

	// Folders missing before last element
	if(ret == -4){
		printf("Previous folders don't exist\n");
		return -1;
	}

	if(ret != -1) 
	{
		printf("\nFolder already exists\n");
		return -1;
	}
	
	

	
	//init the dir and finds the next unused entry
	int newDirLoc = initDir(MAX_DIR_ENTRIES, pp->parent, vcbGlobal->blockSize);
	
	int index = findUnusedEntry(pp-> parent);

	int blocksNeeded = ((sizeof(DE) * MAX_DIR_ENTRIES) + vcbGlobal->blockSize - 1) / vcbGlobal->blockSize;
	//sets name of new dir
	strcpy(newDir->name, pp->lastElement);
	newDir->loc = newDirLoc;
	newDir->blocks = blocksNeeded;

	// Time in main folder might be slightly off subfolders.
	time_t currentTime = time(NULL);
	newDir->accessed = currentTime;
	newDir->modified = currentTime;
	newDir->create = currentTime;
	newDir->isDir = 1;
 	
	pp->parent[index] = *newDir;
	
	int ovrDataRet = overwriteData(pp->parent[0].loc, blocksNeeded, pp->parent);

	return 0;
}

fdDir * fs_opendir(const char *pathname){
	ppInfo *ppi = (ppInfo*) malloc(sizeof(ppInfo));
	
	parsePath(pathname, ppi);

	

	if (strcmp(currentDirectoryPath, "/") == 0){
		fdDir *fdd = (fdDir*) malloc(sizeof(fdDir));
		fdd->directory = g_rootDir;
        fdd->di = (fs_diriteminfo*) malloc(sizeof(fs_diriteminfo));
		fdd->dirEntryPosition = 0;
		fdd->MAXentries = MAX_DIR_ENTRIES;
		return fdd;
	}

	if(isDirectory(ppi->parent[ppi->index])){
		fdDir *fdd = (fdDir*) malloc(sizeof(fdDir));
		fdd->directory = loadDir(&(ppi->parent[ppi->index]));
        fdd->di = (fs_diriteminfo*) malloc(sizeof(fs_diriteminfo));
		fdd->dirEntryPosition = 0;
		fdd->MAXentries = MAX_DIR_ENTRIES;
		return fdd;
	}

	if(ppi->index == -1) {
		return NULL;
	}



	return NULL;
}


fs_diriteminfo* fs_readdir(fdDir *fdd){
	for(int i = fdd->dirEntryPosition; i < fdd->MAXentries; i++)
	{
		// If it's unused we just go to the next i
		if(isUsed(fdd->directory[i]))
		{
			strcpy(fdd->di->d_name, fdd->directory[i].name);
            fdd->di->fileType = getType(&fdd->directory[i]);
			fdd->dirEntryPosition = i + 1;
			return fdd->di;
		}
	}
	// Reached the end with no more directory entries
	return NULL;
}




/* 
 * Closes directory stream associated with dirp
 * Also closes underlying file descriptor associated with dirp
 * Returns 0 on success. 
 * Returns error value on failure(?)
 */
int fs_closedir(fdDir *dirp){
	/* TODO:
	Might need to consider implementing free() for other 
	variables that underwent malloc(). However, only a 
	fdDir pointer has been passed to this function. */

	/* Checks for... uh, some freak accident */
	if(dirp == NULL){
		return -1;
	}
	

		/* Free pointers that underwent malloc() */
		free(dirp);	
		
		/* Overwrite-proofing */
		dirp = NULL;

		return 0;	 // success case
	
}

int findEntryInDirectory(DE *parent, char *name){
	if(parent == NULL) return -1;
	if(name == NULL) return -1;
	int maxEntries = parent->size / sizeof(DE); // we're interested in parent[0], this will give us that!
	for(int i = 0; i < maxEntries; i++){
		int x = strcmp(parent[i].name, name);
		if(x == 0){ // They match
			return i;
		}
	}
	// No matches :(
	return -1;
}

int isDirectory(DE file){
	return file.isDir;
}

DE* loadDir(DE *de){
	DE *child = (DE*) malloc(de->blocks * vcbGlobal->blockSize);
	int res =  readData(de->loc, child);

	if(res != de->blocks){
		printf("(loadDir) expected %d blocks but got %d", de->blocks, res);
	}

	return child;
}

char* fs_getcwd(char * pathname, size_t size) {

    if (strlen(currentDirectoryPath) > size) {
        printf("Buffer size is not enough");
        return NULL;
    }

    strcpy(pathname, currentDirectoryPath);
    return pathname;
}

// changed path to be a const char for fs_stat compatibility
int parsePath(const char * path, ppInfo *ppi) {
    if (path == NULL || ppi == NULL) {
        return -1;
    }

    DE * startDir;

	 //Determine where to start parsing 
    if (path[0] == '/') {	// begin at the root '/'
        startDir = g_rootDir;
    } 
	else {				// begin at the current working directory
        startDir = g_cwd;
    }
	

    DE * parent = startDir;

    char * savePtr;
    char * mutPath = strdup(path);
    if (mutPath == NULL) {
		printf("mutPath is NULL");
        return -1;
    }

	/* Tokenize string */
    char * token1 = strtok_r(mutPath, "/", &savePtr);

    if (token1 == NULL) {	
        if (strcmp(path, "/") == 0) {	// NULL, still valid case
			ppi -> parent = parent;
            ppi -> index = -1;
            ppi -> lastElement = NULL;
            free(mutPath);
            return 0;
        }
		printf("token1 is NULL");
        free(mutPath);
        return -1;
    }

    while(token1 != NULL) {
        int index = findEntryInDirectory(parent, token1);
		/* Check if next token is NULL */
		char * token2 = strtok_r(NULL, "/", &savePtr);

		if(index == -1)
		{
			ppi -> parent = parent;
			ppi -> index = -1;
			ppi->lastElement = token1;

			// Not end of string but couldn't find folder
			if(token2 != NULL){
				return -4;
			}
			
			return -1;
		}
			

		
        if (token2 == NULL) {	// Done if NULL
            ppi -> parent = parent;
            ppi -> index = index;
            ppi -> lastElement = strdup(token1);
            return 0;
        }

        if(index == -1) {
            return -2;
        }

        if (!isDirectory(parent[index])) {
            return -2;
        }

        DE *temp = loadDir(&(parent[index]));

        if (temp == NULL) {
            return -3;
        }

        parent = temp;
        token1 = token2;
    }

    free(mutPath);
    return 0;
}

/* More bugfixes */
bool cleanPath(const char *inputPath, char *outputPath) {
   /* Error handling */
    if (inputPath == NULL || outputPath == NULL){
        return false;
    }

    if (strcmp(inputPath, "") == 0){    // empty input case
        strcpy(outputPath, currentDirectoryPath);
        return true;
    }

    /* Check: absolute or relative */
    char tempPath[MAX_FILEPATH_SIZE];
    bool isAbsolute = inputPath[0] == '/';
    strcpy(outputPath, "/");

    if (isAbsolute) {    // absolute path case
        strcpy(tempPath, inputPath);

    } else {             // relative path case
        /*  Combine the current working directory path
            with the relative path to make an absolute path */
        strcpy(tempPath, currentDirectoryPath);


        /* If the path doesn't have a tailing '/', append with it */
        if(strrchr(tempPath, '/') != NULL){
            strcat(tempPath, "/");
        }

        strcat(tempPath, inputPath); // This should be an absolute path now
    }

    char *savePtr;
    char *token = strtok_r(tempPath, "/", &savePtr);

    while (token != NULL) {
        /* Skip current working directory entry '.' */
        if (strcmp(token, ".") == 0) {
            // Keeping this empty fixes stuff lmao

            /* Handle parent directory '..' */
        } else if (strcmp(token, "..") == 0) {
            char *lastSlash = strrchr(outputPath, '/');

            /* Remove last component of the output path */
            if (lastSlash != NULL){
                *lastSlash = '\0';

                /* Keep root if navigating beyond */
            } else {
                strcat(outputPath, "/");
            }
        } else {
            /* Process normal directory names */
            if (strlen(outputPath) > 1 && outputPath[strlen(outputPath) - 1] != '/') {
                strcat(outputPath, "/");
            }
            /* Concatenate directory name to output path */
            strcat(outputPath, token);
        }

        token = strtok_r(NULL, "/", &savePtr);
    }

    /* Remove trailing '/' if present in output path */
    if (strlen(outputPath) > 1 && outputPath[strlen(outputPath) - 1] == '/') {
        outputPath[strlen(outputPath) - 1] = '\0';
    }

    /* If resulting path is empty, set it to '/' */
    if (strlen(outputPath) == 0 && strlen(inputPath) != 0){
        strcpy(outputPath, "/");
    }

    /* If outputPath somehow lacks a beginning '/' */
    if (outputPath[0] != '/') {
        strcpy(outputPath + 1, outputPath);
        outputPath[0] = '/';
    }

    return true;
}

int fs_setcwd(char * pathname) {
    /* Clean the pathname. */
    char cleanedPathname[MAX_PATHNAME_SIZE];
    if (!cleanPath(pathname, cleanedPathname)) {
        printf("Failed to clean path");
        return -1;
    }


    ppInfo *ppi = (ppInfo*) malloc(sizeof(ppInfo));
    int result = parsePath(cleanedPathname, ppi);


	if(strcmp("/", cleanedPathname) != 0 && isDirectory(ppi->parent[ppi->index]) == 0){
		printf("%s is not a folder\n", pathname);
		return -1;
	}

    if (result != 0) {
        printf("Failed to parse path returns: %i",result);
        return -1;
    }

    strncpy(currentDirectoryPath, cleanedPathname, sizeof(currentDirectoryPath));

	if(strcmp(currentDirectoryPath, "/") == 0){
		g_cwd = g_rootDir;
	}
	if(ppi->lastElement != NULL){
		int index = findEntryInDirectory(ppi->parent, ppi->lastElement);


		// error occurs here
        if(index != -1) {
            g_cwd = loadDir(&ppi->parent[index]);
			return 0;
        }
        return -1;
	}
    return 0;

}

int fs_stat(const char *path, struct fs_stat *buf) {

    ppInfo *ppi;

	char cleanedPathname[MAX_PATHNAME_SIZE];
    if (!cleanPath(path, cleanedPathname)) {
        printf("Failed to clean path");
        return -1;
    }

	ppi = (ppInfo*) malloc(sizeof(ppi));

    int result = parsePath(cleanedPathname, ppi);

    if (result != 0 && fs_isFile(cleanedPathname) == 0) {
        printf("Failed to parse the path.\n");
        return -1;
    }

    DE *de = &(ppi->parent[ppi->index]);

    buf->st_size = de->size;
    buf->st_blksize = vcbGlobal->blockSize;
    buf->st_blocks = de->blocks;
    buf->st_accesstime = de->accessed;
    buf->st_modtime = de->modified;
    buf->st_createtime = de->create;

	buf->fileType = (de->isDir) ? FT_DIRECTORY : FT_REGFILE;

    return 0;

}

int fs_rmdir(const char *pathname){
	ppInfo *ppi = (ppInfo*) malloc(sizeof(ppInfo));

	

	char outputPath[MAX_FILEPATH_SIZE];
	cleanPath(pathname, outputPath);

	if(parsePath(outputPath, ppi) != 0){
		printf("\n(fs_rmDir) folder doesn't exist\n");
	}

	DE toRemove = ppi->parent[ppi->index];

	// You are in the folder that's to be removed!
	if(strcmp(outputPath, currentDirectoryPath) == 0){
		printf("\ncd out of the folder you want to delete.\n");
		return -1;
	}

	// TODO: Think this error check might need to be changed
	if(ppi->index == -1) return -1;
	if(isDirectory(toRemove)){
		// Load Dir's data
		DE *contents = loadDir(&toRemove);
		DE *parentContents = loadDir(ppi->parent);
		// Function will fail if the directory is not empty
		// i is 2 so it skips the . and .. directories
		for(int i = 2; i < MAX_DIR_ENTRIES; i++){
			if(isDirectory(contents[i]) == 0 || isDirectory(contents[i]) == 1){
				printf("\nFolder contains files or other folders.\n");
				return -1;
			}
		}
		// Location of the folder's blob of data
		int rm = toRemove.loc;
		int correctSize = toRemove.blocks;
		// Need to remove this entry from it's parent and it's data
		parentContents[ppi->index].size = 0;
		parentContents[ppi->index].loc = -1;
		parentContents[ppi->index].isDir = -1;
		parentContents[ppi->index].accessed = 0;
		parentContents[ppi->index].modified = 0;
		parentContents[ppi->index].create = 0;
		strcpy(parentContents[ppi->index].name, "\0");
		int blocksNeeded = (MAX_DIR_ENTRIES * sizeof(DE) + vcbGlobal->blockSize - 1)/ vcbGlobal->blockSize;
		int written = overwriteData(parentContents[0].loc, blocksNeeded, parentContents);
		if (blocksNeeded != written){
			printf("Blocks written doesn't match up with size of data to write");
			free(ppi);
			return -1;
		}

		// unallocate data in folder's data block
		int res = unallocateFolder(rm);

		if(res != correctSize){
			printf("Removed %d but should have removed %d", res, correctSize);
		}
		
		if(g_cwd[0].loc == parentContents[0].loc){
			g_cwd = parentContents;
		}

		if(g_cwd[0].loc == parentContents[0].loc && 
			strcmp(currentDirectoryPath, "/") == 0){
			g_cwd = parentContents;
			g_rootDir = parentContents;
		}

		free(ppi);
		return 0;
	}
	else{
		// Not a folder
		free(ppi);
		return -2;
	}
}

int fs_delete(char* filename){
	ppInfo *ppi = (ppInfo*) malloc(sizeof(ppInfo));


	char outputPath[MAX_FILEPATH_SIZE];

	cleanPath(filename, outputPath);
	
	int respp = parsePath(outputPath, ppi);

	if (respp != 0) {
		printf("\nFolder or file doesn't exist\n");
		return -1;
	}

	if(isDirectory(ppi->parent[ppi->index])) return -1;
	
	int index = findEntryInDirectory(ppi->parent, filename);
	if(index == -1){
		printf("File not found");
		return -1;
	}

	DE *parentContents = loadDir(ppi->parent);

	if(g_cwd->loc == parentContents[0].loc){
		g_cwd = parentContents;
	}

	unallocateFolder(parentContents[index].loc);

	parentContents[index].size = 0;
	parentContents[index].loc = -1;
	parentContents[index].isDir = -1;
	strcpy(parentContents[index].name, "\0");

	int blocksNeeded = (MAX_DIR_ENTRIES * sizeof(DE) + vcbGlobal->blockSize - 1)/ vcbGlobal->blockSize;
	
	int ret = overwriteData(parentContents[0].loc, blocksNeeded, parentContents);

	if(ret != blocksNeeded){
		printf("Expected to write %d blocks, but wrote %d", blocksNeeded, ret);
		return -1;
	}

	

	

	if(g_cwd->loc == g_rootDir->loc){
		g_rootDir = parentContents;
	}

	free(ppi);

	return 0;
}
