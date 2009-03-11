
/*
  Name: FAT filesystem driver
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:30
  Description: This is the implementation of the FAT filesystem based on the
  "Microsoft Extensible Firmware Initiative FAT32 File System Specification"
  version 1.03 released by Microsoft Corporation on December 6,2000
  
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
#include "fat12.h"
#include "../devmgr/dex32_devmgr.h"
#include "../iomgr/iosched.h"
#include "../vfs/vfs_core.h"

//#define DEBUG_FAT12

int fat_deviceid;

int obtain_next_cluster(int cluster,void *fat,int fat_type,BPB *bpbblock,int id)
{
   WORD *temp;
   BYTE  *fat12;
   WORD  *fat16;
   DWORD *fat32;
   int x,index;

   if (fat == 0 || fat_type == FAT12_FAT32)
   {
      DWORD start_of_fat = bpbblock->num_boot_sectors;   
      DWORD fat_sector = (cluster >> 7) + start_of_fat;
      DWORD handle;
      fat32 = (DWORD*)malloc(512);
      
      #ifdef DEBUG_FAT12
      printf("----reading FAT32 at %d:%d..",start_of_fat,cluster >> 7);
      #endif
      
      handle=dex32_requestIO(id,IO_READ,fat_sector,1,fat32);
      while (!dex32_IOcomplete(handle));
      dex32_closeIO(handle);        
         
      cluster = fat32[cluster & 0x7f] & 0x0FFFFFFF;
      
      free(fat32);
      
      #ifdef DEBUG_FAT12
      printf("done %d.\n",cluster);
      #endif
      
      return cluster ;
   };
   
   //determine the correct type of FAT
   if (fat_type == FAT12_FAT12) fat12 =(BYTE*)fat;
       else
   if (fat_type == FAT12_FAT16) fat16 =(WORD*)fat;
       else
   if (fat_type == FAT12_FAT32) fat32 =(DWORD*)fat;
   
   if (fat_type == FAT12_FAT12)
   {
      
      x=(cluster*3)/2; //index=cluster*3/2
      temp=(WORD*)(fat12+x);
      index=*temp;
   
      if (cluster & 1) //cluster is odd
            return index >> 4;// take higher 12 bits
      else
            return index & 0xFFF;//  take lower 12 bits
   }
      else
   if (fat_type == FAT12_FAT16)
         return fat16[cluster];
      else
   if (fat_type == FAT12_FAT32)
         return fat32[cluster] & 0x0FFFFFFF;         
         
   return 0;
;};


int fat_write_cluster(int cluster,int value,void *fat,int fat_type,BPB *bpbblock,int id)
{
   WORD *temp;
   BYTE  *fat12;
   WORD  *fat16;
   DWORD *fat32;
   int x,index;

   if (fat == 0 || fat_type == FAT12_FAT32)
   {
      DWORD start_of_fat = bpbblock->num_boot_sectors;   
      DWORD fat_sector = (cluster >> 7) + start_of_fat;
      DWORD handle;
      fat32 = (DWORD*)malloc(512);
      
      #ifdef DEBUG_FAT12
      printf("----reading FAT32 at %d:%d..",start_of_fat,cluster >> 7);
      #endif
      
      handle=dex32_requestIO(id,IO_READ,fat_sector,1,fat32);
      while (!dex32_IOcomplete(handle));
      dex32_closeIO(handle);        
         
      fat32[cluster & 0x7f] = value & 0x0FFFFFFF;   

      handle=dex32_requestIO(id,IO_WRITE,fat_sector,1,fat32);
      while (!dex32_IOcomplete(handle));
      dex32_closeIO(handle);        
      
      free(fat32);
      
      #ifdef DEBUG_FAT12
      printf("done %d.\n",cluster);
      #endif
      
      return cluster ;
   };
   
   //determine the correct type of FAT
   if (fat_type == FAT12_FAT12) fat12 =(BYTE*)fat;
       else
   if (fat_type == FAT12_FAT16) fat16 =(WORD*)fat;
       else
   if (fat_type == FAT12_FAT32) fat32 =(DWORD*)fat;
   
   if (fat_type == FAT12_FAT12)
   {
      
      x=(cluster*3)/2; //index=cluster*3/2
      temp=(WORD*)(fat12+x);
      index=*temp;

      if (cluster & 1) //cluster is odd
      {
         value = value << 4;
         *temp= (*temp&0x000F) | value;
      }
        else
     {
      value =value & 0x0FFF;
      *temp=(*temp&0xF000) | value;
     };
   }
      else
   if (fat_type == FAT12_FAT16)
        fat16[cluster] = value;
   return 0;
;};


int clustertoblock(BPB *bpbblock,int cluster)
{
   int start_sector = bpbblock->num_boot_sectors;
   BPB32 *bpb32= bpbblock;
   
   if (bpbblock->sectors_per_fat == 0)
   start_sector+=bpbblock->num_fats * bpb32->fatsz32;
         else
   start_sector+=bpbblock->num_fats * bpbblock->sectors_per_fat;
   //computes sectors used by a FAT12/16 directory                   
   int root_sectors =((bpbblock->num_root_dir_ents * 32) + (bpbblock->bytes_per_sector - 1)) 
      / bpbblock->bytes_per_sector;

   if (bpbblock->num_root_dir_ents!=0) 
   return start_sector + ((cluster - 2)*(bpbblock->sectors_per_cluster) ) +
          root_sectors;
       else
   return (start_sector + (cluster - 2)*(bpbblock->sectors_per_cluster) );
};

DWORD fat_sectors_per_fat(BPB *bpbblock)
{
 BPB32 *bpb32= bpbblock;
    if (bpbblock->sectors_per_fat == 0)
    return bpb32->fatsz32;
         else
    return bpbblock->sectors_per_fat;
};

void readBPB(BPB *bpbblock,int id)
{
   startints();
   DWORD handle=dex32_requestIO(id,IO_READ,0,1,bpbblock);
   while (!dex32_IOcomplete(handle));
   dex32_closeIO(handle);
};

void interpretBPB(BPB *bpbblock)
{
   printf("bytes per sector:%d\n sectors per cluster:%d\n bootsectors:%d\n",
      (WORD)bpbblock->bytes_per_sector,
   (BYTE)bpbblock->sectors_per_cluster,(WORD)bpbblock->num_boot_sectors);
   printf("number of root entries :%d\n",(WORD)bpbblock->num_root_dir_ents);
   printf("size of BPB:%d",sizeof(BPB));
   
};

void fat_diskdiagnostics(int id)
{
       DWORD *buf = malloc(512);
       char c=' ';
       int blocknum = 0 , i;
       do
       {
            printf("\n=====Block number %d======\n", blocknum);
             
            DWORD handle=dex32_requestIO(id,IO_READ,blocknum,1,buf);
            while (!dex32_IOcomplete(handle));
            dex32_closeIO(handle);
            
            for (i=0;i < 20 ; i++)
                printf("[%d]\n", buf[i]&0x0FFFFFFF);
            blocknum++;    
            c=getch();                        
       } while (c!='q');
       free(buf);
};

void loadfat(BPB *bpbblock,void *fat,int id)
      {
         DWORD i;
         DWORD start_of_fat = bpbblock->num_boot_sectors;
         
         #ifdef DEBUG_FAT12
         printf("loading fat size %d..",fat_sectors_per_fat(bpbblock));
         #endif
         
         DWORD handle=dex32_requestIO(id,IO_READ,start_of_fat,fat_sectors_per_fat(bpbblock),fat);
         while (!dex32_IOcomplete(handle));
         dex32_closeIO(handle);     
         
         #ifdef DEBUG_FAT12
         printf("done.\n");
         #endif
         
      };


int loaddirectory(fatdirentry *dir, char **buf,int id)
{
  DWORD start_cluster = (dir->st_clust_msw << 16) | dir->st_clust;
  return fat_loaddirectoryfromcluster(start_cluster,buf,id);
};

void debug_printbuf(char *buf)
{
    int i;
    for (i=0;i<512;i++)
      {
           printf("%c",buf[i]); 
      };
};

DWORD fat_readcluster(int cluster,BPB *bpbblock, char *buf,int id)
{
    DWORD handle;
    DWORD start_sector = clustertoblock(bpbblock,cluster);
    int i,i2,ind = 0,v=0;
    handle=dex32_requestIO(id,IO_READ,start_sector,bpbblock->sectors_per_cluster,buf);
    while (!dex32_IOcomplete(handle));
    dex32_closeIO(handle);     
};

DWORD fat_writecluster(int cluster,BPB *bpbblock, char *buf,int id)
{
    DWORD handle;
    DWORD start_sector = clustertoblock(bpbblock,cluster);
    int i,i2,ind = 0,v=0;
    handle=dex32_requestIO(id,IO_WRITE,start_sector,bpbblock->sectors_per_cluster,buf);
    while (!dex32_IOcomplete(handle)) ;
    dex32_closeIO(handle);     
};

DWORD fat_writeblock(int block,BPB *bpbblock, char *buf,int id)
{
    DWORD handle;
    DWORD start_sector = block;
    int i,i2,ind = 0,v=0;
    handle=dex32_requestIO(id,IO_WRITE,start_sector,bpbblock->sectors_per_cluster,buf);
    while (!dex32_IOcomplete(handle)) ;
    dex32_closeIO(handle);     
};

int fat_loaddirectoryfromcluster(DWORD start_clust,char **buf,int id)
      {
         unsigned int cluster=start_clust;
         unsigned int i,x,b,v=0,c=0; //obtain starting cluster
         WORD index,*temp;
         BYTE *fat=0;
         int ind=0;
         int fat_type;
         DWORD *fat32;
         char *dirbuf;
         
         BPB bpbblock;
         
         #ifdef DEBUG_FAT12
         printf("loaddirectoryfromcluster called..");
         #endif
         
         readBPB(&bpbblock,id);
         
         fat_type = fat_get_fat_type(id,&bpbblock);
         
         #ifdef DEBUG_FAT12
         printf("loaddirectoryfromcluster: loading FAT\n");
         #endif
         
         if (fat_type!=FAT12_FAT32)
         {
         fat=(BYTE*)malloc(fat_sectors_per_fat(&bpbblock)*512);//allocate memory for FAT
         loadfat(&bpbblock,fat,id);
         };
         
         if (cluster==0) return 0;
         
         //figure out the size of the directory by going through the FAT cluster
         //chain and performing some math which is:
         //directory_size = total directory sectors x 512
         #ifdef DEBUG_FAT12
         printf("loaddirectoryfromcluster: computing directory size\n");
         #endif
         
         int directory_size = get_sector_fromcluster(cluster,&bpbblock,0,fat,id)*512;
         
         #ifdef DEBUG_FAT12
         printf("now reading directory with size %d ..", directory_size);
         #endif
         
         dirbuf = (char*)malloc(directory_size);
         
         do {
            char temp[255];      
            #ifdef DEBUG_FAT12
            printf("reading cluster %d ..", cluster);
            #endif
            
            fat_readcluster(cluster,&bpbblock,dirbuf+v,id);

            //compute the next cluster
            cluster = obtain_next_cluster(cluster,fat,fat_type,&bpbblock,id); 
            
            v+=bpbblock.sectors_per_cluster*512;
            
            #ifdef DEBUG_FAT12            
            printf("done next is %d.\n",cluster);
            #endif
            
            //increment ind which keeps track of the size of this
            //directory
            ind++;
            }
         while (cluster < fat_get_eoc(fat_type) && ind<1000);
         
         #ifdef DEBUG_FAT12
         printf("ldfc done.\n");
         #endif
                  
         *buf = dirbuf;
         
         if (fat!=0) free(fat);
         
         #ifdef DEBUG_FAT12
         printf("done.\n");
         #endif
         
         return ind*512*bpbblock.sectors_per_cluster; //success
      ;};


      
int fat_validatefilename(const char *filename)
      {
         int i;
         for (i=0;filename[i]&&i<12;i++)
         {
            if (filename[i]=='!' ||
               filename[i]=='*' ||
               filename[i]=='?' ||
               filename[i]=='\\' ||
               filename[i]=='/' ||
               filename[i]=='|' ||
               filename[i]=='>' ||
            filename[i]=='<' )
            return -1;
         };
         return 1;
      };


int loadroot(fatdirentry **buf,const BPB *bpbblock,int id)
{
   fatdirentry *dir; 
   //compute for the block location of the BPB
   int blockloc=bpbblock->sectors_per_fat*bpbblock->num_fats+
   bpbblock->num_boot_sectors;
   
   int sector=sizeof(fatdirentry)*bpbblock->num_root_dir_ents/
   bpbblock->bytes_per_sector;
   int i;
   DWORD handle;
   
   //detect if it is FAT32, if it is we use the rootcluster field of the BPB
   if (fat_get_fat_type(id,bpbblock) == FAT12_FAT32)
      {
            BPB32 *bpb32 = bpbblock;
            int size;
            size = fat_loaddirectoryfromcluster(bpb32->rootcluster,buf,id);
            
            #ifdef DEBUG_FAT12
            printf("FAT32 root directory read. size %d\n",size);
            #endif
            
            return size;
      };

   dir = (fatdirentry*)malloc(sector*512);
   *buf = dir;
   //Load Them: We first queue the read request to the block device to the IO manager
   handle=dex32_requestIO(id,IO_READ,blockloc,sector,(void*)dir);
   
   //Wait until the IO manager has finished the request and then close the request
   //handles
   while (!dex32_IOcomplete(handle));
   
   dex32_closeIO(handle);

   return sector*512;
};



void strtofile12(const char *str,char *s)
{
   int makeextension=0;
   int i,i2,i3;
   
   for (i=0;i<8&&str[i];i++)
   {
      if (str[i]=='.') {makeextension=1;i2=i+1;break;};
      s[i]=toupper(str[i]);
   };
   
   for (;i<8;i++) s[i]=' ';
   
   i=0;
   
   if  (!makeextension)
   for (i3=0;str[i3];i3++)
      if (str[i3]=='.') {makeextension=1;i2=i3+1;break;};
   
   if (makeextension)
   for (i=0; i<3 && str[i2] ; i++)
      s[8+i]=toupper(str[i2++]);
   
   for (;i<3;i++) s[8+i]=' ';
;};



void file12tostr(fatdirentry *dir,char *str)
{
   int i=0,i2=0;
   for (i=0;i<8;i++)
   {
      if (dir->name[i]==' ') break;
      str[i2]=tolower(dir->name[i]);i2++;
   };
   
   if (dir->ext[0]==' '&&dir->ext[1]==' '&&dir->ext[2]==' ')
      {str[i2]=0;return;};
   if (!(dir->attrib&FDIRECTORY))
      str[i2]='.';i2++;
   for (i=0;i<3;i++)
   {
      if (dir->ext[i]==' ') break;
      str[i2]=tolower(dir->ext[i]);i2++;
   };
   str[i2]=0;
   str[13]=0;
};

DWORD fat_writefileEX(vfs_node *f,char *bufr,int start,int end,int id)
{
   BPB    *buf=(BPB*)malloc(512);
   //perform the read
   
   fatdirentry   *buf2=0;
   int found=0,size=0,i;
   readBPB(buf,id);
   
#ifdef WRITE_DEBUG
   printf("fat_writefileEX() called..\n");
#endif
   
   if (f!=0)
      buf2=(fatdirentry*)f->misc;
   if (buf2&&bufr)
   {
      
#ifdef WRITE_DEBUG
      printf("found file loading: %d..\n",buf2->file_size);
#endif
      
      if (!writefile12EX2(buf2,buf,bufr,1,start,end,id))
      {
         //error while loading the file
         
         free(buf);
         return 0;
      };
      
   };
   if (buf2)
      size=buf2->file_size;
   //    else
   //  return 0;
   free(buf);
   return size;
};

DWORD fat_createfileEX(vfs_node *f,int id)
{
   BPB    *buf;
   int ret;
   
#ifdef WRITE_DEBUG
   printf("fat_createfileex called [%s].\n",f->path->name);
#endif
   
   buf=(BPB*)malloc(512);
   readBPB(buf,id);
   
#ifdef WRITE_DEBUG
   printf("fat_createfileex called [%s].\n",f->path->name);
#endif
   
   ret=fat_createfile(f,buf,id);
   
#ifdef WRITE_DEBUG
   printf("fat_createifleex done.\n");
#endif
   
   free(buf);
   return ret;
};

DWORD fat_addsectorsEX(vfs_node *f,DWORD sectors /*sectors to add*/,int id)
{
   BPB    *buf=(BPB*)malloc(512);
#ifdef WRITE_DEBUG
   printf("fat_addsectorsEX called..\n");
#endif
   readBPB(buf,id);
   return fat_addsectors(f,buf,sectors,id);
   free(buf);
};


int fat_getfreeblocks(int id)
{
   BPB  *bpbblock=(BPB*)malloc(512);
   BYTE *fat;
   int i;
   readBPB(bpbblock,id);
   DWORD total=0;
   fat=(BYTE*)malloc(bpbblock->sectors_per_fat*512);//allocate memory for FAT
   loadfat(bpbblock,fat,id);
   
   for (i=0;i<bpbblock->total_sectors-3;i++)
      if (obtaincluster(i,fat)==0) total++;
   
   free(bpbblock);
   free(fat);
   
   return total;
};

DWORD fat_getbytesperblock(int id)
{
    BPB bpbblock;
    readBPB(&bpbblock,id);
    return bpbblock.sectors_per_cluster*512;
;};

void fat_getfileblocks(file_PCB *f,DWORD *sectinfo,int id)
{
   BPB  *buf=(BPB*)malloc(512);
   fatdirentry *buf2=0;
   vfs_node *fp=f->ptr;
   // printf("fat_getfileblocks() called...\n");
   readBPB(buf,id);
   // printf("obtaining directory information\n");
   if (f!=0)
      buf2=(fatdirentry*)fp->misc;
   
   if (buf2)
      fillsectorinfo(buf2,buf,sectinfo,id);
   free(buf);
   
;};

//returns the number of sectors occupied by a directory
DWORD getdirsectorsize(fatdirentry *dir,BPB *bpbblock,int func,BYTE *fat,int id)
{
    return get_sector_fromcluster(dir->st_clust,bpbblock,func,fat,id);
};

DWORD get_sector_fromcluster(DWORD cluster,BPB *bpbblock,int func,BYTE *fat, int id)
{
   DWORD i,x,b,cur = cluster; //obtain starting cluster
   WORD index,*temp;
   int ret=0;
   int fat_type;
      
   if (cluster==0) return 0;
   
   fat_type = fat_get_fat_type(id, bpbblock);
   
   do {
   
      cur = cluster;
      cluster = obtain_next_cluster(cluster, fat, fat_type, bpbblock,id);
      ret++;
   }
   while (cluster<fat_get_eoc(fat_type));
   
   if (func==1) //function 1: return last cluster
      return cur;
   else
      return (ret * bpbblock->sectors_per_cluster); //function 2: return total *sectors*
};

DWORD update_dirs(BPB *bpbblock,vfs_node *tdir,BYTE *fat,int id)
{
   int i,i2;
   vfs_node *parentdir=tdir->path;
   fatdirentry *dir=(fatdirentry*)tdir->misc2;
   //write the updated directory to the disk
   if (tdir->attb&FILE_MOUNT) //root directory? perform raw write
   {
      int i ,ofs = 0;
      int blockloc, sector;
      
      #ifdef WRITE_DEBUG
      printf("updating the root directory..\n");
      #endif

      //For a FAT32 volume
      if (fat_get_fat_type(id,bpbblock) == FAT12_FAT32)
      {
            BPB32 *bpb32 = bpbblock;
            DWORD root_cluster = bpb32->rootcluster;
            blockloc = clustertoblock(bpbblock,root_cluster);
            sector = get_sector_fromcluster(root_cluster,&bpbblock,0,fat,id);
      }
      else      
      //For a FAT12/16 volume
      {
          blockloc=fat_sectors_per_fat(bpbblock)*bpbblock->num_fats+
          bpbblock->num_boot_sectors;
          
          sector=sizeof(fatdirentry)*bpbblock->num_root_dir_ents/
          bpbblock->bytes_per_sector;
          
          //queue the WRITE request
          DWORD handle=dex32_requestIO(id,IO_WRITE,blockloc,sector,dir);
          while (!dex32_IOcomplete(handle)) ;
          dex32_closeIO(handle);   
      };

   }
   else
      //a normal subdirectory
   {
      fatdirentry *direntry= (fatdirentry*) tdir->misc;
      char temp[255];
      file12tostr(direntry,temp);
      //printf("dir name from parent: %s size %d\n", temp,tdir->miscsize2);
      //printf("starting sector: %d \n",direntry->st_clust);
      writefile12EX2(direntry,bpbblock,tdir->misc2,1,0,tdir->miscsize2,id);
   };
   
};

DWORD update_fats(BPB *bpbblock,BYTE *fat,int id)
{
   int i,i2;
   int fat_start_sector   = bpbblock->num_boot_sectors;
   int total_fat_clusters = fat_sectors_per_fat(bpbblock);

   //write the file allocation table to the disk
   //there are two identical FAT, so we must also write to the other..
   for (i=0;i<2;i++)
   {
   DWORD handle=dex32_requestIO(id,IO_WRITE,fat_start_sector,
                 total_fat_clusters,(void*)fat);
                 
   while (!dex32_IOcomplete(handle)) ;
   dex32_closeIO(handle);
   
   fat_start_sector+=fat_sectors_per_fat(bpbblock);
   };
      
};

DWORD update_dirs_fats(BPB *bpbblock,BYTE *fat,vfs_node *tdir,int id)
{
   update_dirs(bpbblock,tdir,fat,id);
   if (fat!=0) update_fats(bpbblock,fat,id);
};


int fat_getsectorsizeEX(vfs_node *f,int id)
{
   BPB  *buf=(BPB*)malloc(512);
   vfs_node *parentdir=(vfs_node*)f->path;
   BYTE *fat = 0;
   DWORD sectors=0;
   int i;
   
   readBPB(buf,id);
   
   fat=(BYTE*)malloc(fat_sectors_per_fat(buf)*512);//allocate memory for FAT
   for (i=0;i<fat_sectors_per_fat(buf);i++)
   {
      DWORD handle=dex32_requestIO(id,IO_READ,buf->num_boot_sectors+i,1,(void*)(fat+i*512));
      while (!dex32_IOcomplete(handle)) ;
      dex32_closeIO(handle);
   };
   
   fatdirentry *desc=(fatdirentry*)f->misc;
   sectors=getdirsectorsize(desc,buf,2,fat,id);
   free(buf);
   free(fat);
   return sectors;
;};


void fat_rewritefileEX(vfs_node *f,int id)
{
   BPB  *buf=(BPB*)malloc(512);
   readBPB(buf,id);
   fat_rewritefile(f,buf,id);
   free(buf);
;};



DWORD fat_modifyattb(vfs_node *f, DWORD attb,int id)
{
   fatdirentry *dir,*pdir;
   int i,cur,fc;
   char temp[255];
   vfs_node *parentdir=(vfs_node*)f->path;
   BYTE *fat = 0;
   BPB  *bpbblock=(BPB*)malloc(512);
   readBPB(bpbblock,id);
   
   fat=(BYTE*)malloc(fat_sectors_per_fat(bpbblock)*512);//allocate memory for FAT
   
   loadfat(bpbblock,fat,id);
   
   fatdirentry *desc=(fatdirentry*)f->misc;
   
   if (attb&FAT12_FSIZE)
      desc->file_size=f->size;
   
   if (attb&FAT12_FNAME)
   {
      strtofile12(f->name,temp);
      memcpy(desc->name,temp,11);
      file12tostr(desc->name,f->name);
   };
   
   
   update_dirs(bpbblock,parentdir,fat,id);
   free(bpbblock);
   free(fat);
};

//This function is when the vfs wants to append sectors to a file
DWORD fat_addsectors(vfs_node *f,BPB *bpbblock,
DWORD sectors /*sectors to add*/,int id)
{
   fatdirentry *pdir;
   int i,cur,fc,total,fat_type;
   vfs_node *parentdir=(vfs_node*)f->path;
   fatdirentry *desc=(fatdirentry*)f->misc;
   BYTE *fat = 0;
   int ind=0;
   
#ifdef WRITE_DEBUG
   printf("fat_addsectors() called: parent directory name %s..\n",f->path->name);
   // getch();
#endif
   
   
    fat_type = fat_get_fat_type(id,bpbblock);         

    if (fat_type!=FAT12_FAT32)
    {
      fat=(BYTE*)malloc(bpbblock->sectors_per_fat*512);//allocate memory for FAT
      loadfat(bpbblock,fat,id);
    };
         
   
#ifdef WRITE_DEBUG2
   printf("reading FAT..\n");
   // getch();
#endif
  
#ifdef WRITE_DEBUG
   printf("getting size..\n");
   // getch();
#endif
   
   desc->file_size=f->size; //record the new size;
   
#ifdef WRITE_DEBUG
   printf("getting the last sector \n");
   printf("starting cluster of file is %d\n",desc->st_clust);
#endif
   
   cur = getdirsectorsize(desc,bpbblock,1,fat,id); //get the last sector
   
#ifdef WRITE_DEBUG2
   printf("doing stuff..last cluster located at: %d \n",cur);
   printf("Sectors requested: %d\n",sectors);
   getch();
#endif
   
   for (i=0;i<sectors;i++)
   {
      fc=obtainfreecluster(fat,bpbblock->total_sectors-3);

      /*Mark this cluster as being used*/     
      fat_write_cluster(fc,1,fat,fat_type,bpbblock,id);
      
      if (fc==-1) //out of space on disk??
      {
         free(fat);
         printf("fat: Out of space on the device\n");
         return -1;
      };
      //write to the last sector
#ifdef WRITE_DEBUG2
      printf("writing to cluster %d value %d.\n",cur,fc);
      getch();
#endif

      if (cur==0) 
      {
      desc->st_clust=fc;
      f->start_sector = fc;
      }
            else
      fat_write_cluster(cur,fc,fat,fat_type,bpbblock,id);

      
      cur=fc;
   };

   /*write cluster done*/   
   fat_write_cluster(cur,fat_get_eoc(fat_type),fat,fat_type,bpbblock,id);

   //write the update FAT and directories to the disk
#ifdef WRITE_DEBUG
   printf("updating...\n");
#endif
   
   update_dirs_fats(bpbblock,fat,parentdir,id);
   
   if (fat_type!=FAT12_FAT32) free(fat);
   
#ifdef WRITE_DEBUG
   printf("Add sectors complete..\n");
#endif
   
   return 1;
   
};


DWORD fat_deletefile(vfs_node *f,int id)
{
   fatdirentry *desc;
   BPB bpbblock;
   vfs_node *parentdir;
   BYTE *fat = 0;
   int cluster,i;
   int fat_type;
   readBPB(&bpbblock,id);
   // dir=(fatdirentry*)f->path->misc;
   desc=(fatdirentry*)f->misc;
   
   fat_type = fat_get_fat_type(id,&bpbblock);
   
   f->size=0;
   desc->file_size=0;
   desc->name[0]=0xe5;
   cluster=desc->st_clust; //obtain the starting cluster

   if (fat_type!=FAT12_FAT32)
   {
           fat=(BYTE*)malloc(fat_sectors_per_fat(&bpbblock)*512);//allocate memory for FAT
           loadfat(&bpbblock,fat,id);
   };
   
   parentdir=(vfs_node*)f->path; //the parent directory of the file ... : )
   
   
   
   cluster=obtaincluster(desc->st_clust,fat);
   if (desc->st_clust!=0)
   {
      fat_write_cluster(desc->st_clust,0,fat,fat_type,&bpbblock,id);
      
      while (cluster < fat_get_eoc(fat_type))
      {
         DWORD next_cluster=obtaincluster(cluster,fat);
         fat_write_cluster(cluster,0,fat,fat_type,&bpbblock,id);
         cluster=next_cluster;
      };
   };
   
   desc->name[0]=0xe5; //mark file as deleted.
   
   update_dirs_fats(&bpbblock,fat,parentdir,id);
   
   if (fat_type!=FAT12_FAT32) free(fat);
};


DWORD fat_rewritefile(vfs_node *f,BPB *bpbblock,int id)
{
   fatdirentry *desc;
   vfs_node *parentdir;
   BYTE *fat = 0;
   int cluster,i;
   int fat_type;
#ifdef WRITE_DEBUG2
   printf("fat_rewritefile(vfs_node *f,BPB *bpbblock) called..\n");
#endif
   fat_type = fat_get_fat_type(id,bpbblock);

   if (fat_type!=FAT12_FAT32)
   {
           fat=(BYTE*)malloc(fat_sectors_per_fat(bpbblock)*512);//allocate memory for FAT
           loadfat(bpbblock,fat,id);
   };
   
   // dir=(fatdirentry*)f->path->misc;
   desc=(fatdirentry*)f->misc;
   
   f->size=0;
   desc->file_size=0;
   
   cluster=desc->st_clust; //obtain the starting cluster
   parentdir=(vfs_node*)f->path; //the parent directory of the file ... : )
   
   cluster=obtaincluster(desc->st_clust,fat);
   if (desc->st_clust!=0)
   {
      fat_write_cluster(desc->st_clust,0,fat,fat_type,bpbblock,id);
      while ( cluster < fat_get_eoc(fat_type) )
      {
         DWORD next_cluster=obtaincluster(cluster,fat);
         fat_write_cluster(cluster,0,fat,fat_type,bpbblock,id);
         cluster=next_cluster;
      };
   };
   
   desc->st_clust=0;
   f->start_sector = 0;
   
   update_dirs_fats(bpbblock,fat,parentdir,id);
   
   if (fat_type!=FAT12_FAT32) free(fat);
   
#ifdef WRITE_DEBUG2
   printf("fat_rewritefile(vfs_node *f,BPB *bpbblock) done.\n");
#endif
   
};

DWORD fat_createfile(vfs_node *f,BPB *bpbblock,int id)
{
   fatdirentry *dir,*pdir;
   char temp[255],*s;
   int i,i2,i3,fc,maxindex;
   vfs_node *parentdir=(vfs_node*)f->path; //the parent directory of the file ... : )
   BYTE *fat = 0;
   int ind=0, fat_type;
   DWORD dirsize,foundslot=0,placemarker=0;
   
   fat_type = fat_get_fat_type(id,bpbblock);
   
#ifdef WRITE_DEBUG
   printf("fat2_createfile() called: parent directory name %s..\n",f->path->name);
   getch();
#endif
   
   
   dir=(fatdirentry*)f->path->misc2;
   
   if (fat_type!=FAT12_FAT32)
   {
       fat=(BYTE*)malloc(fat_sectors_per_fat(bpbblock)*512);//allocate memory for FAT
       loadfat(bpbblock,fat,id);
   };
   
   
   //get the size of the directory
   if (!f->path->attb&FILE_MOUNT) //must not be root directory
   {
      dirsize=getdirsectorsize((fatdirentry*)parentdir->misc,bpbblock,0,fat,id)*bpbblock->bytes_per_sector;
      maxindex=dirsize/sizeof(fatdirentry);
      
#ifdef WRITE_DEBUG
      printf("directory..max: %d entries..\n",maxindex);
#endif
      
   }
   else
      maxindex=bpbblock->num_root_dir_ents;
   
   for (i=0;i<maxindex;i++)
   {
      if (dir[i].name[0]==0xe5 ||
      dir[i].name[0]==0x00)
      {
         int spacemode=0;
         dex32_datetime d;
         dos_date dosdate;
         
         if (dir[i].name[0]==0x00)
         {
            if (i+1<maxindex)
               dir[i+1].name[0]=0;
         };
         //allocate a cluster for the file
#ifdef WRITE_DEBUG
         printf("directory space allocated..\n");
#endif
         
         if (fc==-1) //out of space on disk?
         {
            printf("FAT12DRVR: Device out of space!!\n");
            free(fat);
            return 0;
            
         };
         
         //convert dex's VFS format filename to MS-DOS 8.3 format
         strtofile12(f->name,temp);
         memcpy(dir[i].name,temp,11);
         
         //update the filename in the VFS to reflect the change to MS-DOS 8.3 format
         file12tostr(&dir[i],f->name);
         
#ifdef WRITE_DEBUG
         printf("writing used cluster to fat in memory.\n");
#endif
         
         dir[i].st_clust=0;
         dir[i].file_size=0;
         f->start_sector = 0;
         memset(&dir[i].attrib,0,sizeof(dir[i].attrib));
         //compute and record the date create for this file
         
         getdatetime(&d);
         
         f->date_created=d;
         f->date_modified=d;
         
         dosdate.year=(d.year-1980)&0x7f;
         dosdate.date=d.day;
         dosdate.month=d.month;
         
         dir[i].cdate=dosdate; //store the date created
         dir[i].mdate=dosdate; //store the date modified
         
         
         f->misc=&dir[i];
         f->miscsize=0;
         f->miscsize2=0;
         
         /*create a directory?*/
         if (f->attb&FILE_DIRECTORY)
         {
            fatdirentry *dirent;
            vfs_node *p;
            fc=obtainfreecluster(fat,bpbblock->total_sectors-3);
            fat_write_cluster(fc,fat_get_eoc(fat_type),fat,fat_type,bpbblock,id);
           
            dir[i].attrib = ATTR_DIRECTORY;
            /* allocate a cluster for this directory*/
            dir[i].st_clust = fc;
            f->start_sector = fc;
            dirent = (fatdirentry*) malloc(512);
            f->misc2=(void*)dirent;
            f->miscsize2 = 512;
            memset(f->misc2,0,512);
            
            
            /* set up the initial entries . and ..*/
            p = (vfs_node*)malloc(sizeof(vfs_node));
            memset(p,0,sizeof(vfs_node));
            strcpy(p->name,".");
            p->attb = FILE_DIRECTORY;
            p->path = f;
            memset(dirent[0].name,' ',11);
            dirent[0].name[0]='.';
            dirent[0].attrib = ATTR_DIRECTORY;
            dirent[0].st_clust = dir[i].st_clust;
            dirent[0].cdate = dosdate;
            dirent[0].mdate = dosdate;
            p->misc = &dirent[0];
            p->miscsize = 0;
            p->misc2 = f->misc2;
            p->miscsize2 = f->miscsize2;
            p->next = 0;
            f->files = p;
            
            
            p = (vfs_node*)malloc(sizeof(vfs_node));
            memset(p,0,sizeof(vfs_node));
            strcpy(p->name,"..");
            p->attb = FILE_DIRECTORY;
            p->path = f;
            memset(dirent[1].name,' ',11);
            dirent[1].name[0]='.';
            dirent[1].name[1]='.';
            dirent[1].attrib = ATTR_DIRECTORY;
            dirent[1].cdate = dosdate;
            dirent[1].mdate = dosdate;
            
            p->misc = &dirent[1];
            p->miscsize = 0;
            p->misc2 = f->path->misc2;
            p->miscsize2 = f->path->miscsize2;
            p->next = 0;
            f->files->next = p;
            f->size = 0 ;
            if (f->path->misc!=0)
            {
               fatdirentry *dirent2 = (fatdirentry*) f->path->misc;
               dirent[1].st_clust = dirent2->st_clust;
            }
            else /*root directory?*/
            {
               dirent[1].st_clust = 0;
            };
            
            writefile12EX2(f->misc,bpbblock,dirent,1,0,512,id);
         };
         
         
         foundslot=1; //successfully found a slot in the directory
         break;
      };
      
   ;};
   
   if (!foundslot) //no slot was found??
   {
      //try to add a new sector
      if (!fat_addsectors(parentdir,bpbblock,1,id))
      {
         //cannot add a new sector, no more disk space probably?
#ifdef WRITE_DEBUG
         printf("not enough space to add sectors to directory!!\n");
#endif
         
         if (fat_type!=FAT12_FAT32) free(fat);
         return 2; //perform retry
      };
   };
   
   //commit changes...
   //write the updated FATS and directories to the disk ...
   update_dirs_fats(bpbblock,fat,parentdir,id);
   
   if (fat_type!=FAT12_FAT32) free(fat);
   printf("success!!\n");
   
   
   return 1; //success
;};


int fat_mount_root(vfs_node *mountpoint,int id)
{
   BPB         *bpb;
   int fat_type,size;
   if (mountpoint->files!=0) return -1;
   bpb=(BPB*)malloc(512);
      
 
   readBPB(bpb,id); //read the bios parameter block
   printf("done\n");   
   fatdirentry *fatdir;

   //=(fatdirentry*)malloc(bpb->num_root_dir_ents*sizeof(fatdirentry));
   fat_type = fat_get_fat_type(id,bpb);
   if (fat_type==FAT12_FAT32)
   printf("FATDRVR: mounting FAT32.\n");
     else
   if (fat_type == FAT12_FAT16)
   printf("FATDRVR: mounting FAT16.\n");
     else
   if (fat_type == FAT12_FAT12)
   printf("FATDRVR: mounting FAT12.\n");
   
   printf("         Sectors per cluster: %d\n", bpb->sectors_per_cluster);
   printf("         bytes per sector: %d\n", bpb->bytes_per_sector);
   
   printf("fat: loading root directory.\n");
   size = loadroot(&fatdir,bpb,id);
   
   mountpoint->misc=0;
   mountpoint->miscsize=0;
   mountpoint->misc2=(void*)fatdir;
   mountpoint->miscsize2=size;
   mountpoint->fsid = fat_deviceid;
   mountpoint->memid = id;
   mountpoint->attb = mountpoint->attb | FILE_OREAD | FILE_OWRITE;
   mountpoint->files = 0;
   //mount the directories
#ifdef DEBUG_FAT12
   printf("fat: Calling fat_mount\n");
#endif
   fat_mount(mountpoint,fatdir,bpb,id);
   free(bpb);
   return 1;
};

char *unicodetoascii(WORD *unicodestr,char *targ,int length)
{
   int i;
   
   for (i=0;unicodestr[i]&&( (length!=0&&i<length) || length==0)  ;i++)
      targ[i]=(char)unicodestr[i];
   targ[i]=0;
   return targ;
   
};

/******************************************************************************
fat_mountdirectory() - mounts a driectory
*/

int fat_mountdirectory(vfs_node *directory, int id)
{
     BPB bpbblock;
     fatdirentry *buf3;
     int dirsize;
     
     //read the bios parameter block
     readBPB(&bpbblock,id);

     //load the directory information
     dirsize=loaddirectory(directory->misc,&buf3,id);
     
     directory->misc2 = (void*)buf3;
     directory->miscsize2 = dirsize;
     directory->attb = FILE_DIRECTORY | FILE_OREAD | FILE_OWRITE;
     
     fat_mount(directory,buf3,&bpbblock,id);
     return 1;
};


//recursively mounts directories
int fat_mount(vfs_node *mountpoint,fatdirentry *buf2,BPB *bpb,int id)
{
   
   
   vfs_node *node;
   char temp[255],temp2[255];
   char lfn_buffer[255];// a buffer used for the long filename
   int i,long_filename_scan=0,lfn_size=0;
   int total_files=0, fat_type;
   DWORD total=0;
   
   total_files = mountpoint->miscsize2 / sizeof(fatdirentry);
   
   strcpy(lfn_buffer,"");
   
   fat_type = fat_get_fat_type(id,bpb);
   
   for (i=0;i< total_files ;i++)
      
   {
      vfs_node *node;
      char filename[255];
      
      file12tostr(&buf2[i],filename);
      
#ifdef DEBUG_FAT12
      printf("fat: Adding %s\n",filename);
#endif
      if ((unsigned char)filename[0]==0x00) break;
      //check for FAT12 deleted marks and end marks
      if ((unsigned char)filename[0]!=0xe5&&
      (unsigned char)filename[0]!=0x00)
      {
         dos_date d;
         
         if ((buf2[i].attrib&ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
         {
            dos_long_entry *dl=(dos_long_entry*)&buf2[i];
            //A long filename sub-component
#ifdef FAT12_LFNSUPPORT
            
            if (long_filename_scan==0) //The first of a series of LFN entries?
            {
               strcpy(lfn_buffer,"");
               
               strcat(lfn_buffer,unicodetoascii(dl->name1,temp,5));
               strcat(lfn_buffer,unicodetoascii(dl->name2,temp,6));
               strcat(lfn_buffer,unicodetoascii(dl->name3,temp,2));
               
               
            }
            else
            {
               
               strcpy(temp2,lfn_buffer);
               strcpy(lfn_buffer,unicodetoascii(dl->name1,temp,5));
               strcat(lfn_buffer,unicodetoascii(dl->name2,temp,6));
               strcat(lfn_buffer,unicodetoascii(dl->name3,temp,2));
               
               strcat(lfn_buffer,temp2);
               
            };
            
            if (dl->order!=LAST_LONG_ENTRY)
               long_filename_scan = 1;
            else
               long_filename_scan = 0;
#endif
            
         }
         else
         {
            
            long_filename_scan = 0;
            
            //allocate a new vfs_node
            node=(vfs_node*)malloc(sizeof(vfs_node));
            memset(node,0,sizeof(vfs_node));
            vfs_createnode(node,mountpoint);
            
            //fill in the required fields of the vfs node
#ifdef FAT12_LFNSUPPORT
            if (strcmp(lfn_buffer,"")!=0) //A long filename was detected before this entry, so use it
            {
               strcpy(node->name,lfn_buffer);
               strcpy(lfn_buffer,"");
            }
            else
#endif
            strcpy(node->name,filename);
            
            //tell the VFS that this node is accessed using fat
            node->fsid=fat_deviceid;
            node->memid = id;
            node->locked=0;
            node->files=0;
            node->size=buf2[i].file_size;
            node->start_sector = buf2[i].st_clust;
            //convert from DOS date format to dex32 VFS date format
            d=buf2[i].cdate; //The date created
            
            node->date_created.day=d.date&0x1f;
            node->date_created.year=d.year&0x7f;
            node->date_created.year+=1980;
            node->date_created.month=d.month&0xf;
            
            d=buf2[i].mdate; //The date modified
            
            node->date_modified.day=d.date&0x1f;
            node->date_modified.year=d.year&0x7f;
            node->date_modified.year+=1980;
            node->date_modified.month=d.month&0xf;
            
            node->misc=&buf2[i];
            node->miscsize=0;
            node->miscsize2=0;
            
           
            node->attb = FILE_OREAD | FILE_OWRITE;
            
            if (buf2[i].ext[0]=='E' &&
                buf2[i].ext[1]=='X' &&
                buf2[i].ext[2]=='E')
            node->attb |= FILE_OEXE;     
           
            if (buf2[i].attrib&FDIRECTORY)
            {
               int dirsize;
               fatdirentry *buf3;
               node->attb|=FILE_DIRECTORY;
               
               if (strcmp(node->name,".")!=0&&strcmp(node->name,"..")!=0)
               {
                       if (fat_type == FAT12_FAT32 || fat_type == FAT12_FAT16)
                               node->files = VFS_NOT_MOUNTED;
                           else
                       {    
                           //load the directory information
                           dirsize=loaddirectory(&buf2[i],&buf3,id);
                           node->misc2=(void*)buf3;
                           node->miscsize2=dirsize;
                           fat_mount(node,buf3,bpb,id);
                       };
               };
            };
            
         };
         
      };
   };
   

;};


int obtaincluster(int cluster,BYTE *fat)
{
   WORD *temp;
   int x,index;

   index=cluster;
   x=(index*3)/2; //index=cluster*3/2
   temp=(WORD*)(fat+x);
   index=*temp;
   if (cluster & 1) //cluster is odd
   {
      return index >> 4;// take higher 12 bits
   }
   else
   {
      return index & 0xFFF;//  take lower 12 bits
   };
   
   return 0;
;};


int obtainfreecluster(BYTE *fat,int maxentries)
{
   int i;
   for (i=0;i<maxentries;i++)
      if (obtaincluster(i,fat)==0) return i;
   return -1;
;};

int fillsectorinfo(fatdirentry *dir,BPB *bpbblock,DWORD *sectinfo,int id)
{
   unsigned int cluster=dir->st_clust,i,x,b,v=0,n=0; //obtain starting cluster
   WORD index,*temp;
   BYTE *fat;
#ifdef DEBUGX
   // printf("Allocating FAT\n");
#endif
   //printf("fill sector info called\n");
   fat=(BYTE*)malloc(fat_sectors_per_fat(bpbblock)*512);//allocate memory for FAT
   for (i=0;i<fat_sectors_per_fat(bpbblock);i++)
   {
      DWORD handle=dex32_requestIO(id,IO_READ,bpbblock->num_boot_sectors+i,1,(void*)(fat+i*512));
      while (!dex32_IOcomplete(handle)) ;
      dex32_closeIO(handle);
      
      /* if (!read_block(bpbblock->num_boot_sectors+i,(void*)(fat+i*512)))
      { //check for errors
      free(fat);
      return 0;
      }; //read the FAT into memory*/
      };
      do {
         b=((cluster - 2)*(bpbblock->sectors_per_cluster) )+
      bpbblock->num_boot_sectors+
      (bpbblock->num_fats*bpbblock->sectors_per_fat)+
      ((bpbblock->num_root_dir_ents * 32) +
      (bpbblock->bytes_per_sector - 1)) / bpbblock->bytes_per_sector;
      
      sectinfo[n]=b;
      
      n++;
      //compute the next cluster
      x=(cluster*3)/2; //index=cluster*3/2
      temp=(WORD*)(fat+x);
      index=*temp;
      if (cluster & 1) //cluster is odd
         {
      cluster=index >> 4;// take lower 12 bits
      }
      else
         {
      cluster=index & 0xFFF;//  take higher 12 bits
      };
      }
      while (cluster<FAT12_END_CLUSTER);
#ifdef DEBUGX
      //  printf("ending..\n");
#endif
      free(fat);
      return 1; //cannot find cluster?
      ;};

      
int writefile12EX2(fatdirentry *dir,BPB *bpbblock,char *buf,int se,int start,int end,int id)
      {
         unsigned int cluster, sector, ofs=0 , block=0; //obtain starting cluster
         DWORD handle;
         DWORD bytes_per_cluster = fat_getbytesperblock(id);
         DWORD startblock = start / bytes_per_cluster;
         DWORD endblock =   end / bytes_per_cluster;
         DWORD adj= end % bytes_per_cluster + 1;
         DWORD startadj =  start % bytes_per_cluster ;
         DWORD startlength= bytes_per_cluster - startadj;
         DWORD sectors_per_cluster = bpbblock->sectors_per_cluster;
         BYTE *fat = 0;
         int fat_type;
         void *temp_buffer;
         
         
         DWORD total_requests = endblock-startblock + 1;
         
         fat_type = fat_get_fat_type(id,bpbblock);         

         cluster=dir->st_clust;
         
         if (cluster==0) return 0;
         
         if (fat_type!=FAT12_FAT32)
         {
         fat=(BYTE*)malloc(bpbblock->sectors_per_fat*512);//allocate memory for FAT
         loadfat(bpbblock,fat,id);
         };
         
         ofs = 0;
         
         //allocate temporary buffer, where we will place our data
         temp_buffer = (void*)malloc(bytes_per_cluster);
         
         do {
            
            /*convert cluster numbers to sectors numbers, since the block device
            only understands sector numbers*/
            sector = clustertoblock(bpbblock,cluster);
            
            //Check if this block is in the range of the raquested clusters
            if (block >=startblock && block<=endblock)
            {

               /*when writing a block, there are 4 cases, 3 of those cases need to read 
                 from the disk.*/
               if (block == startblock || block == endblock)
               {
                  //In performing a write operation, we have to read the current
                  //data on the sector, copy data from the buffer and then
                  //write it back to the sector since disks could only write on
                  //a sector by sector basis
                  DWORD hdl=dex32_requestIO(id,IO_READ, sector, sectors_per_cluster, temp_buffer);
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
                  memcpy((void*) temp_buffer, buf + ofs, bytes_per_cluster);
                  ofs+= bytes_per_cluster;
               };
               
               
               //queue the request to the IO manager for the actual write
               handle = dex32_requestIO(id,IO_WRITE, sector ,sectors_per_cluster, temp_buffer);
               
               //since this is a write, which cannot be reordered, we must wait for it to finish.
               while (!dex32_IOcomplete(handle)) ;
               dex32_closeIO(handle);
               
               //if this is the last block of requested writes we're done                              
               if ( block == endblock ) break;
            };
            
            //compute the next cluster from the FAT
            cluster = obtain_next_cluster(cluster,fat,fat_type,bpbblock,id);
            block ++;
            
         }
         //check if this is the last block of the file, if so we stop.
         while (cluster < fat_get_eoc(fat_type));
         

         free(temp_buffer);
                  
         if (fat!=0)
         free(fat);
         return 1; //success
      ;};
      
      
DWORD fat_openfileEX(vfs_node *f,char *bufr,int start,int end,int id)
{
   BPB    *bpbblock=(BPB*)malloc(512);
   //perform the read
   
   fatdirentry   *dir=0;
   int found=0,size=0,i;
   readBPB(bpbblock,id);
   
   if (f!=0)
      dir=(fatdirentry*)f->misc;
      
   if (dir&&bufr)
   {
      if (!loadfile12EX2(dir,bpbblock,bufr,1,start,end,id))
      {
         //error while loading the file
         
         free(bpbblock);
         return 0;
      };
   };
free(bpbblock);
return size;
};

int loadfile12EX2(fatdirentry *dir,BPB *bpbblock,char *buf,int se,int start,int end,int id)
      {
         unsigned int cluster=( (DWORD)dir->st_clust_msw << 16) | (DWORD)dir->st_clust;
         unsigned int i,x, sector , ofs=0 , block = 0; //obtain starting cluster
         DWORD bytes_per_cluster = bpbblock->sectors_per_cluster*512;
         DWORD startblock, endblock, adj, startadj, startlength;
         
         startblock  = start / bytes_per_cluster;
         endblock    = end / bytes_per_cluster;
         adj         = (end % bytes_per_cluster) + 1;
         startadj    = start % bytes_per_cluster;
         startlength = bytes_per_cluster - startadj;
         
         WORD index,*temp;
         BYTE *fat = 0;
         int ind=0, total_requests = 0 ,fat_type;
         DWORD *handle;
         DWORD *xbuf;
         
         char temps[250];
         char t1[16],t2[16],t3[16];
         
         DWORD cluster_requests = endblock-startblock + 1;
        
         handle = (DWORD*)malloc( cluster_requests*sizeof(DWORD));
         xbuf   = (DWORD*)malloc( cluster_requests*sizeof(DWORD));
         
         
         if (cluster==0) return 0;
         
         
         fat_type  = fat_get_fat_type(id,bpbblock);
         
         if (fat_type!=FAT12_FAT32)
         {
            fat=(BYTE*)malloc(fat_sectors_per_fat(bpbblock)*512);//allocate memory for FAT
            loadfat(bpbblock,fat,id);
         };
         
         block = 0;
         
         do {
            //determine the starting sector this cluster resides      
            sector = clustertoblock(bpbblock,cluster);
            
            if (  block >= startblock && block <= endblock)
            {
               void *tbuf=(void*)malloc(bytes_per_cluster);

               handle[ind]=dex32_requestIO(id,IO_READ,sector,bpbblock->sectors_per_cluster,
                                  tbuf);
               xbuf[ind]=(DWORD)tbuf;
               ind++;
               
               if (ind >= cluster_requests) break;
               if ( block == endblock ) break;
               
            };
            
            //compute the next cluster
            cluster = obtain_next_cluster(cluster,fat,fat_type,bpbblock,id);
            block++;
         }
         while (cluster<fat_get_eoc(fat_type));

         printf("waiting..\r");
         
         //wait until the IO manager completes the read requests
         for (i=0;i<ind;i++)
         {
             while (!dex32_IOcomplete(handle[i]));     
         };

         //copy data read into the buffer   
         ofs = 0;      
         for (i=0;i<ind;i++)
         {
            if (i==startblock && i==endblock)
            {
               memcpy((void*)buf,xbuf[i]+startadj,end-start + 1);
               ofs += (end-start);
               break;
            }
            else
            if (i==startblock)
            {
               memcpy((void*)buf,xbuf[i]+startadj,startlength);
               ofs += startlength;
            }
            else
            if (i==endblock)
            {
               memcpy((void*)(buf+ofs),xbuf[i],adj);
               ofs += adj;
               break;
            }
            else
            {
               memcpy((void*)(buf+ofs),xbuf[i],bytes_per_cluster);
               ofs += bytes_per_cluster;
            };
            
         };
         

         for (i=0;i<ind;i++) 
         {
             free(xbuf[i]);      
             dex32_closeIO(handle[i]);
         };
         
         free(handle);
         free(xbuf);
         
         if (fat!=0)
         free(fat);
         return 1; //success
      ;};
      
      
      int loadfile12(fatdirentry *dir,BPB *bpbblock,char *buf,int id)
      {
         int ret;
         
#ifdef DEBUG_READ
         printf("loadfile12 called: %d - %d\n",0,dir->file_size);
#endif
         
         ret= loadfile12EX2(dir,bpbblock,buf,0,0,dir->file_size,id);
         
#ifdef DEBUG_READ
         printf("loadfile12 done.\n");
#endif
         
         return ret;
      };
      
      
      int fat_get_eoc(int fat_type)
      {
            switch (fat_type)
            {
               case FAT12_FAT12 : return 0x0FF8;
               case FAT12_FAT16 : return 0xFFF8;
               case FAT12_FAT32 : return 0x0FFFFFF8;
            };
         return -1;
      };
      
      int fat_get_fat_type(int devid, BPB *bpb)
      {
            BPB bpbblock;
            BPB32 *bpbblock32;
            int rootdirsectors, fatsz;
            int totalsectors, datasec , countofclusters;
            
            if (bpb!=0 || devid == 0)
            memcpy(&bpbblock,bpb,sizeof(BPB));
                        else
            readBPB(&bpbblock,devid);
            
            bpbblock32 = &bpbblock;            
            
            //obtain number of root directory sectors as illustrated in the
            //FAT documentation
            rootdirsectors = ((bpbblock.num_root_dir_ents * 32) + (bpbblock.bytes_per_sector - 1 )) /
                               bpbblock.bytes_per_sector;
            
            if (bpbblock.sectors_per_fat!=0)
                 fatsz = bpbblock.sectors_per_fat;
            else
                 fatsz = bpbblock32->fatsz32;
                        
            if (bpbblock.total_sectors!=0)
                 totalsectors = bpbblock.total_sectors;
            else
                 totalsectors = bpbblock.total_sectors_large;
                        
            datasec = totalsectors - (bpbblock.num_boot_sectors + 
                      (bpbblock.num_fats * fatsz) + rootdirsectors);
                      
            countofclusters = datasec / bpbblock.sectors_per_cluster;
            
            if (countofclusters < 4085) 
                  return FAT12_FAT12;
                    else 
            if (countofclusters < 65525) 
                  return FAT12_FAT16;      
                    else 
                  return FAT12_FAT32;      
      };
      
      int fat_register(const char *name)
      {
         devmgr_fs_desc fat_fs_desc;
         memset(&fat_fs_desc,0,sizeof(devmgr_fs_desc));
         //fill up the fat_fs_desc "form" for registering with the device manager
         strcpy(fat_fs_desc.hdr.name,name);
         strcpy(fat_fs_desc.hdr.description,"MS-DOS FATfs driver");
         fat_fs_desc.hdr.type             = DEVMGR_FS;   //we define a filesystem
         fat_fs_desc.hdr.size             = sizeof(fat_fs_desc);
         fat_fs_desc.mountroot            = fat_mount_root;
         fat_fs_desc.rewritefile          = fat_rewritefileEX;
         fat_fs_desc.readfile             = fat_openfileEX;
         fat_fs_desc.chattb               = fat_modifyattb;
         fat_fs_desc.getsectorsize        = fat_getsectorsizeEX;
         fat_fs_desc.deletefile           = fat_deletefile;
         fat_fs_desc.addsectors           = fat_addsectorsEX;
         fat_fs_desc.writefile            = fat_writefileEX;
         fat_fs_desc.createfile           = fat_createfileEX;
         fat_fs_desc.getbytesperblock     = fat_getbytesperblock;
         fat_fs_desc.mountdirectory       = fat_mountdirectory;
         fat_fs_desc.validate_filename    = fat_validatefilename;
         //register this filesystem
         fat_deviceid = devmgr_register((devmgr_generic*)&fat_fs_desc);
         return fat_deviceid;
      };
      
      
