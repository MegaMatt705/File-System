# File System

A Linux based file system made using FAT.

Interfaces:

```
b_io_fd b_open (char * filename, int flags);
int b_read (b_io_fd fd, char * buffer, int count);
int b_write (b_io_fd fd, char * buffer, int count);
int b_seek (b_io_fd fd, off_t offset, int whence);
int b_close (b_io_fd fd);

```

Directory Functions - see [https://www.thegeekstuff.com/2012/06/c-directory/](https://www.thegeekstuff.com/2012/06/c-directory/) for reference.

```
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
fdDir * fs_opendir(const char *name);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

char * fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *buf);   //linux chdir
int fs_isFile(char * path);	//return 1 if file, 0 otherwise
int fs_isDir(char * path);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file

// This is NOT the directory entry, it is JUST for readdir.
struct fs_diriteminfo
    {
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
    };
```
Finally file stats:

```
int fs_stat(const char *path, struct fs_stat *buf);

struct fs_stat
    {
    off_t     st_size;    		/* total size, in bytes */
    blksize_t st_blksize; 		/* blocksize for file system I/O */
    blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
    time_t    st_accesstime;   	/* time of last access */
    time_t    st_modtime;   	/* time of last modification */
    time_t    st_createtime;   	/* time of last status change */
	
    * add additional attributes here for your file system */
    };

```

A shell program designed to demonstrate the file system called fsshell.c is provided.  It has a number of built in functions that will work if you implement the above interfaces, these are:
```
ls - Lists the file in a directory
cp - Copies a file - source [dest]
mv - Moves a file - source dest
md - Make a new directory
rm - Removes a file or directory
touch - creates a file
cat - (limited functionality) displays the contents of a file
cp2l - Copies a file from the test file system to the linux file system
cp2fs - Copies a file from the Linux file system to the test file system
cd - Changes directory
pwd - Prints the working directory
history - Prints out the history
help - Prints out help
```



