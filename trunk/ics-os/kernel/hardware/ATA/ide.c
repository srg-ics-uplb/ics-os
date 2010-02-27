/*
  Name: ide.c
  Copyright: 
  Author: Joseph Emmmanuel DL Dayo
  Date: 02/02/04 04:10
  Description: The ATA/IDE driver for harddisks and CD-ROM drives, also includes a partition
  detecteor. There is no capability for detecting extended partitions yet though.
  
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

/*Include the files from ATADRVR*/
#include "ataioint.c"
#include "ataiopio.c"
#include "ataioreg.c"
#include "ataiosub.c"
#include "ataiotmr.c"

typedef struct _ide_ata_ident {
WORD configinfo;
WORD cylinders;
WORD reserved;
WORD heads;
WORD bytes_per_track;
WORD bytes_per_sector;
WORD sectors_per_track;
WORD inter_sector_gap;
WORD phase_lock_field; 
WORD num_status_words;
WORD serial[10];
WORD controller_type;
WORD buffer_size;
WORD ecc_bytes;
WORD firmware_revision[4];
WORD model_number[20];
WORD read_write_multiples;
WORD double_word;
WORD reserved2;
WORD reserved3;
WORD min_pio_data_time;
WORD min_dma_data_time;
} ide_ata_ident;


typedef struct __attribute__((packed)) _partition_table {
    BYTE active_flag;
    char chs_info1[3];
    BYTE type;
    char chs_end[3];
    DWORD startlba;
    DWORD sector_size;
} partition_table;

typedef struct __attribute__((packed)) _partition_mbr {
    char program[446];
    partition_table tables[4];
    char magic_value[2];
} partition_mbr;


typedef struct _ide_partition_info {
    int   mydeviceid;
    int   hd_deviceid;
    DWORD startlba;
    DWORD endlba;
} ide_partition_info;

typedef struct _ide_drive_info {
int mydeviceid;
int read_only;
int atapi, asus_adjustment;
int interface,dev;
char model[21],serial[21];
int cylinder, heads, sectors_per_track;
int bytes_per_sector;
} ide_drive_info;

typedef struct __attribute__((packed)) _ide_cdrom_seek 
{
   BYTE opcode;
   BYTE reserved;
   BYTE lba_address[4];
   BYTE reserved1[6]; 
} ide_cdrom_seek;

typedef struct __attribute__((packed)) _ide_cdrom_read_packet {
BYTE opcode;
BYTE reserved;
BYTE lba_address[4];
BYTE reserved2;
BYTE blocks[2];
BYTE reserved3[3];
} ide_cdrom_read_packet;

typedef struct __attribute__((packed)) _ide_cdrom_readCD_packet {
BYTE opcode;
BYTE device_head;
BYTE lba_address[3];
BYTE sectors;
BYTE features;
} ide_cdrom_readCD_packet;


ide_drive_info ide_drivelist[4];
 
//IDE ports
#define IDE_COMMAND 0x1f7
//commands
#define IDE_IDENTIFY 0xEC

unsigned char * devTypeStr[]
       = { "no device found", "unknown type", "ATA", "ATAPI" };

int ide_ready = 0;

void partition_interpret_table( partition_mbr *mbr)
{
int i;
char temp[10],temp2[10];
for (i=0;i<4;i++)
    {
        printf("parition type: %d\n",mbr->tables[i].type);
        printf("start lba: %s   end: %s\n",itoa(mbr->tables[i].startlba,temp,10), 
                   itoa(mbr->tables[i].startlba+mbr->tables[i].sector_size,temp2,10));
        printf("------------------------------\n");
    };
};

void ide_interpret_config(ide_ata_ident *buf,ide_drive_info *driveinfo)
{
    int i, i2;
    printf("-----------------ATA Information --------------\n");
    printf("model number: ");
    i2 = 0;
    
    for (i=0;i<10;i++)
      {
        printf("%c", buf->model_number[i] >> 8);
        printf("%c", buf->model_number[i] & 0xFFFF);
        driveinfo->model[i2++] = buf->model_number[i] >> 8;
        driveinfo->model[i2++] = buf->model_number[i] & 0xFFFF;
      };  
      
    driveinfo->model[i2] = 0;
    
    printf("\n");        
    printf("Serial: ");
    
    i2 = 0;
    
    for (i=0;i<10;i++)
       {
        printf("%c", buf->serial[i] >> 8);
        printf("%c", buf->serial[i] & 0xFFFF);
        driveinfo->serial[i2++] = buf->serial[i] >> 8;
        driveinfo->serial[i2++] = buf->serial[i] & 0xFFFF;
       }; 
    
    driveinfo->model[i2] = 0;
    
    printf("\n");           
    printf("Firmware revision: ");
    for (i=0;i<4;i++)
       { 
        printf("%c", buf->firmware_revision[i] >> 8);
        printf("%c", buf->firmware_revision[i] & 0xFFFF);
       }; 

    printf("\n");    
    
    driveinfo->cylinder = buf->cylinders;
    driveinfo->heads = buf->heads;
    driveinfo->sectors_per_track = buf->sectors_per_track;
    driveinfo->bytes_per_sector = buf->bytes_per_sector;
    
    printf("Cylinders   = %d,", buf->cylinders);
    printf("Heads       = %d,", buf->heads);
    printf("Spt         = %d,", buf->bytes_per_sector);
    printf("Buffer Size = %d\n", buf->buffer_size);
};

void ide_irqhandler()
{
    ide_ready=1;
};

void ide_getdriveinfo()
{

};

int ide_readsectors(int interface,int dev,DWORD lba,DWORD sectors, char *buffer)
{
    int base;
    int i,ofs=0;
    int total_sectors = 0, block_ofs = 0;
 
 if (interface == 0) base = 0x1f0;
                        else base = 0x170;
    pio_set_iobase_addr(base, base + 0x200 );    
    
  do {  
      if (sectors>0xF0)
        {
           total_sectors = 0xF0;
           sectors-=0xF0;
        }
           else
        {  
            total_sectors = sectors;  
            sectors = 0;
        };
    
        reg_pio_data_in_lba(dev, CMD_READ_SECTORS, 0, total_sectors,
                lba +block_ofs,SYS_DATA_SEL,buffer + ofs,total_sectors,0);
        
        ofs+=0xF0 * 512;
        block_ofs+=0xF0;
                
    } while (sectors>0);
    return 1;
};

int ide_writesectors(int interface,int dev,DWORD lba,DWORD sectors, char *buffer)
{
    int base;
    int i,ofs=0;
    int total_sectors = 0, block_ofs = 0;
 
 if (interface == 0) base = 0x1f0;
                        else base = 0x170;
    pio_set_iobase_addr(base, base + 0x200 );    
    
  do {  
      if (sectors>0xF0)
        {
           total_sectors = 0xF0;
           sectors-=0xF0;
        }
           else
        {  
            total_sectors = sectors;  
            sectors = 0;
        };
    
        reg_pio_data_out_lba(dev, CMD_WRITE_SECTORS, 0, total_sectors,
                lba + block_ofs,SYS_DATA_SEL,buffer + ofs,total_sectors,0);
        
        ofs+=0xF0 * 512;
        block_ofs+=0xF0;
    } while (sectors>0);
    
    return 1;
};

BYTE switchbyte(BYTE x)
{
    BYTE temp1 = x >> 4 , temp2 = x << 4;
    return ( temp1 | temp2 );
    
};

typedef struct __attribute__((packed)) _ide_cdrom_eject_packet {
BYTE opcode;
BYTE action;
BYTE reserved[2];
BYTE start;
BYTE reserved1[3];
BYTE slot;
BYTE reserved2[2];
BYTE flag;
} ide_cdrom_eject_packet;

typedef struct __attribute__((packed)) _ide_cdrom_lock_packet {
BYTE opcode;
BYTE reserved[3];
BYTE status;
BYTE reserved2[7];
} ide_cdrom_lock_packet;

int ide_cdrom_lock(int interface,int dev)
{
   ide_cdrom_lock_packet packet; 
   int base,i,stat;
   if (interface == 0) base = 0x1f0;
              else 
                       base = 0x170;
   
   //select the drive to use
   pio_set_iobase_addr(base, base + 0x200 );    

   memset(&packet,0,sizeof(ide_cdrom_lock_packet));
   packet.opcode = 0x1e;
   packet.status = 1;

   reg_packet(dev,12, SYS_DATA_SEL,&packet,0,0,SYS_DATA_SEL,0);
};


int ide_cdrom_unlock(int interface,int dev)
{
   ide_cdrom_lock_packet packet; 
   int base,i,stat;
   if (interface == 0) base = 0x1f0;
              else 
                       base = 0x170;
   
   //select the drive to use
   pio_set_iobase_addr(base, base + 0x200 );    

   memset(&packet,0,sizeof(ide_cdrom_lock_packet));
   packet.opcode = 0x1e;
   packet.status = 0;

   reg_packet(dev,12, SYS_DATA_SEL,&packet,0,0,SYS_DATA_SEL,0);
};

int ide_cdrom_eject(int interface,int dev)
{
   ide_cdrom_eject_packet packet; 
   int base,i,stat;
   if (interface == 0) base = 0x1f0;
              else 
                       base = 0x170;
   
   //select the drive to use
   pio_set_iobase_addr(base, base + 0x200 );    

   memset(&packet,0,sizeof(ide_cdrom_eject_packet));
   packet.opcode = 0x1B;
   packet.start = 2;

   reg_packet(dev,12, SYS_DATA_SEL,&packet,0,0,SYS_DATA_SEL,0);
};

int ide_cdrom_load(int interface,int dev)
{
   ide_cdrom_eject_packet packet; 
   int base,i,stat;
   if (interface == 0) base = 0x1f0;
              else 
                       base = 0x170;
   
   //select the drive to use
   pio_set_iobase_addr(base, base + 0x200 );    

   memset(&packet,0,sizeof(ide_cdrom_eject_packet));
   packet.opcode = 0x1B;
   packet.start = 3;
   reg_packet(dev,12, SYS_DATA_SEL,&packet,0,0,SYS_DATA_SEL,0);
};

int ide_cdromreadsectors(int interface,int dev,DWORD lba,DWORD sectors, char *buffer)
{
   ide_cdrom_read_packet atapi_packet;
   ide_cdrom_seek atapi_seek_packet;
   ide_cdrom_readCD_packet atapi_read_packet;
   
   int base,i,stat;
   if (interface == 0) base = 0x1f0;
              else 
                       base = 0x170;
   
   //select the drive to use
   pio_set_iobase_addr(base, base + 0x200 );    
      
   atapi_seek_packet.opcode = 0x00;
   stat = reg_packet(dev,12, SYS_DATA_SEL,&atapi_seek_packet,0,0,SYS_DATA_SEL,0);
   
   printf("status returned: =%d\n",stat);   
   //prepare the ATAPI packet command
   memset( &atapi_packet, 0, sizeof(ide_cdrom_read_packet));
   atapi_packet.opcode = 0x28;   //read command
   
   //ATAPI lba address format and blocks seems to be in BIG endian... so we have to make
   //the necessary conversion
 
   atapi_packet.lba_address[0] = (lba & 0xFF000000) >> 24;
   atapi_packet.lba_address[1] = (lba & 0x00FF0000) >> 16;
   atapi_packet.lba_address[2] = (lba & 0x0000FF00) >> 8;
   atapi_packet.lba_address[3] = lba & 0xFF;
   
   atapi_packet.blocks[0] = (sectors & 0x0000FF00) >> 8;
   atapi_packet.blocks[1] = (sectors & 0xFF);
   
   if (reg_packet(dev,12, SYS_DATA_SEL,&atapi_packet,0,2048,
          SYS_DATA_SEL,buffer)!=0) {printf("read failure!\n");return -1;};
   
   return 1;
};

int ide_cdromgetblocksize()
{
    return 2048;
};

//A universal get bytes per block function for all ATA devices
int ide_uni_get_bytes_per_block()
{
     int info_index = -1;
     int i;
     /* get the index of the drive information structure associated with this device
        depending on which device DEX used to call this function, this is done using
        the context inforation set by the bridge when interfacing with this module*/
     for (i=0; i < 4; i++)
      {
              if (ide_drivelist[i].mydeviceid == devmgr_getcontext())
                   {info_index = i;break;};
      };
      
      if (info_index == -1) 
      {
          printf("ATA driver: unidentified device id supplied!\n");
          return -1;
      };
    
    return ide_drivelist[info_index].bytes_per_sector;  
};

//A universal get total sectors for all ATA devices
int ide_uni_get_total_sectors()
{
     int info_index = -1;
     int i;
     /* get the index of the drive information structure associated with this device
        depending on which device DEX used to call this function, this is done using
        the context inforation set by the bridge when interfacing with this module*/
     for (i=0; i < 4; i++)
      {
              if (ide_drivelist[i].mydeviceid == devmgr_getcontext())
                   {info_index = i;break;};
      };
      
      if (info_index == -1) 
      {
          printf("ATA driver: unidentified device id supplied!\n");
          return -1;
      };
    
    return ide_drivelist[info_index].sectors_per_track *
           ide_drivelist[info_index].cylinder *
           ide_drivelist[info_index].heads;  
};


//A universal read block function for all ATA devices
int ide_uni_read_block(int block,char *blockbuff,int numblocks)
{
 int info_index = -1;
 int i;
 /* get the index of the drive information structure associated with this device
    depending on which device DEX used to call this function, this is done using
    the context inforation set by the bridge when interfacing with this module*/
 for (i=0; i < 4; i++)
  {
          if (ide_drivelist[i].mydeviceid == devmgr_getcontext())
               {info_index = i;break;};
  };
  
  if (info_index == -1) 
  {
      printf("ATA driver: unidentified device id supplied!\n");
      return -1;
  };
  
  if (ide_drivelist[info_index].asus_adjustment) block-=1;
  
  /*Check if it is an IDE device, if it is we use an IDE call*/
  if (!ide_drivelist[info_index].atapi)
       return ide_readsectors(ide_drivelist[info_index].interface, 
       ide_drivelist[info_index].dev,block,numblocks,blockbuff);
  else
  /*Use the ATAPI interface*/
       return ide_cdromreadsectors(ide_drivelist[info_index].interface, 
       ide_drivelist[info_index].dev,block,numblocks,blockbuff);
};

//A universal write block function for all ATA devices
int ide_uni_write_block(int block,char *blockbuff,int numblocks)
{
 int info_index = -1;
 int i;
 /* get the index of the drive information structure associated with this device
    depending on which device DEX used to call this function, this is done using
    the context inforation set by the bridge when interfacing with this module*/
 for (i=0; i < 4; i++)
  {
          if (ide_drivelist[i].mydeviceid == devmgr_getcontext())
               {info_index = i;break;};
  };
  
  if (info_index == -1) 
  {
      printf("ATA driver: unidentified device id supplied!\n");
      return -1;
  };

  if (ide_drivelist[info_index].read_only || ide_drivelist[info_index].atapi)
  {
      printf("ATA driver: (Error) Write Protection enabled for this device.\n");
      return -1;
  };  
  /*Check if it is an IDE device, if it is we use an IDE call*/
  return ide_writesectors(ide_drivelist[info_index].interface, 
       ide_drivelist[info_index].dev, block, numblocks, blockbuff);
};


int ide_total_partition = 0;
ide_partition_info ide_partitions[10];

//read block with partition adjustments
int ide_read_block_partition(int block, char *blockbuff,int numblocks)
{
    int i;
    //determine which partition was accessed by the io manager based on the
    //current deviceid
    int device_context = devmgr_getcontext();
    
    //search for partition information in my parition lists
    for (i=0; i < ide_total_partition; i ++)
        {
             //is this it?.   
             if (ide_partitions[i].mydeviceid == device_context)
                 {
                      //obtain the device id of the HDD containing this partition            
                      devmgr_block_desc *myblock;
                      myblock = (devmgr_block_desc*)devmgr_getdevice(ide_partitions[i].hd_deviceid);
                      
                      //check if the device still exists
                      if (myblock==-1) return -1;
                      //read the required block after adjusting
                      return bridges_call(myblock,&myblock->read_block,
                             block+ide_partitions[i].startlba,blockbuff,numblocks);             
                 };   
        };    
        
    //partition not found    
    return -1;
};

//write block with partition adjustments
int ide_write_block_partition(int block, char *blockbuff,int numblocks)
{
    int i;
    //determine which partition was accessed by the io manager based on the
    //current deviceid
    int device_context = devmgr_getcontext();
    
    //search for partition information in my parition lists
    for (i=0; i < ide_total_partition; i ++)
        {
             //is this it?.   
             if (ide_partitions[i].mydeviceid == device_context)
                 {
                      //obtain the device id of the HDD containing this partition            
                      devmgr_block_desc *myblock;
                      myblock = (devmgr_block_desc*)devmgr_getdevice(ide_partitions[i].hd_deviceid);
                      
                      //check if the device still exists
                      if (myblock==-1) return -1;
                      //read the required block after adjusting
                      return bridges_call(myblock,&myblock->write_block, 
                             block+ide_partitions[i].startlba, blockbuff,numblocks);             
                 };   
        };    
        
    //partition not found    
    return -1;
};


//identify the partition name given the type ... highly unreliable considering
//many FS's share the same type... but usefult for informational purposes.
const char *ide_identify_partition_type(int part_type)
{
    switch (part_type)
        {
            case 0: return "none";  break;
            case 4: return "FAT16"; break;
            case 7: return "NTFS";  break;
            case 0xb: return "Windows95 FAT32"; break;
            case 0xc: return "Windows95 FAT32 (LBA)"; break;
            case 0xE: return "Windows95 FAT16 (LBA)";break;
            case 0xF: return "Windows95 Extended (LBA)";break; 
            case 0x82: return "Linux Swap"; break;
            case 0x83: return "Linux native fs or ext2fs"; break;
            case 0x85: return "Linux extended";break;
            default:   return "unknown";break;            
        };
        
};
//register the partitions in a HD to the device manager
int ide_registerpartitions(int deviceid)
{
    char temp[10],temp2[10];
    int i;
    devmgr_block_desc *myblock = (devmgr_block_desc*)devmgr_getdevice(deviceid);
    partition_mbr *mbr=malloc(sizeof(partition_mbr));
        
    //check if the device exists
    if (myblock==-1) return -1;

    //obtain partition information from the MBR(Master Boot Record) of the target HDD
    bridges_call(myblock,&myblock->read_block,0,mbr,1);
    
    //Interpret partition information and register them
    for (i=0;i<4;i++)
    {
        devmgr_block_desc mypartition;
        if (mbr->tables[i].type!=0)
           {
                int mydeviceid;      
                int pindex=ide_total_partition++;
                memset(&mypartition,0,sizeof(devmgr_block_desc));
                mypartition.hdr.size = sizeof(devmgr_block_desc);
                mypartition.hdr.type = DEVMGR_BLOCK;
                
                //name this partition something like hd_p1_prt0
                sprintf(mypartition.hdr.name,"%sp%d",myblock->hdr.name,i);
                
                //give a description like "FAT12 on partition 0 at hd_p0"
                sprintf(mypartition.hdr.description,"%s on partition %d at %s",
                        ide_identify_partition_type( mbr->tables[i].type),
                        i,myblock->hdr.name);
                        
                mypartition.read_block = ide_read_block_partition;
                mypartition.write_block = ide_write_block_partition;
                //register the partition to the device manager
                mydeviceid = devmgr_register((devmgr_generic*)&mypartition);
                
                ide_partitions[pindex].hd_deviceid = deviceid;
                ide_partitions[pindex].mydeviceid = mydeviceid;
                //record the lba from the partition table
                ide_partitions[pindex].startlba = mbr->tables[i].startlba;
                ide_partitions[pindex].endlba =   mbr->tables[i].startlba + 
                                                  mbr->tables[i].sector_size;
                
                        
           };
    };

    free(mbr);  
    return 1;  
};

int ide_sendmessage(int type,const char *message)
{
 int info = -1;
 int i;
 /* get the index of the drive information structure associated with this device
    depending on which device DEX used to call this function, this is done using
    the context inforation set by the bridge when interfacing with this module*/
 for (i=0; i < 4; i++)
  {
          if (ide_drivelist[i].mydeviceid == devmgr_getcontext())
               {info = i;break;};
  };

  if (info==-1) return -1;
  
    if (type == DEVMGR_MESSAGESTR)
       {
              int l = strlen(message);
              char *temp = malloc(l+1), *p;
              
              //make a temporary copy sinced we cannot modify message
              strcpy(temp,message);
              
              //get the second parameter
              p = strtok(temp," ");
              p = strtok(0," ");
              if (p!=0)
                   {
                         if ( (strcmp(p,"-eject")==0) && ide_drivelist[info].atapi)             
                             {
                                  startints();                        
                                  ide_cdrom_eject(ide_drivelist[info].interface,
                                                  ide_drivelist[info].dev);

                             }
                         else    
                         if ( (strcmp(p,"-load")==0) && ide_drivelist[info].atapi)             
                             {
                                  startints();                        
                                  ide_cdrom_load(ide_drivelist[info].interface,
                                                  ide_drivelist[info].dev);

                             }
                         else
                         if ( (strcmp(p,"-lock")==0) && ide_drivelist[info].atapi)             
                             {
                                  startints();                        
                                  ide_cdrom_lock(ide_drivelist[info].interface,
                                                  ide_drivelist[info].dev);

                             }
                         else
                         if ( (strcmp(p,"-unlock")==0) && ide_drivelist[info].atapi)             
                             {
                                  startints();                        
                                  ide_cdrom_unlock(ide_drivelist[info].interface,
                                                  ide_drivelist[info].dev);

                             }
                         else        
                         if (strcmp(p,"-asus")==0)
                             {
                                 if (ide_drivelist[info].atapi)
                                 {
                                         if (!ide_drivelist[info].asus_adjustment)
                                         {
                                              ide_drivelist[info].asus_adjustment = 1;
                                              printf("ATA-DRVR: ASUS CD-ROM drive adjustment used.\n");
                                         }
                                         else
                                         {
                                              ide_drivelist[info].asus_adjustment = 0;
                                              printf("ATA-DRVR: ASUS CD-ROM drive adjustment deactivated.\n");
                                         };
                                 };
                             }
                         else
                         if (strcmp(p,"-write_disable") == 0)
                         {
                             printf("ATA driver: IDE drive made read-only\n");
                             ide_drivelist[info].read_only = 1;
                         }
                         else
                         if (strcmp(p,"-write_enable") == 0)
                             {
                             char ans;
                             printf("==================!!!! WARNING !!!!===================\n");
                             printf("DEX-OS IS STILL IN ITS EXPERIMENTAL STAGE\n");
                             printf("THIS ACTION MIGHT CAUSE LOSS OF DATA, DATA CORRUPTION\n");
                             printf("OR OTHER DAMAGES ON YOUR IDE DEVICE. THE AUTHOR WILL\n");
                             printf("NOT BE RESPONSIBLE FOR ANY DAMAGES THAT MIGHT OCCUR.\n");
                             printf("use the -write_disable option to enable Read-Only again\n");
                             printf("==================!!!! WARNING !!!!===================\n");
                             ide_drivelist[info].read_only = 0;
                             }
                         else
                         if (strcmp(p,"-stat")==0)
                             {
                             printf("Interface #  : %d\n", ide_drivelist[info].interface);
                             printf("Device    #  : %d\n", ide_drivelist[info].dev);
                             printf("Model        : %s\n", ide_drivelist[info].model);
                             printf("Serial       : %s\n", ide_drivelist[info].serial);
                             printf("Read Only?   : %s\n", ide_drivelist[info].read_only ? "Yes" : "No");
                             printf("DEX device ID: %d\n", ide_drivelist[info].mydeviceid);
                             printf("ASUS adjust? : %s\n", ide_drivelist[info].asus_adjustment ? "Yes" : "No");
                             }
                         else
                         if (strcmp(p,"-ver")==0)
                             {
                                  printf("ATA-DRVR for DEX-OS version 1.00\n");
                                  printf("IDE drives are read-only\n");
                             }
                         else
                         printf("ATA-DRVR: unrecognized command.\n");
                   }
              else
                   {
                         printf("List of commands:\n");
                         printf("-stat         : Displays status information\n");
                         printf("-ver          : Shows version information\n");
                         
                         if (ide_drivelist[info].atapi)
                         {
                             printf("-asus         : For ASUS CD-ROM drives only\n");
                             printf("-eject        : Ejects CD\n");
                             printf("-load         : Loads the CD\n");
                             printf("-lock         : Locks CD-ROM\n");
                             printf("-unlock       : Unlocks CD-ROM\n");
                         }
                         else
                         {
                             printf("-write_enable : Write enables the IDE drive (!!Experimental!!)\n");
                             printf("-write_disable: Makes the IDE drive read-only\n");
                         };
                  }

              free(temp);
              return 1;
       };
       
       printf("ATA_DRVR: send_message called.\n");
       return 1;
};

/*This function registers the detected ATA devices to th device manager.
  If it is a harddisk partitions will also be detected and mounted.*/
void ide_setupinterface(int type,int intnum, const char *interface_str)
{
    char *buffer =(char*)malloc(512); 
    int hd1 = 0, hd2 = 0, cd1 = 0, cd2 = 0;
    int numDev;
    int base;
    int deviceid;
    int index = 2*type;
    
    if (type == 0) 
       base = 0x1f0;
    else
       base = 0x170;
    
    pio_set_iobase_addr(base, base + 0x200 );

    numDev = reg_config();
    devmgr_block_desc myblock;
    
    
 
    if (reg_config_info[0]==2) hd1 = 1;
    if (reg_config_info[1]==2) hd2 = 1;
    if (reg_config_info[0]==3) cd1 = 1;
    if (reg_config_info[1]==3) cd2 = 1;
    
    if (reg_config_info[0]!=0)
      reg_reset( 0, 0);

    if (reg_config_info[0]!=0)
      reg_reset( 0, 1);
    
    memset( buffer, 0,512);
    
    if (hd1)
    {

    
    reg_pio_data_in(0, CMD_IDENTIFY_DEVICE,0, 0, 0, 0, 0,
               SYS_DATA_SEL,buffer,1L,0);               

    
    //make it visible to the device manager
    memset(&myblock,0,sizeof(myblock));
    sprintf(myblock.hdr.name,"hd%s0",interface_str);
    sprintf(myblock.hdr.description,"ATA %s master HDD (READ-ONLY)",interface_str);
    myblock.hdr.type = DEVMGR_BLOCK;
    myblock.hdr.size = sizeof(myblock);
    
    //done with the preliminaries, now register public functions
    myblock.read_block = ide_uni_read_block;
    myblock.write_block = ide_uni_write_block;
    myblock.hdr.sendmessage = ide_sendmessage;
    myblock.get_block_size = ide_uni_get_bytes_per_block;
    myblock.total_blocks = ide_uni_get_total_sectors;

    deviceid = devmgr_register((devmgr_generic*)&myblock);
    
    ide_drivelist[index].atapi = 0;
    ide_drivelist[index].mydeviceid = deviceid;
    ide_drivelist[index].read_only = 1;
    ide_drivelist[index].dev = 0;
    ide_drivelist[index].interface = type;
    ide_interpret_config(buffer,&ide_drivelist[index]);
    
    //register the device driver and determine partitions on that disk
    ide_registerpartitions(deviceid);
    };
    
    if (hd2)
    {
    reg_pio_data_in(1, CMD_IDENTIFY_DEVICE,0, 0, 0, 0, 0,
               SYS_DATA_SEL,buffer,1L,0);    
                          
    
    //make it visible to the device manager
    memset(&myblock,0,sizeof(myblock));
    sprintf(myblock.hdr.name,"hd%s1",interface_str);
    sprintf(myblock.hdr.description,"ATA %s Slave HDD (READ-ONLY)",interface_str);
    myblock.hdr.type = DEVMGR_BLOCK;
    myblock.hdr.size = sizeof(myblock);
    
    //done with the preliminaries, now register public functions
    myblock.read_block = ide_uni_read_block;
    myblock.write_block = ide_uni_write_block;
    myblock.hdr.sendmessage = ide_sendmessage;
    myblock.get_block_size = ide_uni_get_bytes_per_block;
    myblock.total_blocks = ide_uni_get_total_sectors;

    deviceid = devmgr_register((devmgr_generic*)&myblock);
    
    ide_drivelist[1 + index].atapi = 0;
    ide_drivelist[1 + index].mydeviceid = deviceid;
    ide_drivelist[1 + index].read_only = 1;
    ide_drivelist[1 + index].dev = 1;
    ide_drivelist[1 + index].interface = type;
    ide_interpret_config(buffer,&ide_drivelist[1+index]);
    //register the device driver and determine partitions on that disk
    ide_registerpartitions(deviceid);
    };
    
    if (cd1)
    {
    reg_pio_data_in_lba(0, CMD_IDENTIFY_DEVICE_PACKET,
               0, 0,0L,SYS_DATA_SEL,buffer,1L,0);
    
    //make it visible to the device manager
    memset(&myblock,0,sizeof(myblock));
    sprintf(myblock.hdr.name,"cd%s0",interface_str);
    sprintf(myblock.hdr.description,"ATA %s master CD/DVD-ROM (READ-ONLY)",interface_str);
    myblock.hdr.type = DEVMGR_BLOCK;
    myblock.hdr.size = sizeof(myblock);
    
    //done with the preliminaries, now register public functions
    myblock.read_block = ide_uni_read_block;
    myblock.hdr.sendmessage = ide_sendmessage;
    
    myblock.get_block_size = ide_cdromgetblocksize;
    
    ide_drivelist[index].atapi = 1;
    ide_drivelist[index].mydeviceid = devmgr_register((devmgr_generic*)&myblock);
    ide_drivelist[index].read_only = 1;
    ide_drivelist[index].dev = 0;
    ide_drivelist[index].interface = type;
    ide_interpret_config(buffer,&ide_drivelist[index]);
   
    };
    
    if (cd2)
    {
    reg_pio_data_in_lba(1, CMD_IDENTIFY_DEVICE_PACKET,
               0, 0,0L,SYS_DATA_SEL,buffer,1L,0);
    
    //make it visible to the device manager
    memset(&myblock,0,sizeof(myblock));
    sprintf(myblock.hdr.name,"cd%s1",interface_str);
    sprintf(myblock.hdr.description,"ATA %s slave CD/DVD-ROM (READ-ONLY)",interface_str);
    myblock.hdr.type = DEVMGR_BLOCK;
    myblock.hdr.size = sizeof(myblock);
    
    //done with the preliminaries, now register public functions
    myblock.read_block = ide_uni_read_block;
    myblock.hdr.sendmessage = ide_sendmessage;
    myblock.get_block_size =  ide_cdromgetblocksize;
    
    ide_drivelist[ 1 + index].atapi = 1;
    ide_drivelist[ 1 + index].mydeviceid = devmgr_register((devmgr_generic*)&myblock);
    ide_drivelist[ 1 + index].read_only = 1;
    ide_drivelist[ 1 + index].dev = 1;
    ide_drivelist[ 1 + index].interface = type;
    ide_interpret_config(buffer,&ide_drivelist[1 + index]);
    //register the device driver
    };

    
    free(buffer);
};

void ide_init()
{
    int base;
    int intnum;
    int dev;
    int numDev;
    int i;
    
    base = 0x1f0;
    intnum = 14;
    dev = 0;
    
    for (i=0; i < 4 ; i++)
       memset(&ide_drivelist[i],0,sizeof(ide_drive_info));
    
    //setup primary IDE
    ide_setupinterface(0,14,"p");
    
    //Setup secondary IDE
    ide_setupinterface(1,15,"s");
};
