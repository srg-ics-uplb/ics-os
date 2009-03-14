/*
 *   Name: vfs_core.h
 *   Copyright: 
 *   Author: Joseph Emmanuel DL Dayo
 *   Date: 30/01/04 05:25
 *   Description: This module defines the core functions of the DEX VFS (Virtual File
 *                System)
 
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



DWORD mount_table[255],total=0; //store the address of mount functions

path_cut *vfs_path_cut_head=0;
file_PCB *file_globalopen=0;
int vfs_nextfileid = 1;
vfs_node *vfs_root=0,*homedir; //lists the pointer to the root and the current working
                       //directory for the current user
sync_sharedvar vfs_busy; //used for the busy waiting loops inside the vfs


//Prototype of some functions
vfs_node *vfs_searchname(const char *name);
int file_ok(file_PCB* fhandle); //validates a file handle
char *showpath(char *s);


/***********************************************************************************
vfs_init - Initializes the VFS (Virtual File System) and creates the root directory*/

void vfs_init()
{
    devmgr_fs_desc *fs;
    vfs_root=(vfs_node*)malloc(sizeof(vfs_node));
    strcpy(vfs_root->name,""); //root has a name of NULL
    memset(vfs_root,0,sizeof(vfs_node));
    vfs_root->attb=FILE_DIRECTORY | FILE_MOUNT;
    vfs_root->path=0;
    vfs_root->next=0;
    vfs_root->memid=-1;//floppy_deviceid;
    vfs_root->files=0;
    vfs_root->fsid =-1;//fat12_deviceid;
    vfs_root->misc=0;
    vfs_root->miscsize=0;
    current_process->workdir=vfs_root;
    homedir = vfs_root;
    file_globalopen=0;
    memset(&vfs_busy,0,sizeof(vfs_busy));
};


//checks a directory vfs_node if any of its child nodes is opened by a process
vfs_node *vfs_checkopenfiles(vfs_node *ptr)
{
    vfs_node *tmp=ptr->files;
    
    if ( tmp==0 || ptr==0 || tmp == VFS_NOT_MOUNTED ) return 0;
    
    do
    {
        vfs_node *next=tmp->next;
        //check if there is another device mounted 
        if (tmp->attb&FILE_MOUNT) 
        {
            return -1;
        };

        //If this is a directory perform the same operation recursively      
        if (tmp->attb&FILE_DIRECTORY) 
        {
            int ret = vfs_checkopenfiles(tmp);
            if (ret !=0) return ret;
        }

        //file opened, return vfs_node of that file
        if (tmp->opened>0) 
         return tmp;
        tmp=next;
    } 
    while (tmp!=0);
    
    //No mounted devices and no open files  
    return 0;
};

/***********************************************************************************
 * vfs_directread - This is a core function that communicates with the corresponding
 * filesystem driver of the file in order to read data from a file.*/

int vfs_directread(char *buf,int itemsize,int noitems,file_PCB* fhandle)
{
    DWORD size=itemsize*noitems,read;
    devmgr_fs_desc *fs;
    char temp[200];
    if (fhandle!=0)
    {
        if (file_ok(fhandle)) //validate this handle
        {
            int fs_deviceid;
            DWORD start=ftell(fhandle);
            
            /*reached the end of the file?
              obtain the filesystem to use*/
              
            fs_deviceid = fhandle->ptr->fsid;
            if (fs_deviceid==-1) return 0;

            if (fhandle->ptrlow+size>=fhandle->ptr->size)
            {
                read=fhandle->ptr->size-fhandle->ptrlow;
                fhandle->ptrlow=fhandle->ptr->size;
                ;
            }
            else
            {
                read=size;
                fhandle->ptrlow+=size;
            };
            
            if (read==0) return 0;

            fs=(devmgr_fs_desc*)devmgr_devlist[fs_deviceid];

            //tell the filesystem to read the file
            if (fs->readfile!=0)
                bridges_call(fs,&fs->readfile,fhandle->ptr,buf,
                       start,start+read-1,fhandle->ptr->memid);         
            else
                return 0;

            return read;
        };
        return 0;
    }; 
    return 0;
    ;
};

/***********************************************************************************
 * vfs_directwrite - This is a core function that communicates with the corresponding
 * filesystem driver of the file in order to write data to a file.*/

int vfs_directwrite(char *buf, int itemsize, int n, file_PCB* fhandle)
{
    DWORD size=itemsize*n,write;
    devmgr_fs_desc *fs;
    int bytes_per_allocation_unit=0;    

    if (fhandle!=0)
        if (file_ok(fhandle))
        {
            int fs_deviceid = 0;   
            DWORD start = ftell(fhandle);

            fs_deviceid = fhandle->ptr->fsid;
            if (fs_deviceid==-1) return 0;
            fs=(devmgr_fs_desc*)devmgr_getdevice(fs_deviceid);
            //if required
            if (start+size>=fhandle->ptr->size)
            {
                //compute if additional sectors are necessary
                DWORD totalblocks, neededblocks;
                
                //determine the smallest allocation unit that this filesystem supports
                //and check if we need more blocks
                bytes_per_allocation_unit = bridges_call(fs,&fs->getbytesperblock,fhandle->ptr->memid);
                totalblocks=fhandle->ptr->size/bytes_per_allocation_unit+1;
                neededblocks=(start+size)/bytes_per_allocation_unit+1;

                if (fhandle->ptr->size==0)
                {
                    totalblocks=0;
                };

                if (neededblocks>totalblocks) //more blocks needed so we request for one
                {

                    //update the size of the file
                    //Tell the filesystemdriver to add sectors
                    if (fs->addsectors!=0)
                    {
                        int res = bridges_call(fs,&fs->addsectors,fhandle->ptr,
                               neededblocks-totalblocks,fhandle->ptr->memid); 
                        if (res == -1) return 0;
                    }
                    else
                        return 0; //filesystem does not support add sectors    
                };

            };

           
            //tell the filesystem driver to write to the file
            if (fs->writefile!=0)
                bridges_call(fs,&fs->writefile,fhandle->ptr,buf,start,
                                     start + size - 1,fhandle->ptr->memid);         
            else
                return 0; //filesystem does not support writes   

            //update filesize if necessary                               
            if (fhandle->ptrlow+size>=fhandle->ptr->size)
            {
                write=size;
                fhandle->ptrlow+=size;
                
                if (fs->chattb)
                {
                        fhandle->ptr->size=fhandle->ptrlow;
                        //Tell the filesystem to update the size of the file on the disk   
                        bridges_call(fs,&fs->chattb,fhandle->ptr,FILE_FSIZE,fhandle->ptr->memid);         
                };
            }
            else
            {
                write=size;
                fhandle->ptrlow+=size;
            };
            return write;
        };
    return 0;
};

/***********************************************************************************
 * vfs_readchar - This function handles buffered reads. Unlike vfs_directwrite (Although this function
 * uses directread internally), vfs_readchar is optimized for single character reads,
 * making getc based functions perform fast.*/

int vfs_readchar(file_PCB *handle, char *character)
{

    //validate file handle
    if (file_ok(handle))
    {
        //get our current position
        int pos = ftell(handle);
        //get the position in the buffer
        int relative_position = pos - handle->startptr;

        //check if read is within the size of the file
        if (pos>=handle->ptr->size) return 0;

        //If the file has a buffer type of FILE_IONBF, don't use buffering
        //and instead read directly
        if (handle->buffer == 0 || handle->buffertype == FILE_IONBF)
        {
            return vfs_directread(character,1,1,handle);    
        };

        //check if the file pointer is whitin the range of the buffer, if so we can
        //just get the character sotred in the buffer    
        if (handle->startptr <= pos && pos < handle->startptr + handle->endsize)

        {
            *character = handle->buffer[relative_position];  
            handle->ptrlow = pos + 1;   
            return 1;   
        }
        //The file pointer is not within the range of the buffer, therefore the requested
        //character is not yet available. We now flush the buffer to save its current contents
        //and then read from the disk to get the requested data.   
        else
        {
            vfs_flushbuffer(handle);
            int size_to_read;
            //update the starting position of the buffer
            handle->startptr = pos;
            handle->endsize =  0;

            if (handle->ptr->size - pos < handle->bufsize)
                size_to_read = handle->ptr->size - pos ;
            else
                size_to_read = handle->bufsize;
                
            //printf("read size = %d ", size_to_read);    
            fseek(handle, pos,SEEK_SET);
            if (vfs_directread(handle->buffer, size_to_read , 1 ,handle)<1) return 0;

            *character = handle->buffer[0];
            handle->endsize = size_to_read;
            handle->ptrlow = pos + 1;   
            return 1;   
        };        
        return 0;
    };
    return 0;
};

/***********************************************************************************
 * vfs_writechar - This function handles buffered writes*/

int vfs_writechar(file_PCB *handle, char character)
{
    //validate handle
    if (file_ok(handle))
    {
        int pos = ftell(handle);
        int relative_position = pos - handle->startptr;

        /*determine if there is a buffer, if there is none, perform fwrite,
         *           if there is, check first if it is within the buffers' range otherwise
         *           flush and reset thebuffer's position*/

        //No buffer?  
        if (handle->buffer == 0 || handle->buffertype == FILE_IONBF)
        {
            return vfs_directwrite(&character,1,1,handle);
        };
        //If it reached this point then there is a valid buffer, now
        //we check if it is within range.
        if  ( handle->startptr<=pos && pos < handle->startptr + handle->bufsize &&
            ( character!='\n' || handle->buffertype == FILE_IOFBF) ) 
        {

            handle->buffer[relative_position] = character;
            //adjust the size of the data in the buffer
            if (relative_position + 1 >= handle->endsize)
                handle->endsize = relative_position + 1;
            handle->ptrlow = pos + 1;   
            handle->bufferwrite = 1;
            return 1;   
        }
        else
            //buffer out of range or too small
        {
            //flushdata
            vfs_flushbuffer(handle);
            //update the starting position of the buffer
            handle->startptr = ftell(handle);
            handle->endsize = 0;
            handle->bufferwrite = 1;
            handle->buffer[0] = character;
            handle->endsize = 1;
            handle->ptrlow = pos + 1;   
            return 1;   

        };


        return -1;
    };
    return -1;
};

/*****************************************************************************
vfs_flushbuffer - commits the buffer to the device
*/

int vfs_flushbuffer(file_PCB *handle)
{
    //validate handle
    if (file_ok(handle))
    {
        if (handle->buffertype!=FILE_IONBF && handle->buffer!=0 && handle->endsize!=0 &&
            handle->bufferwrite)
        {
            int prev_position = ftell(handle);
            fseek(handle,handle->startptr,SEEK_SET);
            vfs_directwrite(handle->buffer,handle->endsize,1,handle);
            fseek(handle,prev_position,SEEK_SET);
            handle->endsize=0;
            handle->bufferwrite = 0;
        };
        return 1;
    };
    return 0;
};

/******************************************************************************
- sets and initializes the file's buffer */

int vfs_setbuffer(file_PCB *handle,char *buffer,int bufsize,int mode)
{
    //validate handle
    if (file_ok(handle))
    {
        /*Determine current status of the buffer*/
        if (handle->buffer!=0) 
        { 
            if (handle->bufferwrite) vfs_flushbuffer(handle);
            if (!handle->userbuffer) free(handle->buffer);
            handle->buffer = 0;
        };  

        /*full buffering requested,flush only when closedor when buffer is full*/
        if (mode == FILE_IOFBF || mode == FILE_IOLBF)
        {
            handle->buffertype = mode;


            if (buffer!=0) 
            {
                handle->buffer = buffer;
                handle->userbuffer = 1;
            }
            else
            {
                if (bufsize == 0) bufsize = FILE_BUFSIZE;
                handle->buffer=(char*)malloc(bufsize);
                handle->userbuffer = 0;
            };

            handle->bufsize = bufsize;
            handle->buffertype = mode;
            handle->startptr=ftell(handle);
            handle->endsize = 0;
        }       
        else
            /*No buffering was requested, write everytime*/
            if (mode == FILE_IONBF)
            {
                handle->buffertype = FILE_IONBF;
                if (handle->buffer!=0&&!handle->userbuffer) free(handle->buffer);
                handle->userbuffer = 0;
                handle->buffer = 0;
                handle->bufsize = 0;
            };
    };

};

int vfs_checkdirused(vfs_node *ptr)
{
    PCB386 *ptr2;
    int total = get_processlist(&ptr), i;
    
    for (i=0; i < total ;i++)
    {
        if (ptr2[i].workdir == ptr )
               return 1; 
    };
    return 0;
};

int vfs_freedirectory(vfs_node *ptr)
{
    vfs_node *tmp=ptr->files;
    
    if (tmp==VFS_NOT_MOUNTED) {ptr->files = 0; return 1;};
    
    if (tmp==0||ptr==0) return 0;
    
    do
    {
        vfs_node *next=tmp->next;
        if (tmp->attb&FILE_DIRECTORY) vfs_freedirectory(tmp);

        if (tmp->miscsize!=0) free(tmp->misc);
        if (tmp->miscsize2!=0) free(tmp->misc2);
        free(tmp);

        tmp=next;
    } 
    while (tmp!=0);
    ptr->files=0;
    return 1;
};

int vfs_unmount_device(const char *location)
{
     vfs_node *node=vfs_searchname(location);
     vfs_unmount(node);
};

int vfs_unmount(vfs_node *node)
{
    //get the vfs_node of the location
    if (node!=0)
    {
        //make sure that the location is a mount point
        if (node->attb&FILE_MOUNT)
        {
            devmgr_fs_desc *fs;
            devmgr_block_desc *myblock;
                                
            printf("checking open files ..\n");                
            //make sure that there are no open files
            if (vfs_checkopenfiles(node)!=0) 
            {
            printf("vfs: Cannot unmount if volume has open files.\n");
            return -1;
            };
            
            //make sure that no process is using it as the current directory
            if (vfs_checkdirused(node)!=0)
            {
            printf("vfs: cannot unmount if a process is using it as a working directory\n");
            return -1;
            };
            
            //If the filesystem provides an unmount command, notify it
            //that it will be unmounted
            printf("obtaining interface\n");
            
            fs = devmgr_getdevice(node->fsid); //obtain the device itnerface            
            myblock = devmgr_getdevice(node->memid);
            
            printf("Calling unmount function\n");
            if (fs!=-1) //check if exists
            { 
            //check if fs has an unmount function
                   if (fs->unmount)           
                   bridges_call(fs,&fs->unmount,node);//do it!         
            };
            
            //unlock the mounted device
            devmgr_setlock(node->memid,0);
            
            printf("deallocating child nodes..\n");
            //deallocate memory used by child nodes
            vfs_freedirectory(node);
            
            //free the miscellanous elements of the vfs node
            if (node->miscsize!=0) free(node->misc);
            if (node->miscsize2!=0) free(node->misc2);
            
            /*Call the invalidate cache function of a device if it has one, 
              so that in case a new block device was inserted (like floppy), the
              data in the cache will not be inconsistent*/
            if (myblock!=-1) 
            if (myblock->invalidate_cache!=0)
            bridges_call(myblock,&myblock->invalidate_cache);
            
            printf("Removing node\n");
            //remove the mountpoint from the vfs
            vfs_removenode(node);
            
            //free the memory allocated to the node
            free(node);
            
            //done
            return 1;
        };
        return -1;
    };
    return -1;
};


vfs_node *mkvirtualdir(const char *name,int fsid,int deviceid);
//This function "mounts" a device to the filesystem. Mounting means that
//the files and or directories on a device gets to be mapped to the VFS so that
//it becomes visible to the operating system.
int vfs_mount_device(const char *fsname,const char *devname,const char *location)
{
    devmgr_fs_desc *myfs;
    devmgr_block_desc *myblock;
    int retval=0;
    
    if (fsname == 0 ) //autodetect file system?
    {
        printf("function not yet implemented\n");
        return -1;
        // NOT YET IMPLEMENTED
    }
    else
    {

        int fsid, devid;
        vfs_node *mount_location;
        //get the device id of the devices requested
        fsid = devmgr_finddevice( fsname);
        if (fsid == -1)
	{
           printf("Unknown filesystem type: %s\n",fsname);
	   return -1;
	}
        devid = devmgr_finddevice(devname);
        if (devid == -1) 
        {
          printf("Unknown device name: %s\n",fsname);
          return -1;
        }

        //tell the device manager to obtain the interfaces for the
        //filesystem and block device driver
        myfs = (devmgr_fs_desc*) devmgr_getdevice(fsid);
        myblock = (devmgr_block_desc*) devmgr_getdevice(devid);

        /**************** Validate the interfaces******************/
        
        // the device driver selected is not a filesystem
        if (myfs->hdr.type != DEVMGR_FS)  return -1;

        // the device driver selected is not a block device
        if (myblock->hdr.type != DEVMGR_BLOCK){
           printf("Not a valid block device.\n");
           return -1;
        }

        //Make sure that the device has not already been mounted...
        if (devmgr_getlock(devid))
	{
           printf("%s device is already mounted.\n", devname);
	   return -1;
	}


        //obtain the vfs_node for the mountpoint
        if (location==0){
            printf("No mount point specified.\n");
            return -1;
        }
        else{
            mount_location= vfs_searchname(location);
        }    
        if (mount_location == 0) //create directory
        {
            mount_location = mkvirtualdir(location,fsid,devid);
        }
        else{
		printf("%s device is already mounted.\n", devname);
        	return -1; //mountpoint already exists!!
	}
        
        mount_location->attb|= FILE_MOUNT;
        
        //lock the device to prevent it from being mounted again
        devmgr_setlock(devid,1);

        //everything seems to be in order, mount the device using the selected filesystem
        retval = bridges_link(myfs,&myfs->mountroot,mount_location,devid,0,0,0,0);
        
        if (retval == -1) //unsuccessful mount
        {
                    vfs_unmount(mount_location);
        };
        //return myfs->mountroot(mount_location, devid);
        return retval;
    };  
    //return -1;
};

//opens a file, supports file locking if opened for writing,append
file_PCB *openfilex(char *filename,int mode)
{
    DWORD sectsize,flag;
    int fs_device;
    file_PCB *fcb;
    
    vfs_node *ptr=vfs_searchname(filename);

#ifdef DEBUG_VFSREAD
    printf("openfilex %s called\n",filename);
#endif

    if (ptr!=0)
    {
        fs_device = ptr->fsid;
        if (fs_device==-1) return 0;
    };

    if (ptr == 0 && (mode == FILE_READWRITE || mode == FILE_WRITE || mode == FILE_APPEND) ) 
        ptr=createfile(filename,0);
    else
    if (ptr!=0 && (mode == FILE_WRITE) && !ptr->locked && !ptr->opened)
        {
            devmgr_fs_desc *fs; 
            fs=(devmgr_fs_desc*)devmgr_devlist[fs_device];
            
            //make sure the filesystem supports this command           
            if (fs->rewritefile!=0)
                        bridges_call(fs,&fs->rewritefile,ptr,ptr->memid);
            else
                        return 0;
        };

    //check for file locking    
    if ( (ptr==0) || (ptr->locked) || (mode!=FILE_READ && ptr->opened))
    {
#ifdef WRITE_DEBUG
        printf("openfilex failed.\n");
#endif
        return 0;
    };   

    ptr->opened++;



    sync_entercrit(&vfs_busy);
    //create the file_PCB list which maintains the list of
    //opened files!!!!

    fcb = (file_PCB*)malloc(sizeof(file_PCB));    
    memset(fcb,0,sizeof(file_PCB));    
    
    //use add to head method
    if (file_globalopen==0)
    {
        file_globalopen = fcb;
        fcb->next = 0;
        fcb->prev = 0;
    }
    else
    {
        fcb->next = file_globalopen;
        fcb->prev = 0;
        file_globalopen->prev = fcb;
        file_globalopen = fcb;
    };   

    if (mode==FILE_WRITE) /*write mode active?? lock the file*/
    {
        ptr->locked=1;
        fcb->locked=1;
    };

    fcb->bufsize=0;
    fcb->buffer=0;
    fcb->fileid = vfs_nextfileid++;

    fcb->processid=current_process->processid;
    fcb->mode=mode;
    fcb->ptr=ptr;

    /*intialize the file pointers ( the pointers used for fread to determine
                                     the position in the file)*/
    if (mode==FILE_APPEND) fcb->ptrlow=ptr->size;
    else
        fcb->ptrlow=0;
    fcb->ptrhigh=0;

    //set file buffering mode using Full buffering
    vfs_setbuffer(fcb,0,512,FILE_IOFBF);

    sync_leavecrit(&vfs_busy);

    //printf("openfile(%s) called handle value %d\n",filename,file_nextptr);
    return fcb;
};

/* ==================================================================
   char * vfs_getcwd (char *buffer, size_t size)
   *returns the current working directory name in buffer.
   *"size" is the size of the target buffer that is used.
   
   ====================================================================*/
   
char * vfs_getcwd (char *buffer, unsigned int size)
{
    char *workdir_name = current_process->workdir->name;
    int i;
    
    if (buffer==0) return -1;
    
    for (i=0;i< (size-1) && workdir_name[i]; i++)
      {
          buffer[i] = workdir_name[i];
      }; 
    buffer[i] = 0;

    return buffer;
};

int file_ok(file_PCB* fhandle)
{
    int retval = 0;
    file_PCB *ptr=file_globalopen;
    if (fhandle==0) return 0;

    sync_entercrit(&vfs_busy);
    while (ptr!=0)
    {
        if (ptr==fhandle) {
            retval=1;
            break;
        };
        ptr=ptr->next;
        ;
    };

    sync_leavecrit(&vfs_busy);  
    return retval;
};



//closes all the files a process has opened
int closeallfiles(DWORD pid)
{
    file_PCB *ptr,*next;
    int retval = 0;

    //wait until the vfs is ready
    sync_entercrit(&vfs_busy);

    ptr = file_globalopen;
    if (pid==0)  retval = 0;
    else
        while (ptr!=0)
        {
            if (ptr->processid==pid)
            {
                fclose(ptr);
                retval = 1;
                break;
            };
            ptr=ptr->next;
        };

    sync_leavecrit(&vfs_busy);
    return retval;
};

//for buffered I/O, this function commits the writes to the device
int fflush(file_PCB* fhandle)
{
    //saves all the changes of the file
    /*nothing to implement yet.... file writing is still not
     *      supported*/
    if (file_ok(fhandle)) 
    if (fhandle->bufsize && fhandle->buffer)
    {
        vfs_flushbuffer(fhandle);
    };
    
    return 1;
};

/* Works exactly the same as the stdlib function feof which returns
   1 if is alredy the end of the file, 0 otherwise*/
int feof(file_PCB* fhandle)
{
    if (fhandle!=0)
        if (file_ok(fhandle))
        {
            if (fhandle->ptrlow>=fhandle->ptr->size)
                return 1;
            else
                return 0;
        };
    return 0;
};


char *vfs_readline(char *s,int n,const char term, file_PCB *fhandle)
{
    devmgr_fs_desc *fs;
    int safesize=0,i;
    int fs_device = 0;

    if (fhandle!=0)
        if (file_ok(fhandle))
        {
            char *temp=(char*)malloc(n+1);
            //perform some checks
            if (feof(fhandle)) { free(temp);return 0;};
            if (fhandle->ptrlow+n>fhandle->ptr->size)
            {
                safesize=fhandle->ptr->size-fhandle->ptrlow;
                ;
            }
            else
                safesize=n;

            //determine what filesystem to use
            fs_device = fhandle->ptr->fsid;
            if (fs_device == -1) {free(temp);return;};
            //Tell the filesystem driver to read the file
            fs=(devmgr_fs_desc*)devmgr_devlist[fs_device];
            fs->readfile(fhandle->ptr,temp,fhandle->ptrlow, fhandle->ptrlow+safesize,fhandle->ptr->memid);

            //copy characters until \n is detected
            for (i=0;i<safesize;i++)
            {
                if (temp[i]==term)
                {
                    s[i]=0;
                    i=i+1;
                    break;
                };
                s[i]=temp[i];
            };
            fhandle->ptrlow+=i;
            s[i]=0;//append null terminator
            free(temp);
            return s;
        };
    return 0;
};

/*Gets a string from a file including the \n character*/
char *fgets(char *s,int n,file_PCB *fhandle)
{
    int i;
    char *retval = 0;
    char c;
    if (file_ok(fhandle))
    {
        for (i=0;i<n-1;i++)
        {

            if (fread(&c,1,1,fhandle)!=0)
            {
                s[i]=c;
                if (c=='\n')
                break;
            }
            else 
            /*EOF reached?*/
            {
                s[i+1]=0;  
                return  0;      
                break;
            };
        };
        
        s[i+1]=0;
        retval = s;       
    };
    return retval;
};

void vfs_fillstat(vfs_stat *stat, vfs_node *node)
{
       memset(stat,0,sizeof(vfs_stat));
       stat->st_dev = node->memid;
       stat->st_rdev = node->fsid;
       stat->st_ino = node->start_sector;
       stat->st_size = node->size;
};

int fstat(file_PCB *fhandle,vfs_stat *statbuf)
{
    vfs_stat file_info;
    if (fhandle!=0)
        if (file_ok(fhandle))
        {
            vfs_fillstat(&file_info,fhandle->ptr);
            memcpy(statbuf,&file_info,sizeof(vfs_stat));
            return 0;
        };
    return -1;
};

int vfs_getstat(const char *filename,vfs_stat *statbuf)
{
    vfs_node *ptr =vfs_searchname(filename);
    vfs_stat file_info;
    if (ptr!=0)
    {
        vfs_fillstat(&file_info,ptr);
        memcpy(statbuf,&file_info,sizeof(vfs_stat));
        return 0;
    };
    return -1;
};

long int ftell(file_PCB *fhandle)
{
    if (fhandle!=0)
        if (file_ok(fhandle)) //validate this handle
        {
            return fhandle->ptrlow;
        };
    return -1;
};


//renames a file
int rename (const char *oldname, const char *newname)
{

    vfs_node *filenode=vfs_searchname( oldname);

    if (filenode!=0 && !filenode->locked)
    {
        int fs_deviceid;
        devmgr_fs_desc *fs;
        vfs_node *check=vfs_searchname(newname); 
        if (check!=0) //A file with that name already exits?
        { 
            //check if they are in the same directory
            if (check->path==filenode->path) return 0;  
        };

        //update filename in VFS   
        strcpy(filenode->name,newname);

        //obtain the file system driver to read this file
        fs_deviceid = filenode->fsid;
        if (fs_deviceid==-1) return 0;
        fs=(devmgr_fs_desc*)devmgr_devlist[fs_deviceid];
        //the the file system driver to change the filename
        fs->chattb(filenode,FAT12_FNAME,filenode->memid);

        return 1;     
    };

    return 0;  
};

//adjust the file pointer of a file
int fseek(file_PCB *fhandle, long offset, int whence)
{
    if (fhandle!=0)
        if (file_ok(fhandle)) //validate this handle
        {
            if (whence==SEEK_SET)
                fhandle->ptrlow=offset;
            else
                if (whence== SEEK_CUR)
                    fhandle->ptrlow+=offset;
                else
                    if (whence== SEEK_END){
                        fhandle->ptrlow=fhandle->ptr->size-offset;
                    }

            if (fhandle->ptrlow>fhandle->ptr->size) fhandle->ptrlow=fhandle->ptr->size;
            ;
        };
    return -1;
};

//used to read data from a file
int fread(char *buf,int itemsize,int noitems,file_PCB* fhandle)
{
    DWORD size=itemsize*noitems,i;
    devmgr_fs_desc *fs;
    char temp[200];
#ifdef DEBUG_VFSREAD
    printf("fread called\n");
#endif
    if (fhandle!=0)
    {
        if (file_ok(fhandle)) //validate this handle
        {
            //for large reads, do not use the buffer
            if (size>FILE_BUFSIZE)
            {
                fflush(fhandle);      
                return vfs_directread(buf,itemsize,noitems,fhandle);
            };    

            for (i=0;i<size;i++)
                if (vfs_readchar (fhandle,&buf[i])!=1) break; 
            return i;
        };
        return 0;
    }; 
    return 0;
    ;
};

int fgetsectors(file_PCB* fhandle)
{
    devmgr_fs_desc *fs;
    if (fhandle!=0)
        if (file_ok(fhandle))
        {
            fs=(devmgr_fs_desc*)devmgr_getdevice(fhandle->ptr->fsid);
            if (fs==-1) return 0;
            return fs->getsectorsize(fhandle->ptr,fhandle->ptr->memid);
        };
    return 0;
};

char *getfullpath(vfs_node *node,char *s);
/* Copies a file from one location to another, if a file with the same
name already exists in the destination, fcopy will return an error*/
int fcopy(char *source, char *dest)
{
    int i;
    char *buf;
    DWORD size;
    vfs_stat fileinfo;
    file_PCB *handle,*desthandle;
    vfs_node *destfile,*srcfile;
    //open the source file
    
    handle=openfilex(source,FILE_READ);
    //create the destination file
    destfile = vfs_searchname(dest);

    //source file must be present
    if (!handle) return -1;      

    //file already exists?
    if (destfile) 
    {
        if (destfile->attb&FILE_DIRECTORY)
        {
            char newdest[255];
            srcfile = vfs_searchname(source);
            getfullpath(destfile,newdest);
            strcat(newdest,srcfile->name);
            printf("copying to %s..\n",newdest);
            desthandle=openfilex(newdest,FILE_WRITE);
            if (!desthandle) return -1;              
        }
        else
        { 
            //file already exists!!!    
            fclose(desthandle); 
            return -1;
        };
    }
    else
    {
        desthandle=openfilex(dest,FILE_WRITE); 
        if (desthandle == 0) return -1;
    };    
    
    fstat(handle,&fileinfo);      

    //set up buffer to optimize reads and writes
    vfs_setbuffer(handle,0,(fileinfo.st_size < 0x100000) ? fileinfo.st_size : 0x100000,FILE_IOFBF);      
    vfs_setbuffer(desthandle,0,(fileinfo.st_size < 0x100000) ? fileinfo.st_size : 0x100000,FILE_IOFBF);

    //get the size of the source file
    size= fileinfo.st_size;
    printf("Copying %d bytes...\n", size);

    //allocate memory
    buf=(char*)malloc(size);

    printf("reading source file..\n");
    fread(buf,size,1,handle);

    printf("writing source file..\n");
    if (fwrite(buf,size,1,desthandle)==0)
    {
        /*Copy failed!!!*/
        fdelete(desthandle);
        fclose(handle);
        fclose(desthandle);
        return -1;
    }
    else
        printf("copy done.\n");

    free(buf);
    fclose(handle);
    fclose(desthandle);
    
    return 1;
};

//Delete a vfs_node from a singly-linked list structure
int vfs_removenode(vfs_node *node)
{
    int ret,found=0;
    //get the files(Head) pointer of the parent directory of this node
    vfs_node *parent;
    vfs_node *ptr,*prev;
    parent = node->path;
    ptr = parent->files;
    //check the case when the file is the first entry
    if (ptr==node)  
    {
        parent->files = node->next;
        found = 1;	 
    }
    else
    {
        prev = 0;
        
        do{
        
            if (ptr==node)
            {
                prev->next=node->next;
                found=1;
                break;
            };
            prev=ptr;
            ptr=ptr->next;
        } 
        while (ptr!=0);
    };
    return found;
};

/*Deletes a file based on a vfs_node pointer*/
int vfs_deletefile(vfs_node *ptr)
{
    devmgr_fs_desc *fs;
    int fs_deviceid=0, ret;
    
    if (ptr->locked) return -1; //file is open, cannot delete
    if (!vfs_removenode(ptr)) return -1;

    //Determine device ID of the filesystem to use
    fs_deviceid = ptr->fsid;
    if (fs_deviceid==-1) return 0;
    
    fs=(devmgr_fs_desc*)devmgr_getdevice(fs_deviceid);
    
    /* Tell filesystem driver to delete the file using an intermodule call
       if there is an deletefile function available for the device */
    if (fs->deletefile!=0)
        ret = bridges_call(fs, &fs->deletefile, ptr, ptr->memid);
    else
        ret = -1;
        
    free(ptr);        
    return ret;    
};

//closes and deletes a file based on a handle
int fdelete(file_PCB *fhandle)
{
    int ret;
    
    if (fhandle!=0)
        if (file_ok(fhandle))  //validate the handle provided
        {
            /*Obtain the vfs node associated with this File handle*/    
            vfs_node *file=fhandle->ptr;
            
            if (fclose(fhandle) == -1) return -1;
            
            //Delete the file
            ret = vfs_deletefile(file);
            
            return ret;
        };
        
    return -1;
};

int fwrite(char *buf, int itemsize, int n, file_PCB* fhandle)
{
    DWORD size=itemsize*n,i;
    devmgr_fs_desc *fs;    
#ifdef WRITE_DEBUG
    printf("fwrite(): called, itemsize %d. \n",size);
#endif

    if (fhandle!=0)
        if (file_ok(fhandle))
        {
            if (size>FILE_BUFSIZE)
            {
                fflush(fhandle);     
                return vfs_directwrite(buf,itemsize,n,fhandle);
            };
            for (i=0;i<size;i++)
                if (vfs_writechar(fhandle,buf[i])!=1) break;       
            return i;   
        };
#ifdef WRITE_DEBUG
    printf("Write failed, handle passed = %d.\n",fhandle);
#endif
    return 0;
};

/*fclose closes a file handle, it returns 0 on success, -1 on an error otherwise*/
int fclose(file_PCB *fhandle)
{
    //validate handle given
    if (fhandle!=0)
        if (file_ok(fhandle)) //validate this handle
        {
            
            //save all changes the file needs to do    
            if (!fflush(fhandle)) return -1;
            fhandle->ptr->opened--;

           
            if (fhandle->buffer && !fhandle->userbuffer) free(fhandle->buffer);
            
            if (fhandle->locked&&fhandle->ptr->locked)
                fhandle->ptr->locked=0;
                
            //remove the file from the list of opened files
            if (fhandle->prev!=0)
                fhandle->prev->next = fhandle->next;
                
            
            if (fhandle->next!=0)
                fhandle->next->prev = fhandle->prev;
                
            
            if (file_globalopen == fhandle)
                file_globalopen = fhandle->next;                   
            free(fhandle);
            
            /*tell the I/O manager to commit all writes to the disk*/
            forceflush=1; 
            
            return 0;
        };
    return -1;
};

void findfile(char *name)
{
    vfs_node *ptr=vfs_searchname(name);
    if (ptr!=0)
    {
        printf("filename:%s size: %d \n",ptr->name,ptr->size);
    }else
    {
        printf("File not found.\n");
    }
};

vfs_node *mkvirtualdir(const char *name,int fsid,int deviceid)
{
    vfs_node *destdir; //obtain the destination directory
    vfs_node *node,*rollback;
    int i,fs_deviceid,retval;
    fatdirentry *dirs, *dirlocation;
    devmgr_fs_desc *fs;
    char path[255],dirname[255],fname[255];

    //Determine the directory vfs node to place the new diretory
    node=vfs_searchname(name);

    //If node is nonzero, a file or directory with <name> already exists
    if (node!=0) return -1;

    //since name is a constant we copy its contents to a temporary variable "path"
    strcpy(path,name);

    //separate the directory and filename information from the name of the full qualified path
    parsedir(path,dirname,fname);

    //get the vfs node of the destination directory
    destdir=vfs_searchname(dirname);

    //make sure that this directory exists
    if (destdir==0)
    {
        printf("error locating directory!\n");
        return -1;
    };

    //take note of the old values so that we could rollback
    //in case of an unsuccessful create file
    rollback = destdir->files;

    //allocate a new vfs node for this directory
    node=(vfs_node*)malloc(sizeof(vfs_node));
    memset(node,0,sizeof(vfs_node));

    //add the directory to the vfs
    vfs_createnode(node,destdir);

    //fill up some directory information
    strcpy(node->name,fname);
    node->attb = FILE_DIRECTORY;
    node->fsid = fsid;
    node->memid = deviceid;
    return node;
};

//creates a directory
int mkdir(const char *name)
{
    vfs_node *destdir; //obtain the destination directory
    vfs_node *node,*rollback;
    int i,fs_deviceid,retval;
    fatdirentry *dirs, *dirlocation;
    devmgr_fs_desc *fs;
    char path[255],dirname[255],fname[255];

    //Determine the directory vfs node to place the new diretory
    node=vfs_searchname(name);

    //If node is nonzero, a file or directory with <name> already exists
    if (node!=0) return -1;

    //since name is a constant we copy its contents to a temporary variable "path"
    strcpy(path,name);

    //separate the directory and filename information from the name of the full qualified path
    parsedir(path,dirname,fname);

    //get the vfs node of the destination directory
    destdir=vfs_searchname(dirname);

    //make sure that this directory exists
    if (destdir==0)
    {
        printf("error locating directory!\n");
        return -1;
    };

    //take note of the old values so that we could rollback
    //in case of an unsuccessful create file
    rollback = destdir->files;

    //determine the filesystem driver to use
    fs_deviceid = destdir->fsid;
    if (fs_deviceid==-1) return -1;


    //allocate a new vfs node for this directory
    node=(vfs_node*)malloc(sizeof(vfs_node));
    memset(node,0,sizeof(vfs_node));

    //add the directory to the vfs
    vfs_createnode(node,destdir);

    //set up the filesystem and the device that this directory is mounted on
    node->fsid  = destdir->fsid;
    node->memid = destdir->memid;
    
    //fill up some directory information
    strcpy(node->name,fname);
    node->attb =  destdir->attb&(0xFFFFFFFF^FILE_MOUNT);


    fs=(devmgr_fs_desc*)devmgr_getdevice(fs_deviceid);
    //Tell the filesystem driver to create the directory
    if (fs->hdr.type!=DEVMGR_FS)
    {
        printf("interface error\n");
        retval = -1;
    };
    
    if (fs->createfile(node,destdir->memid)==-1)
        retval = -1;
    else
        retval = 1;

    //rollback to previous state if there is trouble
    if (retval == -1)
    {
        //undo changes of vfs_createnode()
        destdir->files=rollback;
        free(node);
    };

    return retval;
};

//adds node to parent, automatically fills up the path,next and files attribute
//of the node and updates the file pointer of the parent directory
void vfs_createnode(vfs_node *node, vfs_node *parent)
{
    vfs_node *next_node = parent->files;
    
    if (next_node == VFS_NOT_MOUNTED) next_node = 0;
    
    
    node->next = next_node;
    if (next_node!=0) next_node->prev = node;
    parent->files = node;
    node->path = parent;
    node->files=0;
};


int vfs_isvalidname(const char *name)
{
    int i;
    for (i=0;name[i];i++)
        {
           switch(name[i])
                {
                  case '/' : return 0;
                  case ':' : return 0;
                  case '|' : return 0;
                  case '*' : return 0;
                  case '?' : return 0;
                  case '(' : return 0;
                  case ')' : return 0;
                  case '\\' : return 0;
                  case ' ' : return 0;
                };
        };
    return 1;
};

//Creaates a file, this function is automatically called by openfilex
//if the mode is FILE_WRITE and no file exists
vfs_node *createfile(const char *name,DWORD attb)
{
    vfs_node *destdir,*rollback,*node,*retval; //obtain the destination directory
    devmgr_fs_desc *fs;
    int fs_deviceid = 0,result;
    char path[255],dirname[255],fname[255];

    //wait until the vfs is ready
    sync_entercrit(&vfs_busy);

    //determine if the file already exists
    node=vfs_searchname(name);

    //file already exists!!
    if (node!=0) return node;

#ifdef WRITE_DEBUG
    printf("decoding name..\n");
#endif

    //obtain the filename from the path given in name
    strcpy(path,name);
    parsedir(path,dirname,fname);

    //verify that this is a valid filename
    if (!vfs_isvalidname(fname)) return -1;
    
    //obtain the vfs_node of the parent directory
    destdir=vfs_searchname(dirname);

    if (destdir==0) {

        printf("error locating directory!\n");
        sync_leavecrit(&vfs_busy);
        return -1;
    }; //directory or folder not found!

#ifdef WRITE_DEBUG
    printf("Adding the file to the node..\n");
#endif

    //create a new vfs_node
    node = (vfs_node*)malloc(sizeof(vfs_node));

    //Initialize the node structure
    memset(node,0,sizeof(vfs_node));

    //Save the old value of the parent directory's files pointer so
    //that we could rollback in case something unfortunate happens
    rollback = destdir->files;

    //use the FS driver of the parent
    fs_deviceid = destdir->fsid;
    if (fs_deviceid==-1) return -1;

    //attach the node to the parent directory
    vfs_createnode(node,destdir);

    //fill in the required fields in vfs_node
    strcpy(node->name,fname);

#ifdef WRITE_DEBUG
    printf("callling fat12_createfileEX() %s..\n",node->path->name);
#endif

    node->fsid  = fs_deviceid;

    //use the block driver of the parent
    node->memid = destdir->memid;

    //Obtain an interface for the fs driver
    fs=(devmgr_fs_desc*)devmgr_getdevice(fs_deviceid);

    //tell the fs driver to create file on disk
    if (fs->createfile!=0)
    result = bridges_call(fs,&fs->createfile,node,node->memid);

    if (fs->createfile == 0 || result == -1)
    {
        //create file unsuccessful, rollback to previous state
        free(node);
        destdir->files = rollback;
        retval = -1;
    }
        else
    retval = node;

        
    sync_leavecrit(&vfs_busy);
    
#ifdef WRITE_DEBUG
    printf("createfile done.\n");
#endif

    return retval;
    ;
};

//returns the directory where the file resides
void parsedir(char *fullpath,char *loc,char *name)
{
    char path[256];
    char *s;
    int x,i;
    if (fullpath[0]=='/') //path starts from the root
        strcpy(path,fullpath);
    else
    {
        showpath(path);
        strcat(path,fullpath);
    };


    x=strlen(path);

    for (i=x;i>=0;i--)
    {
        if (path[i]=='/')
        {
            path[i]=0;
            strcpy(name,&path[i+1]);
            break;
        };
    };
    strcpy(loc,path);
};

//used by the progressive mount feature of the VFS to mount a
//directory one by one instead of all at once
int vfs_mountdirectory(vfs_node *node)
{
    devmgr_fs_desc *fs;
    //determine the filesystem driver of this file
    fs =(devmgr_fs_desc*)devmgr_getdevice(node->fsid);
    //call the mountdirectory function of the filesystem
    if (fs->mountdirectory!=0)
       {
        //make an intermodule call
        bridges_link(fs,&fs->mountdirectory,node,node->memid,0,0,0,0);    
        return 1;
       } 
    else
        return 0; 
};

/* searches a file for the given name and then returns
   a vfs_node pointer */
   
vfs_node *vfs_searchname(const char *name)
{
    char path[256];
    char *s;
    vfs_node *node_ptr = vfs_root->files;

    if (strcmp(name,"")==0) return vfs_root;
    if (node_ptr == 0) return 0;  //invalid pointer??
    
    if (strcmp(name,"")==0)
        showpath(path);
    else
        if (name[0]=='/') //path starts from the root
            strcpy(path,name);
        else
        {
            char *first_entry,*s;    
            char next_entry[256];
            
            strcpy(path,name);
            first_entry = strtok(path,"/");
            
            if (s=strtok(0,0))
                strcpy(next_entry,s);
            else
                strcpy(next_entry,"");
                       
            
            if (vfs_ispathcut(first_entry))
                {
                     char *str;         
                     str = vfs_getpathcut(first_entry);
                     
                     if (str!=-1)
                     {
                         strcpy(path,str);  
                         strcat(path,next_entry);      
                     };
                }  
              else
            if (strcmp(first_entry,"..")==0)
                {
                                
                     //make sure this is not the root directory
                     if (current_process->workdir->path!=0)
                     {
                         getfullpath(current_process->workdir->path,path);         
                         strcat(path,next_entry);
                     }
                       else
                     return 0; 
                }
              else
                {       
                     showpath(path);
                     strcat(path,name);
                };
        };
    
    //remove trailing slash
    if (path[strlen(path)-1]=='/')
    {
        path[strlen(path)-1]=0;
    };
    
    s=strtok(path,"/");
    
    
    do {
        if (strcmp(s,node_ptr->name)==0)
        {
            s=strtok(0,"/");

            if (s==0) //we have found the file!
                return node_ptr;
                
            //a directory?    
            if ( (node_ptr->attb&FILE_DIRECTORY) 
                 && (strcmp(node_ptr->name,"..")!=0)
                 && (strcmp(node_ptr->name,".")!=0)  )   
               {
                    devmgr_fs_desc *fs;               
                    
                    /*check if directory has been mounted, if not
                      instruct the filesystem driver to mount it.
                      
                      Notes: VFS_NOT_MOUNTED is used so that the VFS does not
                             mount everything in one shot, very useful for large drives*/
                    if (node_ptr->files == VFS_NOT_MOUNTED)
                    {
                      if (vfs_mountdirectory(node_ptr)==0) return 0;
                    };
                    
                    node_ptr = node_ptr->files;
                    continue;
               }; 
        }; 
        
        node_ptr = node_ptr->next;
    } 
    while (node_ptr != 0);
    return 0;
};





vfs_node *getdirectory(const char *name)
{
    vfs_node *dptr=current_process->workdir;
    vfs_node *fileptr=dptr->files;
    if (name==0)
        return homedir;

    if (strcmp(name,"/")==0||strcmp(name,"")==0)
        return vfs_root;

    if (strcmp(name,"..")==0&&dptr->path!=0)
        return dptr->path;

    fileptr=vfs_searchname(name);
    if (fileptr!=0)
        if (fileptr->attb&FILE_DIRECTORY)
        {
            return fileptr;
        }
        else return 0;//the selected file is not a directory
    return 0; //cannot find the directory

};

//change the working directory of the current process
int chdir(const char *name)
{
    vfs_node *dptr=current_process->workdir,*fileptr;

    if (name==0)
    {
        current_process->workdir=homedir;
        return 1;
    };

    if (strcmp(name,"/")==0)
    {
        current_process->workdir = vfs_root;
        return 1;
    };

    if (strcmp(name,"..")==0 && dptr->path!=0)
    {
        current_process->workdir=dptr->path;
        return 1;
    };

    fileptr=vfs_searchname(name);


    if (fileptr!=0)
    {
        if (fileptr->attb&FILE_DIRECTORY)
        {
            current_process->workdir=fileptr;
            return 1;   //success
        }
        else return 0;//the selected file is not a directory
    };   

    return 0; //cannot find the directory
};


int changedirectory(const char *name)
{
    vfs_node *dptr=current_process->workdir;

    if (strcmp(name,"")==0)
    {
        printf("%s\n",dptr->name);
        return 1;
    };

    return chdir(name);
};

void swapchar(char *t1,char *t2)
{
    char temp=*t1;
    *t1=*t2;
    *t2=temp;
};

char *rev_str(char *str)
{
    int i;
    int total=strlen(str);
    for (i=0;i<total/2;i++)
        swapchar(&str[i],&str[total-1-i]);
    return str;
};

//returns the parh string base on a vfs_node structure
char *getpath(vfs_node *ptr,char *s)
{
    strcpy(s,"");
    do
    {
        char temp[255];
        strcpy(temp,ptr->name);
        strcat(temp,"/");
        rev_str(temp);
        strcat(s,temp);
        ptr=ptr->path;
    }
    while (ptr!=0);
    rev_str(s);
    return s;
};

char *getfullpath(vfs_node *node,char *s)
{
    vfs_node *ptr=node;
    strcpy(s,"");
    do {
        char temp[255];
        strcpy(temp,ptr->name);
        if (ptr->attb&FILE_DIRECTORY)
            strcat(temp,"/");
        rev_str(temp);
        strcat(s,temp);
        ptr=ptr->path;
    }
    while (ptr!=0);
    rev_str(s);
    return s;
};

//returns the path string
char *showpath(char *s)
{
    return getfullpath(current_process->workdir,s);
};

void file_showopenfiles()
{
    file_PCB *ptr=file_globalopen;
    char owner_name[255];
    printf("Files opened:\n");
    printf("%-30s%-20s%-5s\n","Name","Owner","Device");
    while (ptr!=0)
    {
        dex32_getname(ptr->processid,sizeof(owner_name),owner_name);
        if (ptr->locked)
                textcolor(RED);
        else
                textcolor(WHITE);
        printf("%-30s%-20s%-5d\n",ptr->ptr->name,owner_name,ptr->ptr->memid);
        ptr=ptr->next;
        textcolor(WHITE);
        ;
    };
};



