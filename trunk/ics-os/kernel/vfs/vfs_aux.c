/*
  Name: VFS auxillary module
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 27/02/04 05:30
  Description: This module contains functions that are not really essential to
  the operation of the VFS. Some functions include path aliasing "pcuts" and some
  browsing functions
  
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

/* ==================================================================
   vfs_addpathcut(alias,path):
   
   *Adds pathcut, a patchcut is an alias that is used to denote a particular path.
    for example c: = "/hdd/" this means that a path string which contains
    "c:/mydir1" will automatically be expanded to /hdd/mydir1.
    
   *pathcut names must end with a ':' and has a maximum size of 20.
   
   */
   
int vfs_addpathcut(const char *name, const char *path)
{

    int name_length = strlen(name);
    int path_length;
    char path_temp[512];  
    vfs_node *node;
    
    
    //pathcut name must not be too long (The purpose of pathcuts is to shorten names!)
    if (name_length > 20) return -1;
    
    //pathcut name must end with a ":"
    if (name[name_length-1]!=':') return -1;
    
    
    //verify if the path really exists
    
    if (path!=0)
        node = vfs_searchname(path);
                else
       {         
         showpath(path_temp);   
         node = vfs_searchname(path_temp);
       };
        
    if (node==-1) return -1;
    
    if (node->attb & FILE_DIRECTORY) 
        {
              //use the add to head method
              path_cut *pc = malloc(sizeof(path_cut));
              
              pc->name = malloc(name_length+1);
              strcpy(pc->name,name);              
              
              //automatically use the current path as the path?
              if (path == 0)
                     {
                      pc->path = malloc(strlen(path_temp)+1);              
                      strcpy(pc->path,path_temp);
                     }
              else
                     {
                      pc->path = malloc( strlen(path)+1 );
                      strcpy(pc->path,path);
                     }; 
              
              pc->next=vfs_path_cut_head;  
              
              if (pc->next!=0) pc->next->prev = pc;
              
              vfs_path_cut_head = pc;
              
              printf("pathcut created %s = \"%s\"\n", pc->name,pc->path);
              return 1;
        };
        return -1;
};

/* ==================================================================
   vfs_removepathcuts():
   *removes a pathcut with "name" from the list of pathcuts
   */

int vfs_removepathcut(const char *name)
{
    path_cut *ptr = vfs_path_cut_head;
    while (ptr!=0)
    {
        if (strcmp(name, ptr->name)==0)
           {
                //remove the pathcut from the list
                if (vfs_path_cut_head == ptr)
                    vfs_path_cut_head = ptr->next;
                if (ptr->prev!=0) ptr->prev->next = ptr->next;         
                if (ptr->next!=0) ptr->next->prev = ptr->prev;
                
                free(ptr->name);
                free(ptr->path);
                free(ptr);
                return 1;
           };
           ptr=ptr->next;
    };
    return -1;
};

/* ==================================================================
   vfs_showpathcuts():
   *Simply lists the pathcuts defined
   */
   
void vfs_showpathcuts()
{
    path_cut *ptr = vfs_path_cut_head;
    int total = 0;
    printf("Pathcut list:\n");
    while (ptr!=0)
    {
           printf("%s = \"%s\"\n", ptr->name,ptr->path);
           ptr=ptr->next;
           total++;
    };
    printf("A total of %d pathcuts.\n",total);
};

/* ==================================================================
   vfs_getpathcut(name):
   *Returns the full path of the pathcut specified
   *returns -1 if no such pathcut exists, otherwise it returns a static
    pointer to the full path.
   */
const char *vfs_getpathcut(const char *name)
{
    path_cut *ptr = vfs_path_cut_head;
    while (ptr!=0)
    {
        if (strcmp(name, ptr->name)==0)
           {
                return ptr->path;
           };
           ptr=ptr->next;
    };
    
    return -1;
};

/* ==================================================================
   vfs_ispathcut(name):
   *tells if a name is a potential pathcut
   *returns 1 if a pathcut, zero if it is a normal name.
   */
int vfs_ispathcut(const char *name)
{
    int length = strlen(name);
    
    //check if this is a probable pathcut
    if ( name[length-1] == ':') return 1;
    
    return 0;
};



/* ==================================================================
   vfs_listdir:
   *list the contents of a directory by placing the vfs_node
    information into a buffer. Return value is the actual number
    of files placed into the buffer. The parameter size refers to the
    size of the buffer refered to by "buffer". 
   
   *Passing a parameter of NULL to buffer
   or 0 to size will cause listdir to simply return the total number
   of files in the current directory.
   
   *Notes: The ideal way of using this is to first determine the 
   number of files by passing NULL values to either size or buffer and
   then allocating the necessary amount. Then call vfs_listdir again
   to plce the file info into the buffer. Use the new total number of files
   returned since it might be possible that the number of files changed
   since the last vfs_listdir call.
   
   Refer to console_ls() to see an example usage.
   */
   
int vfs_listdir(vfs_node *current_dir,vfs_node *buffer,int size)
{

    vfs_node *dptr=current_dir;
    vfs_node *fileptr;
    int totalfiles=0;
    int maxbuffer = size / sizeof(vfs_node);
    
    
    
    //The directory might not yet be mounted so mount it 
    if (dptr->files == VFS_NOT_MOUNTED) vfs_mountdirectory(dptr);
    
    //enter critical section, prevent other VFS operations from\
    //taking plce until we leave
    sync_entercrit(&vfs_busy);
    
    //obtain the starting VFS file node
    fileptr = dptr->files;
    
    
    while (fileptr!=0&&fileptr->name!=0)
    {
        if (buffer!=0 && size!=0 && (totalfiles < maxbuffer) )
           {
               memcpy(&buffer[totalfiles],fileptr,sizeof(vfs_node));
           };
        fileptr=fileptr->next;
        totalfiles++;
    };
    
    //Leave critical section
    sync_leavecrit(&vfs_busy);
    
    return totalfiles;
};

/*========================================================================
char *vfs_attbstr(vfs_node *node)
This function obtains the file attribute of a node and then converts
it into a string that is used by the ls command*/
char *vfs_attbstr(vfs_node *node,char *buf)
{
    if (node->attb & FILE_DIRECTORY) buf[0]='d';
      else
    buf[0] = '-';

    if (node->attb & FILE_OEXE) buf[1]='x';
      else
    buf[1] = '-';
    
    if (node->attb & FILE_OREAD) buf[2]='r';
      else
    buf[2] = '-';
    
    if (node->attb & FILE_OWRITE) buf[3]='w';
      else
    buf[3] = '-';
    
    buf[4] =0;
    return buf;
};

/*Removes a directory based on the name*/
int rmdir(const char *name)
{
    vfs_node *dir = vfs_searchname(name);
    if (dir!=0)
         return vfs_rmdir(dir);
    return -1;
};

/*========================================================================
  int vfs_rmdir(vfs_node *ptr)
  This function removes a directory including all of its contents, 
  returns -1 if at least one file was unsuccessfully deleted.*/
  
int vfs_rmdir(vfs_node *node)
{
vfs_node *ptr = node->files;
int retval = 0;
/* 1. This is a mountpoint? Use vfs_unmount instead
   2. Is this directory read-only? */
if ( (node-> attb & FILE_MOUNT) || !(node->attb&FILE_OWRITE)) return -1;

    //make sure the ptr is a directory
    if (node->attb & FILE_DIRECTORY)
        {
             /*Make sure that there are no open files under this directory or
               devices mounted under this directory*/
             if (vfs_checkopenfiles(node)==0)
                {
                    do {
                           vfs_node *next_ptr = ptr->next;
                                        
                            /*Use fdelete on each file, if it is another directory, 
                              recursively use vfs_rmdir on it*/
                            if ( (strcmp(ptr,"..")!=0) && (strcmp(ptr,".")!=0) )
                            { 
                                if ( ptr->attb & FILE_DIRECTORY 
                                     )
                                   {
                                       if (ptr->files == VFS_NOT_MOUNTED)
                                             if (vfs_mountdirectory(ptr)==0) return -1;
                                       if (vfs_rmdir(ptr)==-1) return -1;
                                   };  
                                if (vfs_deletefile(ptr)==-1) return -1;
                            };
                            ptr = next_ptr;
                            
                       } while (ptr!=0);
                    
                }
            else
               return -1;                 
        };
        
    vfs_deletefile(node);    
    return 1;
};

