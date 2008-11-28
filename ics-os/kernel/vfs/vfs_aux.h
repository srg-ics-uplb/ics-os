/*
  Name: VFS auxillary module
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 27/02/04 05:30
  Description: This module contains functions that are not really essential to
  the operation of the VFS. Some functions include path aliasing "pcuts" and some
  browsing functions
*/

/*****Function prototypes Here - See vfs_aux.c for function description**************/
int  vfs_addpathcut(const char *name, const char *path);
char *vfs_attbstr(vfs_node *node,char *buf);
int  vfs_removepathcut(const char *name);
int  vfs_rmdir(vfs_node *node);
void vfs_showpathcuts();
const char *vfs_getpathcut(const char *name);
int  vfs_ispathcut(const char *name);
int  vfs_listdir(vfs_node *current_dir,vfs_node *buffer,int size);

