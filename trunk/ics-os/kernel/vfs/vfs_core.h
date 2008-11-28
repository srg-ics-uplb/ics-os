/*
  Name: vfs_core.h
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 30/01/04 05:25
  Description: This module defines the core functions of the DEX VFS (Virtual File
  System). The Virtual File System is designed to allow support of multiple 
  filesytems and serves as a universal filesystem layer in which the operating system
  will work on.
               
    DEX educational extensible operating system 1.0 Beta
    Copyright (C) 2004  Joseph Emmanuel DL Dayo

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

#ifndef VFS_CORE_H
#define VFS_CORE_H

#include "../dextypes.h"
#include "../stdlib/time.h"
#include "../process/sync.h"

//defines constants for the attribute bits - (influenced by UNIX)
#define FILE_DIRECTORY  0x200
#define FILE_IEXE       0x100
#define FILE_IREAD      0010000000b
#define FILE_IWRITE     0001000000b
#define FILE_GEXE       0000100000b
#define FILE_GREAD      0000010000b
#define FILE_GWRITE     0000001000b
#define FILE_OEXE       0x4
#define FILE_OREAD      0x2
#define FILE_OWRITE     0x1
#define FILE_MOUNT      0x800

// file/device type constants
#define DEV_MEM 1
#define DEV_OUTPUT_STREAM 2
#define DEV_INPUT_STREAM 3
#define DEV_BLOCK 4

// constatns used by fseek
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

// Constants used by fopen() and openfileEX
#define FILE_READ 0
#define FILE_WRITE 1
#define FILE_READWRITE 2
#define FILE_APPEND 3

// Constants used by openfilex
#define FILE_READ 0
#define FILE_WRITE 1
#define FILE_READWRITE 2
#define FILE_APPEND 3

//constants used for buffering, based on GNU C constants
#define FILE_IOFBF  0
#define FILE_IOLBF  1
#define FILE_IONBF  2
#define FILE_BUFSIZE 1024

//Tells the VFS that this part of the VFS node has not yet been mounted
//completely and that it should call the mount function of the filesystem driver
#define VFS_NOT_MOUNTED 0xFFFFFFF0

//used by chattb to tell the filesystem driver what
//aspects of the file attributes to change

//file attributes
#define FILE_FSIZE 1



typedef struct _file {
 char name[255];
 DWORD attb;      //the file attributes
 DWORD fsid,memid; //the deviceid for the filesystem driver and the data device respectively
 struct _file *path; //points back to the parent directory
 DWORD size;

 dex32_datetime date_created;  //A date-time stamp which indicates creation date
 dex32_datetime date_modified; //A date-time stamp which indicates modification date
 DWORD locked;                 //prevents the file from being deleted, written to etc.
 DWORD start_sector;           //marks a file's starting sector
 
 struct _file *prev,*next;  //if a file, points to the previous and next file
                      //in the current directory
 struct _file *files; //for directories only

 /***************** Filesystem dependent structures. the DEX VFS does not
  really care about this, but it automatically frees the data structures during unmount
  in misc and misc2 if miscsize or miscsize2 is nonzero */
  
 int opened;  //maintains the open count.. 0 if the file is closed

 void *misc;    //a structure that may be used by the device
                //for caching purposes, the directory entry for example
                //usage may differ depending on the file system

 void *misc2;   //a structure that may be used by the device
                //for caching purposes, the directory entry for example
                //usage may differ depending on the file system
                
 int misc_flag; //may be used by the filesystem for its own purposes, the VFS does not
                //even touch this element
                
 DWORD miscsize, miscsize2; //the size of the structure pointed to by misc

} vfs_node;


typedef struct _file_device {
	char name[255];
	int (*mount_fxn)(vfs_node *mountpoint);
	struct _file_device *next;
} file_device;

//the file_PCB structure monitors the number of opened files
//and the owners of those files
typedef struct _file_PCB {
    int size;        //The size of this structure
	DWORD processid; //the processid of the owning process
	int fileid;      //A unique identifier for this file handle
	DWORD type;
	vfs_node *ptr;       //points to the vfs_node structure in the VFS
	DWORD ptrlow,ptrhigh;
	int mode,locked;  //stores the mode the file is opened ex. FILE_READ, FILE_WRITE e.g.
	
	//----------------------------------------------
	//VFS BUFFER Management data ->Without this, individual fgetc's and fputc's as well as are
	//all other file operations would be extremely slow.
	int buffertype; //Determines what kind of buffer to use... U*nix made me do it...
	char *buffer;      //pointer to buffer data
	int userbuffer;  //determines if buf was allocated by the user or the system
	int bufferwrite; //a flag inidicating if the buffer was written to
	DWORD startptr;  //position of buffer in relation to the whole file
    DWORD endsize;   //position of pointer in the buffer
    DWORD bufptr;   //position of current pointer in the buffer
    DWORD bufsize;  //size of buffer allocated in memory
	DWORD *sectinfo;//for a block device, this stores the list of blocks where the
                 //device is stored
	struct _file_PCB *next,*prev;
} file_PCB;


//derived from stat.h, modified for use with DEX -- returned by fstat to hold info about the file
typedef struct _vfs_stat
{
    int     size;       /*The size of this structure*/
	int	    st_dev;		/* Equivalent to drive number 0=A 1=B ... */
	int	    st_ino;		/* Always zero ? */
	int	    st_mode;	/* See above constants */
	short	st_nlink;	/* Number of links. */
	short	st_uid;		/* User: Maybe significant on NT ? */
	short	st_gid;		/* Group: Ditto */
	int	    st_rdev;	/* Seems useless (not even filled in) */
	int	    st_size;	/* File size in bytes */
	int	    st_atime;	/* Accessed date (always 00:00 hrs local
				 * on FAT) */
	int	    st_mtime;	/* Modified time */
	int	    st_ctime;	/* Creation time */
} vfs_stat;

//This structure is used for Aliasing paths at the VFS level
typedef struct _path_cut
{
    char *name;
    char *path;
    struct _path_cut *prev,*next;
} path_cut;

extern path_cut *vfs_path_cut_head;
extern file_PCB *file_globalopen;
extern vfs_node *vfs_root,*homedir; //lists the pointer to the root and the current working
                       //directory for the current user


extern sync_sharedvar vfs_busy; //used for the busy waiting loops inside the vfs
extern int vfs_nextfileid;

/*---------------------------------Function Prototypes Here----------------------------------*/
int     changedirectory(const char *name);
int     chdir(const char *name);
int     closeallfiles(DWORD pid);
vfs_node *createfile(const char *name,DWORD attb);
void    create_filesystem();
int     fclose(file_PCB *fhandle);
int     fdelete(file_PCB *fhandle);
int     feof(file_PCB* fhandle);
int     fflush(file_PCB* fhandle);
int     fgetsectors(file_PCB* fhandle);
char    *fgets(char *s,int n,file_PCB *fhandle);
int     file_ok(file_PCB* fhandle);
void    file_showopenfiles();
void    findfile(char *name);
int     fread(char *buf,int itemsize,int noitems,file_PCB* fhandle);
int     fseek(file_PCB *fhandle, long offset, int whence);
int     fstat(file_PCB *fhandle,vfs_stat *statbuf);
long int ftell(file_PCB *fhandle);
int     fwrite(char *buf, int itemsize, int n, file_PCB* fhandle);
vfs_node *getdirectory(const char *name);
char    *getpath(vfs_node *ptr,char *s);
int     mkdir(const char *name);
file_PCB *openfilex(char *filename,int mode);
void    parsedir(char *fullpath,char *loc,char *name);
int     rename (const char *oldname, const char *newname);
char    *rev_str(char *str);
char    *showpath(char *s);
void    swapchar(char *t1,char *t2);
vfs_node *vfs_checkopenfiles(vfs_node *ptr);
void    vfs_createnode(vfs_node *node, vfs_node *parent);
int     vfs_deletefile(vfs_node *ptr);
int     vfs_freedirectory(vfs_node *ptr);
int     vfs_mountdirectory(vfs_node *node);
int     vfs_mount_device(const char *fsname,const char *devname,const char *location);
char    *vfs_readline(char *s,int n,const char term, file_PCB *fhandle);
vfs_node *vfs_searchname(const char *name);
vfs_node *searchname(const char *name,int);
int vfs_unmount(vfs_node *node);
int     vfs_unmount_device(const char *location);
int vfs_directread(char *buf,int itemsize,int noitems,file_PCB* fhandle);
int vfs_directwrite(char *buf, int itemsize, int n, file_PCB* fhandle);
int vfs_readchar(file_PCB *handle, char *character);
int vfs_writechar(file_PCB *handle, char character);
int vfs_flushbuffer(file_PCB *handle);
int vfs_setbuffer(file_PCB *handle,char *buffer,int bufsize,int mode);
int vfs_getstat(const char *filename,vfs_stat *statbuf);

#endif
