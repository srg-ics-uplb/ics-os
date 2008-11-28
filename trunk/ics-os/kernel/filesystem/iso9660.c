/*
  Name: iso9660.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 07/02/04 07:36
  Description: The module that handles the ISO9660 filesystem on most CD-ROMS
  
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
   #include "../dextypes.h"
   #include "../vfs/vfs_core.h"
   #include "../devmgr/dex32_devmgr.h"
   #include "../iomgr/iosched.h"
   #include "iso9660.h"
   
   
   int iso9660_myid = 0;
   
   int iso9660_loaddirectory(iso9660_directory *dirinfo, void **buffer,int id)
   {
        int sectors;
        int handle;
        *buffer = malloc(dirinfo->length_le);
        
        //obtain the number of sectors
        if  ((dirinfo->length_le % 2048) != 0)
                sectors = (dirinfo->length_le / 2048) + 1;
          else
                sectors = (dirinfo->length_le / 2048);
        
        handle=dex32_requestIO(id,IO_READ,dirinfo->first_sector_le ,sectors,*buffer);
        while (!dex32_IOcomplete(handle));
        dex32_closeIO(handle);    
        
        return dirinfo->length_le;
   };
   
   /*This function is called when the VFS first mounts a drive to the filesystem*/
   int iso9660_mountroot(vfs_node *node,int id)
   {
      DWORD handle;
      iso9660_volumedescriptor vol_ptr;
      iso9660_volumedescriptor current_vd; //stores the volume descriptor that will be used
      int vol_num = 1;
      //attempt to read the volume descriptors and make an I/O request to the IO manager
      
      printf("reading primary volume descriptor..\n");
      handle=dex32_requestIO(id,IO_READ,16,1,&vol_ptr);
      while (!dex32_IOcomplete(handle));
      dex32_closeIO(handle);       
      memcpy(&current_vd , &vol_ptr, sizeof(iso9660_volumedescriptor));
      
      //Check for a joliet volume descriptor
      
      do {
      printf("reading secondary volume descriptor..\n");
      
      handle=dex32_requestIO(id,IO_READ,16+vol_num,1,&vol_ptr);
      while (!dex32_IOcomplete(handle));
      dex32_closeIO(handle);      
      
      if ( iso9660_isjoliet(&vol_ptr) )
        {
           printf("Joliet Extension SVD detected\n");     
           memcpy(&current_vd,&vol_ptr,sizeof(iso9660_volumedescriptor));
           break;     
        };
      if (vol_ptr.ident == 3)
        {
           printf("Volume Partition Descriptor detected\n");      
        };    
      vol_num++;
      } while (vol_ptr.ident<255&& (vol_num<8) );
      
      printf("Interpreting volume descriptor..\n");
      //validate data
      if ( current_vd.descriptors[0] == 67 &&
           current_vd.descriptors[1] == 68 &&
           current_vd.descriptors[2] == 48 &&
           current_vd.descriptors[3] == 48 &&
           current_vd.descriptors[4] == 49 &&
           current_vd.descriptors[5] == 1)
           {
                int i,size, type;
                void *buffer;
                printf("valid primary volume descriptor set detected.\n");
                printf("Bytes per lgical block %d\n", current_vd.sector_size_le);
                
                type  = iso9660_isjoliet(&current_vd);
                
                if (type)
                   printf("Joliet Extensions detected. Using UCS-2 Level %d character set.\n",type);
  
                //print the volume identifier
                for (i=0;i<32;i++)
                    printf("%c",current_vd.volume_ident[i]);
                printf("\n");     
                
                printf("mounting root directory\n");
                size = iso9660_loaddirectory( (iso9660_directory*) &current_vd.rootdirrec,
                                        &buffer, id);
                node->misc = 0;
                node->miscsize = 0;                                                                 
                node->misc2=(void*)buffer;
                node->miscsize2=size;
                node->fsid = iso9660_myid;
                node->memid = id;
                node->misc_flag = type;
                
                /*If the CD driver supports drive locking, the "$cd -lock command"
                  should lock the CD to prevent it from being removed*/
                devmgr_sendmessage(id, DEVMGR_MESSAGESTR, "$cd -lock");
                
                iso9660_mount(node,buffer,id);

           }
      else
           {
                printf("invalid primary volume descriptor!.\n");      
                return -1; //return with error, tell the VFS not to mount
           };
           
      return 1;
   };  
   

   char *iso9660_convertname(const char *identifier, char *targ)
   {
      int i;
      for (i=0;identifier[i]!=';';i++)
         {
           targ[i] = identifier[i];
         };
         targ[i] = 0;
         return targ;
   };
   
   /*unicode to ascii converter*/
   char *iso9660_iso_unicodetoascii(WORD *unicodestr,char *targ,int length)
   {
   int i;
   for (i=0;unicodestr[i]&&i<(length/2);i++)
   {
      char ascii = (char)( (unicodestr[i]&0xFF00) >> 8);
      if (ascii == ';') break;
      targ[i]=ascii;
   };
   targ[i]=0;
   return targ;
   };

/*Gets bytes per block assuming mode 1*/   
int iso9660_getbytesperblock()
{
    return 2048;
};

int iso9660_openfile(vfs_node *f,char *buffer,int start,int end,int id)
{
    DWORD startblock, endblock, totalblocks;
    DWORD startadj, endadj;
    iso9660_directory *dir = f->misc;
    DWORD firstblock = dir->first_sector_le;
    DWORD handle;    
    char *data_buffer,*bufptr;    
    int i;
    //compute for the starting block
    startblock  = start / 2048;
    endblock    = end / 2048;
    startadj    = start%2048;
    totalblocks = endblock - startblock + 1;
    
    data_buffer = malloc( 2048 * totalblocks );
    
   
    handle = dex32_requestIO(id,IO_READ, firstblock + startblock, totalblocks, data_buffer);
    while (!dex32_IOcomplete(handle));
    dex32_closeIO(handle);       

    memcpy(buffer, data_buffer + startadj, end - start +1);
    free(data_buffer);
    
    return 1;
};

int iso9660_unmount(vfs_node *directory,int id)
{
    devmgr_sendmessage(directory->memid, DEVMGR_MESSAGESTR, "$cd -unlock");
};

int iso9660_mountdirectory(vfs_node *directory, int id)
{
     int dirsize;
     char *buffer;
     //load the directory information
     dirsize=iso9660_loaddirectory(( iso9660_directory*)directory->misc,&buffer,id);
     
     directory->misc2 = (void*)buffer;
     directory->miscsize2 = dirsize;
     directory->attb = FILE_DIRECTORY | FILE_OREAD;
     iso9660_mount(directory,buffer,id);
     
     return 1;
};


   /*determines if this CD-ROM uses joliet extensions which requires UNICODE
     translation*/
   int iso9660_isjoliet(iso9660_volumedescriptor *vol)
   {
      if (vol->escape_sequence[0] == 0x25 && 
          vol->escape_sequence[1] == 0x2F)
        {   
         if (vol->escape_sequence[2] == 0x40)
            //UCS-2 Level 1
            return 1;
         if (vol->escape_sequence[2] == 0x43)
            //UCS-2 Level 2
            return 2;
         if (vol->escape_sequence[2] == 0x45)
            //UCS-2 level 3
            return 3;
         };
         return 0;
   };

   void iso9660_mount(vfs_node *mountpoint,char *dirbuffer, int devid)
   {
      char *dirptr = dirbuffer;
      DWORD size = 0;
      
      printf("mounting.. directory size %d..\n", mountpoint->miscsize2);
      
      while (size < mountpoint->miscsize2)
      {
            int i;
            iso9660_directory *dir =(iso9660_directory*)dirptr;
            vfs_node *node;
            
            if (dir->size == 0 ) break;
            
            //allocate a new vfs_node
            node=(vfs_node*)malloc(sizeof(vfs_node));
            memset(node,0,sizeof(vfs_node));
            
            vfs_createnode(node,mountpoint);
            
            if (dir->ident_length == 1 && dir->ident[0] == 0)
            strcpy(node->name,".");
              else
            if (dir->ident_length == 1 && dir->ident[0] == 1)  
            strcpy(node->name,"..");
              else
            if (mountpoint->misc_flag)  //Joliet Extensions?? 
            iso9660_iso_unicodetoascii(dir->ident,node->name,dir->ident_length); 
              else
            iso9660_convertname(dir->ident,node->name);
            
            node->memid = devid;
            node->fsid = iso9660_myid;
            node->misc = dirptr;
            node->miscsize = 0; 
            node->miscsize2 = 0;
            node->attb = FILE_OREAD;
            node->size = dir->length_le;
            node->misc_flag = mountpoint->misc_flag;
            
            if (dir->flags & ISO9660_ATTBDIRECTORY)
               {
                node->attb = FILE_DIRECTORY | FILE_OREAD;
                node->files = VFS_NOT_MOUNTED;
               };
            
            size+= dir->size;
            dirptr =(void*)( (DWORD) dirptr + dir->size);
            
      };
      
   };
   
   void iso9660_init()
   {
      devmgr_fs_desc myfs;
      memset(&myfs,0,sizeof(myfs));
      strcpy(myfs.hdr.name,"cdfs");
      strcpy(myfs.hdr.description,"DEX ISO 9660/Joliet CD-ROM filesystem");
      myfs.hdr.size = sizeof(myfs);
      myfs.hdr.type = DEVMGR_FS;
      myfs.mountroot = iso9660_mountroot;
      myfs.mountdirectory = iso9660_mountdirectory;
      myfs.readfile = iso9660_openfile;
      myfs.getbytesperblock = iso9660_getbytesperblock;
      myfs.unmount = iso9660_unmount;
      iso9660_myid = devmgr_register((devmgr_generic*)&myfs);
   };
