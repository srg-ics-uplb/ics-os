/*
  Name: Device Filesystem (For UNIX compatibility purposes)
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 19/01/00 05:37
  Description: This module mounts the block devices in the device manager to a directory
  on the VFS.
  
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

#include "devfs.h"
#include "../iomgr/iosched.h"
#include "../devmgr/dex32_devmgr.h"


int devfs_myid = 0;

DWORD devfs_getbytesperblock(int id)
{
    return 1;
};

/*The VFS Checks for the rewrite file function, so we must make sure this is present*/
void devfs_rewritefile(vfs_node *f,int id)
{
    /*do nothing*/
};

/*Used for writing to block devices like a file*/
int devfs_writefile(vfs_node *f, char *buf, int start, int end, int device_id)
{
    int blockdevid = devmgr_finddevice(f->name);
    if (blockdevid != -1)
    {
         DWORD bytes_per_sector, block_requests, block, total_blocks;
         DWORD startblock, endblock, adj, startadj, startlength;
         DWORD *handles, *buffers, ofs, handle;
         DWORD i, req_ind, index;
         void *temp_buffer;
         devmgr_block_desc *myblock = (devmgr_block_desc *)devmgr_getdevice(blockdevid);
         

         
         /*Compute for block size of the block device */
         if (myblock->get_block_size)
                  bytes_per_sector = bridges_call(myblock, &myblock->get_block_size);
         else
                  return -1;

                  
                           
         /*Since this is a block device, we have to 
           compute everything in terms of blocks*/
         startblock  = start / bytes_per_sector;
         endblock    = end / bytes_per_sector;
         adj         = (end % bytes_per_sector) + 1;
         startadj    = start % bytes_per_sector;
         startlength = bytes_per_sector - startadj;


         /*validate the end block given*/
         if (myblock->total_blocks) 
         {
                  total_blocks = bridges_call(myblock, &myblock->total_blocks);
                  if ( endblock >= total_blocks) return -1;
         }
         else
                  return -1;
         
           /*Compute for the total amount of block requests to be sent to the
           IO manager*/
         block_requests = endblock-startblock + 1;
         
         handles = (DWORD*)malloc( block_requests * sizeof(DWORD));
         buffers = (DWORD*)malloc( block_requests * sizeof(DWORD));
         
         block = 0; req_ind = 0;
         
         ofs = 0;
         
         //allocate temporary buffer, where we will place our data
         temp_buffer = (void*)malloc(bytes_per_sector);
         
         do {
           
            //Check if this block is in the range of the raquested clusters
            if (block >=startblock && block<=endblock)
            {

               /*when writing a block, there are 4 cases, 3 of those cases need to read 
                 from the disk.*/
               if (block == startblock || block == endblock)
               {
                  /*In performing a write operation, we have to read the current
                    data on the sector, copy data from the buffer and then
                    write it back to the sector since disks could only write on
                    a sector by sector basis*/
                  DWORD hdl=dex32_requestIO(device_id,IO_READ, block, 1, temp_buffer);
                  while (!dex32_IOcomplete(hdl));
                  dex32_closeIO(hdl);
               
                   //The data is located in only one block
                   if (block == startblock && block == endblock)
                   {
                      memcpy( (void*)temp_buffer + startadj, buf, end-start + 1);
                      ofs+= end-start +1;
                   }
                   else
                   //This part of the data is located at the start
                   if (block == startblock)
                   {
                      memcpy((void*) temp_buffer + startadj, buf, startlength);
                      ofs+=startlength;
                   }
                   else
                   //This part of the data is located at the end
                   if (block == endblock)
                   {
                      memcpy((void*) temp_buffer, buf+ ofs, adj);
                      ofs+=adj;
                   };
               }
               else
               //This part of the data encompasses a whole cluster
               {
                  memcpy((void*) temp_buffer, buf + ofs, bytes_per_sector);
                  ofs+= bytes_per_sector;
               };
               
               
               //queue the request to the IO manager for the actual write
               handle = dex32_requestIO(device_id,IO_WRITE, block , 1, temp_buffer);
               
               //since this is a write, which cannot be reordered, we must wait for it to finish.
               while (!dex32_IOcomplete(handle)) ;
               dex32_closeIO(handle);
               
               //if this is the last block of requested writes we're done                              
               if ( block == endblock ) break;
            };
            
            block ++;
         }
         //check if this is the last block of the file, if so we stop.
         while (block < endblock);
         free(temp_buffer);
     };
};

/*Used for reading block devices like a file*/
int devfs_openfile(vfs_node *f, char *buf, DWORD start, DWORD end, int device_id)
{
    int blockdevid = devmgr_finddevice(f->name);
    if (blockdevid != -1)
    {
         DWORD bytes_per_sector, block_requests, block, total_blocks;
         DWORD startblock, endblock, adj, startadj, startlength;
         DWORD *handles, *buffers, ofs;
         DWORD i, req_ind, index;
         devmgr_block_desc *myblock = (devmgr_block_desc *)devmgr_getdevice(blockdevid);
         

         
         /*Compute for block size of the block device */
         if (myblock->get_block_size)
                  bytes_per_sector = bridges_call(myblock, &myblock->get_block_size);
         else
                  return -1;

                  
                           
         /*Since this is a block device, we have to 
           compute everything in terms of blocks*/
         startblock  = start / bytes_per_sector;
         endblock    = end / bytes_per_sector;
         adj         = (end % bytes_per_sector) + 1;
         startadj    = start % bytes_per_sector;
         startlength = bytes_per_sector - startadj;


         /*validate the end block given*/
         if (myblock->total_blocks) 
         {
                  total_blocks = bridges_call(myblock, &myblock->total_blocks);
                  if ( endblock >= total_blocks) return -1;
         }
         else
                  return -1;

         
         /*Compute for the total amount of block requests to be sent to the
           IO manager*/
         block_requests = endblock-startblock + 1;
         
         handles = (DWORD*)malloc( block_requests * sizeof(DWORD));
         buffers = (DWORD*)malloc( block_requests * sizeof(DWORD));
         
         block = 0; req_ind = 0;
         
         do 
         {
                //determine the starting sector this cluster resides in
                
                if (  block >= startblock && block <= endblock)
                {
                   void *databuf=(void*)malloc(bytes_per_sector);
    
                   /*Send read request to the IO manager*/
                   handles[req_ind] = dex32_requestIO( blockdevid, IO_READ, block, 1, databuf);
                   buffers[req_ind]= (DWORD)databuf;
                   req_ind++;
                   
                   if ( req_ind >= block_requests) break;
                   if ( block == endblock ) break;
                   
                };
                
                //compute the next cluster
                block++;
         }
         while (block < endblock);

         //wait until the IO manager completes the read requests
         for (index = 0; index < req_ind; index++)
         {
             while (!dex32_IOcomplete(handles[index]));     
         };
         
         /*copy data read into the buffer while taking into consideration
           block boundaries and buffer boundaries*/
         ofs = 0;      
         
         for (i = 0; i < req_ind ;i++)
         {
            if (i == startblock && i == endblock)
            {
               memcpy((void*)buf, buffers[i]+startadj, end-start + 1);
               ofs += (end-start);
               break;
            }
            else
            if (i==startblock)
            {
               memcpy((void*)buf, buffers[i]+startadj, startlength);
               ofs += startlength;
            }
            else
            if (i==endblock)
            {
               memcpy((void*)(buf+ofs), buffers[i], adj);
               ofs += adj;
               break;
            }
            else
            {
               memcpy((void*)(buf+ofs), buffers[i], bytes_per_sector);
               ofs += bytes_per_sector;
            };
            
         };
         
         /*free memory used*/
         for (i=0;i<req_ind;i++) 
         {
             free(buffers[i]);      
             dex32_closeIO(handles[i]);
         };
         

         free(handles);
         free(buffers);
         
    };
    return 1;
};


/*This function is called by the VFS to mount the root volume, since this
does not actually link to a block device, ID is ignored*/
int devfs_mountroot(vfs_node *mountpoint,int device_id)
{
    int i;
    
    /*Let's take care of the root node first*/
    mountpoint->fsid = devfs_myid;
    mountpoint->memid = device_id;
    
    //wait until the device manager is ready
    sync_entercrit(&devmgr_busy);

    for (i=0;i<MAXDEVICES;i++)
      {
            if (devmgr_devlist[i]!=0)
            {
            char typestr[10];
            vfs_node *node;
            
            //allocate a new vfs_node
            node=(vfs_node*)malloc(sizeof(vfs_node));
            memset(node,0,sizeof(vfs_node));
            vfs_createnode(node,mountpoint);
            
            node->memid     = device_id;
            node->fsid      = devfs_myid;
            node->attb      = FILE_OREAD | FILE_OWRITE;
            node->misc_flag = devmgr_devlist[i]->id;
            
            if (devmgr_devlist[i]->type == DEVMGR_BLOCK)
            {
                devmgr_block_desc *blockdev = (devmgr_block_desc*)devmgr_devlist[i];
                if (blockdev->total_blocks!=0 &&
                    blockdev->get_block_size!=0)
                    { 
                      node->size = bridges_call(blockdev,&blockdev->total_blocks) *
                                   bridges_call(blockdev,&blockdev->get_block_size);
                    };
            }
              else 
            node->size = 0;
            
            strcpy(node->name,devmgr_devlist[i]->name);
            
            };
      };
   sync_leavecrit(&devmgr_busy);
   return 1;

};

/*Cretes a null block device*/
void devfs_initnull()
{
      devmgr_block_desc myblock;
      memset(&myblock,0,sizeof(devmgr_block_desc));
      strcpy(myblock.hdr.name,"null");
      strcpy(myblock.hdr.description,"null block device");
      myblock.hdr.size = sizeof(devmgr_block_desc);
      myblock.hdr.type = DEVMGR_BLOCK;
      devmgr_register((devmgr_generic*)&myblock);

};

void devfs_init()
   {
      devmgr_fs_desc myfs;
      memset(&myfs,0,sizeof(myfs));
      strcpy(myfs.hdr.name,"devfs");
      strcpy(myfs.hdr.description,"Device Filesystem");
      myfs.hdr.size = sizeof(myfs);
      myfs.hdr.type = DEVMGR_FS;
      myfs.mountroot = devfs_mountroot;
      myfs.rewritefile = devfs_rewritefile;
      myfs.writefile = devfs_writefile;
      myfs.readfile = devfs_openfile;
      myfs.getbytesperblock = devfs_getbytesperblock;
      myfs.mountdirectory = 0;
      myfs.readfile = 0;
      myfs.getbytesperblock = 0;
      myfs.unmount = 0;

      devfs_myid = devmgr_register((devmgr_generic*)&myfs);
   };
