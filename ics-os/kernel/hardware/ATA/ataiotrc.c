//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOTRC.C
//
// by Hale Landis (hlandis@ibm.net)
//
// There is no copyright and there are no restrictions on the use
// of this ATA Low Level I/O Driver code.  It is distributed to
// help other programmers understand how the ATA device interface
// works and it is distributed without any warranty.  Use this
// code at your own risk.
//
// This code is based on the ATA-2, ATA-3 and ATA-4 standards and
// on interviews with various ATA controller and drive designers.
//
// This code has been run on many ATA (IDE) drives and
// MFM/RLL controllers.  This code may be a little
// more picky about the status it sees at various times.  A real
// BIOS probably would not check the status as carefully.
//
// Compile with one of the Borland C or C++ compilers.
//
// This C source contains the low level I/O trace functions.
//********************************************************************

//#include <stdio.h>
#include <string.h>
//#include <dos.h>

#include "ataio.h"

#include "ataiopd.h"

//**************************************************************

// trace dump buffer returned by trc_err_dump2()
// trc_cht_dump2() and trc_llt_dump2()

static unsigned char trcDmpBuf[100];

// buffer used to assemble print lines

static unsigned char prtBuf[64];

//**************************************************************

// command name lookup table and lookup function

static struct
{
   unsigned char cmdCode;
   unsigned char * cmdName;
} cmdNames[] =
   {
      0xC0 ,   "CFA ERASE SECTORS" ,
      0x03 ,   "CFA REQUEST EXT ERR CODE" ,
      0x87 ,   "CFA TRANSLATE SECTOR" ,
      0xCD ,   "CFA WRITE MULTIPLE WO ERASE" ,
      0x38 ,   "CFA WRITE SECTORS WO ERASE" ,
      0xE5 ,   "CHECK POWER MODE" ,
      0x98 ,   "CHECK POWER MODE" ,
      0x08 ,   "DEVICE RESET" ,
      0x90 ,   "EXECUTE DEVICE DIAGNOSTIC" ,
      0xE7 ,   "FLUSH CACHE" ,
      0x50 ,   "FORMAT TRACK" ,
      0xEC ,   "IDENTIFY DEVICE" ,
      0xA1 ,   "IDENTIFY PACKET DEVICE" ,
      0xE3 ,   "IDLE" ,
      0x97 ,   "IDLE" ,
      0xE1 ,   "IDLE IMMEDIATE" ,
      0x95 ,   "IDLE IMMEDIATE" ,
      0x91 ,   "INITIALIZE DEVICE PARAMETERS" ,
      0x00 ,   "NOP" ,
      0xA0 ,   "PACKET" ,
      0xE4 ,   "READ BUFFER" ,
      0xC7 ,   "READ DMA QUEUED" ,
      0xC8 ,   "READ DMA" ,
      0xC4 ,   "READ MULTIPLE" ,
      0x20 ,   "READ SECTORS" ,
      0x40 ,   "READ VERIFY SECTORS" ,
      0x10 ,   "RECALIBRATE" ,
      0x70 ,   "SEEK" ,
      0xEF ,   "SET FEATURES" ,
      0xC6 ,   "SET MULTIPLE MODE" ,
      0xE6 ,   "SLEEP" ,
      0x99 ,   "SLEEP" ,
      0xE2 ,   "STANDBY" ,
      0x96 ,   "STANDBY" ,
      0xE0 ,   "STANDBY IMMEDIATE" ,
      0x94 ,   "STANDBY IMMEDIATE" ,
      0xE8 ,   "WRITE BUFFER" ,
      0xCA ,   "WRITE DMA" ,
      0xCC ,   "WRITE DMA QUEUED" ,
      0xC5 ,   "WRITE MULTIPLE" ,
      0x30 ,   "WRITE SECTORS" ,
      0x3C ,   "WRITE VERIFY" ,
      0x00 ,   ""                   // end of table
   } ;

unsigned char * trc_get_cmd_name( unsigned char flg, unsigned char cc )

{
   int ndx = 0;

   if ( flg == TRC_FLAG_EMPTY )
      return "";
   if ( flg == TRC_FLAG_SRST )
      return "SOFT RESET";
   while ( * cmdNames[ndx].cmdName )
   {
      if ( cc == cmdNames[ndx].cmdCode )
         return cmdNames[ndx].cmdName;
      ndx ++ ;
   }
   return "";
}

//**************************************************************

// ATA status names lookup table and lookup function

static struct
{
   unsigned char bitPos;
   unsigned char * bitName;
} ataStatusNames[] =
   {
      0x80 , "BSY "  ,
      0x40 , "DRDY " ,
      0x20 , "DF "   ,
      0x10 , "DSC "  ,
      0x08 , "DRQ "  ,
      0x04 , "CORR " ,
      0x02 , "IDX "  ,
      0x01 , "ERR "
   } ;

static unsigned char ataStatusNameBuf[48];

unsigned char * trc_get_st_bit_name( unsigned char st )

{
   int ndx;

   if ( st & 0x80 )
      st = 0x80;
   * ataStatusNameBuf = 0;
   for ( ndx = 0; ndx < 8; ndx ++ )
   {
      if ( st & ataStatusNames[ndx].bitPos )
         strcat( ataStatusNameBuf, ataStatusNames[ndx].bitName );
   }
   return ataStatusNameBuf;
}

//**************************************************************

// ATA error names lookup table and lookup function

static struct
{
   unsigned char bitPos;
   unsigned char * bitName;
} ataErrorNames[] =
   {
      0x80 , "BBK:ICRC " ,
      0x40 , "UNC "      ,
      0x20 , "MC "       ,
      0x10 , "IDNF "     ,
      0x08 , "MCR "      ,
      0x04 , "ABRT "     ,
      0x02 , "TK0NF "    ,
      0x01 , "AMNF "
   } ;

static unsigned char ataErrorNameBuf[48];

unsigned char * trc_get_er_bit_name( unsigned char er )

{
   int ndx;

   * ataErrorNameBuf = 0;
   for ( ndx = 0; ndx < 8; ndx ++ )
   {
      if ( er & ataErrorNames[ndx].bitPos )
         strcat( ataErrorNameBuf, ataErrorNames[ndx].bitName );
   }
   return ataErrorNameBuf;
}

//**************************************************************

// error name lookup table and lookup function

static struct
{
   int errCode;
   unsigned char * errName;
} errNames[] =
   {
       1 ,  "Soft Reset timed out polling for device 0 to set BSY=0"  ,
       2 ,  "Soft Reset timed out polling device 1"                   ,
       3 ,  "Soft Reset timed out polling for device 1 to set BSY=0"  ,

      11 ,  "Device selection timed out polling for BSY=0"            ,
      12 ,  "Device selection timed out polling for RDY=1"            ,

      21 ,  "Non-Data command ended with bad status"                  ,
      22 ,  "Non-Data command timed out waiting for an interrupt"     ,
      23 ,  "Non-Data command timed out polling for BSY=0"            ,
      24 ,  "Exec Dev Diag command timed out polling device 1"        ,

      31 ,  "PIO Data In command terminated by error status"          ,
      32 ,  "Device should be ready to transfer data but DRQ=0"       ,
      33 ,  "PIO Data In command ended with bad status"               ,
      34 ,  "PIO Data In command timed out waiting for an interrupt"  ,
      35 ,  "PIO Data In command timed out polling for BSY=0"         ,

      41 ,  "PIO Data Out command terminated by error status"         ,
      42 ,  "Device should be ready to transfer data but DRQ=0"       ,
      43 ,  "PIO Data Out command ended with bad status"              ,
      44 ,  "PIO Data Out command timed out waiting for an interrupt" ,
      45 ,  "PIO Data Out command timed out polling for BSY=0"        ,
      46 ,  "Extra interrupt at start of a PIO Data Out command"      ,
      47 ,  "PIO Data Out command timed out polling for BSY=0"        ,

      51 ,  "Timeout waiting for BSY=0/DRQ=1 for cmd packet transfer" ,
      52 ,  "Bad status at command packet transfer time"              ,
      53 ,  "Timeout waiting for interrupt for data packet transfer"  ,
      54 ,  "Timeout polling for BSY=0/DRQ=1 for a data packet"       ,
      55 ,  "Bad status at data packet transfer time"                 ,
      56 ,  "Timout waiting for final interrupt at end of command"    ,
      57 ,  "Timeout polling for final BSY=0 at end of command"       ,
      58 ,  "Bad status at end of command"                            ,
      59 ,  "Buffer overrun (host buffer too small)"                  ,
      60 ,  "Byte count for data packet is zero"                      ,

      70 ,  "No DMA channel is setup"                                 ,
      71 ,  "End of command without complete data transfer"           ,
      72 ,  "Timeout waiting for 1st transfer to complete"            ,
      73 ,  "Timeout waiting for command to complete"                 ,
      74 ,  "Bad status at end of command"                            ,
      75 ,  "Timeout waiting for BSY=0/DRQ=1 for cmd packet transfer" ,
      76 ,  "Bad status at command packet transfer time"              ,
      77 ,  "ATA command code is not C8H or CAH"                      ,

      80 ,  "No tag available now"                                    ,
      81 ,  "Timeout polling for SERV=1"                              ,

      0  ,  ""                      // end of table
   } ;

unsigned char * trc_get_err_name( int ec )

{
   int ndx = 0;

   while ( * errNames[ndx].errName )
   {
      if ( ec == errNames[ndx].errCode )
         return errNames[ndx].errName;
      ndx ++ ;
   }
   return "";
}

//**************************************************************

static struct
{
   unsigned int pErrCode;
   unsigned char * pErrName;
} pErrNames[] =
   {
      FAILBIT0  , "slow setting BSY=1 or DRQ=1 after A0 cmd"   ,
      FAILBIT1  , "got interrupt before cmd packet xfer"       ,
      FAILBIT2  , "SC wrong at cmd packet xfer time"           ,
      FAILBIT3  , "byte count wrong at cmd packet xfer time"   ,
      FAILBIT4  , "SC (CD bit) wrong at data packet xfer time" ,
      FAILBIT5  , "SC (IO bit) wrong at data packet xfer time" ,
      FAILBIT6  , "byte count wrong at data packet xfer time"  ,
      FAILBIT7  , "byte count odd at data packet xfer time"    ,
      FAILBIT8  , "SC (CD and IO bits) wrong at end of cmd"    ,
      FAILBIT9  , "fail bit 9"                                 ,
      FAILBIT10 , "fail bit 10"                                ,
      FAILBIT11 , "fail bit 11"                                ,
      FAILBIT12 , "fail bit 12"                                ,
      FAILBIT13 , "fail bit 13"                                ,
      FAILBIT14 , "fail bit 14"                                ,
      FAILBIT15 , "extra interrupts detected"
   } ;

//**************************************************************

// command or reset error display data

static int errDmpLine = 0;
static int errDmpLine2 = 0;

//**************************************************************

// start the display of a command or reset error display

void trc_err_dump1( void )

{

   errDmpLine = 1;
   errDmpLine2 = 0;
}

//**************************************************************

// return one line of a command or reset error display,
// returns NULL at end

unsigned char * trc_err_dump2( void )

{
   unsigned long lba1, lba2;

   if ( reg_cmd_info.flg == TRC_FLAG_EMPTY )
      return NULL;
   if ( errDmpLine == 1 )
   {
      errDmpLine = 2;
      if ( reg_cmd_info.flg == TRC_FLAG_SRST )
         sprintf( trcDmpBuf, "ATA Reset: SR = %s ",
                             trc_get_cmd_name( TRC_FLAG_SRST, 0 ) );
      else
      if ( reg_cmd_info.flg == TRC_FLAG_ATAPI )
         sprintf( trcDmpBuf, "PACKET Command: %02X = %s ",
                             reg_cmd_info.cmd,
                             trc_get_cmd_name( TRC_FLAG_ATA, reg_cmd_info.cmd ) );
      else
         sprintf( trcDmpBuf, "ATA Command: %02X = %s ",
                             reg_cmd_info.cmd,
                             trc_get_cmd_name( TRC_FLAG_ATA, reg_cmd_info.cmd ) );

      return trcDmpBuf;
   }
   if ( errDmpLine == 2 )
   {
      errDmpLine = 3;
      if ( reg_cmd_info.flg == TRC_FLAG_ATA )
      {
         if ( reg_cmd_info.dh1 & 0x40 )
         {
            // LBA28 before and after
            lba1 = reg_cmd_info.dh1 & 0x0f;
            lba1 = lba1 << 8;
            lba1 = lba1 | reg_cmd_info.ch1;
            lba1 = lba1 << 8;
            lba1 = lba1 | reg_cmd_info.cl1;
            lba1 = lba1 << 8;
            lba1 = lba1 | reg_cmd_info.sn1;
            lba2 = reg_cmd_info.dh2 & 0x0f;
            lba2 = lba2 << 8;
            lba2 = lba2 | reg_cmd_info.ch2;
            lba2 = lba2 << 8;
            lba2 = lba2 | reg_cmd_info.cl2;
            lba2 = lba2 << 8;
            lba2 = lba2 | reg_cmd_info.sn2;
            sprintf( trcDmpBuf, "      LBA28: before %lu=%lXH, after %lu=%lXH",
                                 lba1, lba1, lba2, lba2 );
         }
         else
         {
            // CHS before and after
            sprintf( trcDmpBuf, "      CHS: before %5u %2u %3u, after %5u %2u %3u ",
                     (int) ( reg_cmd_info.ch1 << 8 ) | reg_cmd_info.cl1,
                     (int) reg_cmd_info.dh1 & 0x0f,
                     (int) reg_cmd_info.sn1,
                     (int) ( reg_cmd_info.ch2 << 8 ) | reg_cmd_info.cl2,
                     (int) reg_cmd_info.dh2 & 0x0f,
                     (int) reg_cmd_info.sn2 );
         }
         return trcDmpBuf;
      }
      if ( reg_cmd_info.flg == TRC_FLAG_ATAPI )
      {
         if ( reg_atapi_cp_size == 12 )
         {
            sprintf( trcDmpBuf, "      CDB=%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X ",
                     reg_atapi_cp_data[0], reg_atapi_cp_data[1],
                     reg_atapi_cp_data[2], reg_atapi_cp_data[3],
                     reg_atapi_cp_data[4], reg_atapi_cp_data[5],
                     reg_atapi_cp_data[6], reg_atapi_cp_data[7],
                     reg_atapi_cp_data[8], reg_atapi_cp_data[9],
                     reg_atapi_cp_data[10], reg_atapi_cp_data[11] );
         }
         else
         {
            sprintf( trcDmpBuf, "      CDB=%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X ",
                     reg_atapi_cp_data[0], reg_atapi_cp_data[1],
                     reg_atapi_cp_data[2], reg_atapi_cp_data[3],
                     reg_atapi_cp_data[4], reg_atapi_cp_data[5],
                     reg_atapi_cp_data[6], reg_atapi_cp_data[7],
                     reg_atapi_cp_data[8], reg_atapi_cp_data[9],
                     reg_atapi_cp_data[10], reg_atapi_cp_data[11],
                     reg_atapi_cp_data[12], reg_atapi_cp_data[13],
                     reg_atapi_cp_data[14], reg_atapi_cp_data[15] );
         }
         return trcDmpBuf;
      }
   }
   if ( errDmpLine == 3 )
   {
      errDmpLine = 4;
      sprintf( trcDmpBuf, "Driver ErrCode: %d %s ",
                          reg_cmd_info.ec, trc_get_err_name( reg_cmd_info.ec ) );
      return trcDmpBuf;
   }
   if ( errDmpLine == 4 )
   {
      errDmpLine = 5;
      if ( reg_cmd_info.to )
      {
         sprintf( trcDmpBuf, "                   "
                             "Driver timed out (see low level trace for details) !" );
         return trcDmpBuf;
      }
   }
   if ( errDmpLine == 5 )
   {
      errDmpLine = 6;
      sprintf( trcDmpBuf, "Bytes transferred: %ld (%lXH); DRQ packets: %ld (%lXH) ",
                        reg_cmd_info.totalBytesXfer, reg_cmd_info.totalBytesXfer,
                        reg_cmd_info.drqPackets, reg_cmd_info.drqPackets );
      return trcDmpBuf;
   }
   if ( errDmpLine == 6 )
   {
      errDmpLine = 7;
      sprintf( trcDmpBuf, "Device Status: %02X = %s ", reg_cmd_info.st2,
                        trc_get_st_bit_name( reg_cmd_info.st2 ) );
      return trcDmpBuf;
   }
   if ( errDmpLine == 7 )
   {
      errDmpLine = 8;
      sprintf( trcDmpBuf, "Device  Error: %02X = %s ", reg_cmd_info.er2,
                         trc_get_er_bit_name( reg_cmd_info.er2 ) );
      return trcDmpBuf;
   }
   if ( errDmpLine == 8 )
   {
      errDmpLine = 9;
      sprintf( trcDmpBuf, "ATA Intf Regs: FR  ER  SC  SN  CL  CH  DH  CM  ST  AS  DC " );
      return trcDmpBuf;
   }
   if ( errDmpLine == 9 )
   {
      errDmpLine = 10;
      if ( reg_cmd_info.flg == TRC_FLAG_SRST )
         sprintf( trcDmpBuf, "   Cmd Params: "
                  // fr  er  sc  sn  cl  ch  dh  cm  st  as  dc
                    "--  --  --  --  --  --  --  --  --  --  04 " );
      else
         sprintf( trcDmpBuf, "   Cmd Params: "
                  //  fr   er   sc    sn    cl    ch    dh    cm   st  as   dc
                    "%02X  --  %02X  %02X  %02X  %02X  %02X  %02X  --  --  %02X ",
                     reg_cmd_info.fr1, reg_cmd_info.sc1, reg_cmd_info.sn1,
                     reg_cmd_info.cl1, reg_cmd_info.ch1, reg_cmd_info.dh1,
                     reg_cmd_info.cmd, reg_cmd_info.dc1 );
      return trcDmpBuf;
   }
   if ( errDmpLine == 10 )
   {
      errDmpLine = 11;
      sprintf( trcDmpBuf, "    After Cmd: "
                  // fr   er    sc    sn    cl    ch    dh   cm   st    as   dc
                    "--  %02X  %02X  %02X  %02X  %02X  %02X  --  %02X  %02X  -- ",
                     reg_cmd_info.er2, reg_cmd_info.sc2, reg_cmd_info.sn2,
                     reg_cmd_info.cl2, reg_cmd_info.ch2, reg_cmd_info.dh2,
                     reg_cmd_info.st2, reg_cmd_info.as2 );
      return trcDmpBuf;
   }
   if ( ( errDmpLine == 11 ) &&  reg_cmd_info.failbits )
   {
      errDmpLine = 12;
      errDmpLine2 = 0;
      sprintf( trcDmpBuf, "  ATA/ATAPI protocol errors bits (%04XH):",
                          reg_cmd_info.failbits );
      return trcDmpBuf;
   }
   if ( errDmpLine == 12 )
   {
      while ( ( errDmpLine2 < 16 )
              &&
              ( ! ( reg_cmd_info.failbits & pErrNames[errDmpLine2].pErrCode ) )
            )
         errDmpLine2 ++ ;
      if ( errDmpLine2 < 16 )
      {
         sprintf( trcDmpBuf, "      - %s", pErrNames[errDmpLine2].pErrName );
         errDmpLine2 ++ ;
         return trcDmpBuf;
      }
   }
   return NULL;
}

//**********************************************************

// command types to trace, see TRC_TYPE_xxx in ataio.h and
// see trc_cht_types() below.

static unsigned int chtTypes = 0xffff; // default is trace all cmd types

// command history trace buffer

#define MAX_CHT 100

static struct
{
   // trace entry type, flag and command code
   unsigned char flg;         // see TRC_FLAG_xxx in ataio.h
   unsigned char ct;          // see TRC_TYPE_xxx in ataio.h
   unsigned char cmd;         // command code
   // before regs
   unsigned char fr1;         // feature
   unsigned char sc1;         // sec cnt
   unsigned char sn1;         // sec num
   unsigned char cl1;         // cyl low
   unsigned char ch1;         // cyl high
   unsigned char dh1;         // device head
   unsigned char dc1;         // device control
   // driver error codes
   unsigned char ec;          // detailed error code
   unsigned char to;          // not zero if time out error
   // after regs
   unsigned char st2;         // status reg
   unsigned char as2;         // alt status reg
   unsigned char er2;         // error reg
   unsigned char sc2;         // sec cnt
   unsigned char sn2;         // sec num
   unsigned char cl2;         // cyl low
   unsigned char ch2;         // cyl high
   unsigned char dh2;         // device head
   // CDB size
   unsigned char cdbSize;     // packet cdb size
   // packet CDB (16 bytes)
   // or 48-bit LBA before (8 bytes) and after (8 bytes)
   unsigned char cdb[16];        // CDB data (or LBA48 before/after)
   #define LBA_BL ( cdb + 0  )   // location of 48-bit LBA before low
   #define LBA_BH ( cdb + 4  )   // location of 48-bit LBA before high
   #define LBA_AL ( cdb + 8  )   // location of 48-bit LBA after low
   #define LBA_AH ( cdb + 12 )   // location of 48-bit LBA after high
}  chtBuf[MAX_CHT];

static int chtCur = 0;
static int chtDmpLine = 0;
static int chtDmpNdx = 0;

//**************************************************************

// set the commands types that are traced,
// see TRC_TYPE_xxx in ataio.h and chtTypes above.

void trc_cht_types( int type )

{

   if ( type < 1 )
      chtTypes = 0x0000;   // trace nothing
   else
      if ( type > 15 )
         chtTypes = 0xffff;   // trace all
      else
         chtTypes |= 0x0001 << type;  // selective
}

//**************************************************************

// place an command or reset entry into
// the command history trace buffer

void trc_cht( void )

{
   int ndx;

   if ( ! ( ( 0x0001 << reg_cmd_info.ct ) & chtTypes ) )
      return;
   // trace entry type, flag and command code
   chtBuf[chtCur].flg = reg_cmd_info.flg;
   chtBuf[chtCur].ct  = reg_cmd_info.ct ;
   chtBuf[chtCur].cmd = reg_cmd_info.cmd;
   // regs before
   chtBuf[chtCur].fr1 = reg_cmd_info.fr1;
   chtBuf[chtCur].sc1 = reg_cmd_info.sc1;
   chtBuf[chtCur].sn1 = reg_cmd_info.sn1;
   chtBuf[chtCur].cl1 = reg_cmd_info.cl1;
   chtBuf[chtCur].ch1 = reg_cmd_info.ch1;
   chtBuf[chtCur].dh1 = reg_cmd_info.dh1;
   chtBuf[chtCur].dc1 = reg_cmd_info.dc1;
   // driver error codes
   chtBuf[chtCur].ec  = reg_cmd_info.ec ;
   chtBuf[chtCur].to  = reg_cmd_info.to ;
   // regs after
   chtBuf[chtCur].st2 = reg_cmd_info.st2;
   chtBuf[chtCur].as2 = reg_cmd_info.as2;
   chtBuf[chtCur].er2 = reg_cmd_info.er2;
   chtBuf[chtCur].sc2 = reg_cmd_info.sc2;
   chtBuf[chtCur].sn2 = reg_cmd_info.sn2;
   chtBuf[chtCur].cl2 = reg_cmd_info.cl2;
   chtBuf[chtCur].ch2 = reg_cmd_info.ch2;
   chtBuf[chtCur].dh2 = reg_cmd_info.dh2;
   // packet cdb
   if ( reg_cmd_info.cmd == 0xa0 )
   {
      chtBuf[chtCur].cdbSize = reg_atapi_cp_size;
      for ( ndx = 0; ndx < reg_atapi_cp_size; ndx ++ )
         chtBuf[chtCur].cdb[ndx] = reg_atapi_cp_data[ndx];
   }
   // ??? LBA48 before and after here
   chtCur ++ ;
   if ( chtCur >= MAX_CHT )
      chtCur = 0;
}

//**************************************************************

// clear the command history trace buffer

void trc_cht_dump0( void )

{

   for ( chtCur = 0; chtCur < MAX_CHT; chtCur ++ )
      chtBuf[chtCur].flg = TRC_FLAG_EMPTY;
   chtCur = 0;
}

//**************************************************************

// start a dump of the command history trace buffer

void trc_cht_dump1( void )

{

   chtDmpLine = 1;
   chtDmpNdx = chtCur + 1;
   if ( chtDmpNdx >= MAX_CHT )
      chtDmpNdx = 0;
}

//**************************************************************

// return one line of the command history trace buffer,
// returns NULL at end.

unsigned char * trc_cht_dump2( void )

{
   unsigned long lba1, lba2;

   if ( chtDmpLine == 1 )     // 1st line is 1st line of heading
   {
                        //0        1         2         3         4         5         6
                        //123456789012345678901234567890123456789012345678901234567890
      strcpy( trcDmpBuf, "<-- registers before -->  <-- registers after -->  <--->" );
      chtDmpLine = 2;
      return trcDmpBuf;
   }
   if ( chtDmpLine == 2 )     // 2nd line is 2nd line of heading
   {
                        //0        1         2         3         4         5         6
                        //123456789012345678901234567890123456789012345678901234567890
      strcpy( trcDmpBuf, "CM  FR SC SN CL CH DH DC  ST AS ER SC SN CL CH DH  EC TO" );
      chtDmpLine = 4;
      return trcDmpBuf;
   }
   // search for oldest entry
   while ( 1 )
   {
      if ( chtDmpNdx == chtCur )
         return NULL;
      if ( chtBuf[chtDmpNdx].flg != TRC_FLAG_EMPTY )
         break;
      chtDmpNdx ++ ;
      if ( chtDmpNdx >= MAX_CHT )
         chtDmpNdx = 0;
   }
   // display entry (may require more than one pass through here)
   if ( chtDmpLine == 4 )
   {
      if ( chtBuf[chtDmpNdx].flg == TRC_FLAG_SRST )
      {
         sprintf( trcDmpBuf,
                  "SR  -- -- -- -- -- -- --  "
                  "%02X %02X %02X %02X %02X %02X %02X %02X  "
                  "%2d %2d",
                  chtBuf[chtDmpNdx].st2, chtBuf[chtDmpNdx].as2,
                  chtBuf[chtDmpNdx].er2, chtBuf[chtDmpNdx].sc2,
                  chtBuf[chtDmpNdx].sn2, chtBuf[chtDmpNdx].cl2,
                  chtBuf[chtDmpNdx].ch2, chtBuf[chtDmpNdx].dh2,
                  chtBuf[chtDmpNdx].ec, chtBuf[chtDmpNdx].to );
         chtDmpLine = 4;   // done with this entry
         chtDmpNdx ++ ;
         if ( chtDmpNdx >= MAX_CHT )
            chtDmpNdx = 0;
         return trcDmpBuf;
      }
      else
      {
         sprintf( trcDmpBuf,
                  "%02X  %02X %02X %02X %02X %02X %02X %02X  "
                  "%02X %02X %02X %02X %02X %02X %02X %02X  "
                  "%2d %2d",
                  chtBuf[chtDmpNdx].cmd, chtBuf[chtDmpNdx].fr1,
                  chtBuf[chtDmpNdx].sc1, chtBuf[chtDmpNdx].sn1,
                  chtBuf[chtDmpNdx].cl1, chtBuf[chtDmpNdx].ch1,
                  chtBuf[chtDmpNdx].dh1, chtBuf[chtDmpNdx].dc1,
                  chtBuf[chtDmpNdx].st2, chtBuf[chtDmpNdx].as2,
                  chtBuf[chtDmpNdx].er2, chtBuf[chtDmpNdx].sc2,
                  chtBuf[chtDmpNdx].sn2, chtBuf[chtDmpNdx].cl2,
                  chtBuf[chtDmpNdx].ch2, chtBuf[chtDmpNdx].dh2,
                  chtBuf[chtDmpNdx].ec, chtBuf[chtDmpNdx].to );
         chtDmpLine = 5;   // do 2nd line of this entry next
         return trcDmpBuf;
      }
   }
   else
   {
      // chtDmpLine == 5, second line of command entry trace data
      if ( chtBuf[chtDmpNdx].flg == TRC_FLAG_ATA )
      {
         if ( chtBuf[chtDmpNdx].dh1 & 0x40 )
         {
            // LBA28 before and after
            lba1 = chtBuf[chtDmpNdx].dh1 & 0x0f;
            lba1 = lba1 << 8;
            lba1 = lba1 | chtBuf[chtDmpNdx].ch1;
            lba1 = lba1 << 8;
            lba1 = lba1 | chtBuf[chtDmpNdx].cl1;
            lba1 = lba1 << 8;
            lba1 = lba1 | chtBuf[chtDmpNdx].sn1;
            lba2 = chtBuf[chtDmpNdx].dh2 & 0x0f;
            lba2 = lba2 << 8;
            lba2 = lba2 | chtBuf[chtDmpNdx].ch2;
            lba2 = lba2 << 8;
            lba2 = lba2 | chtBuf[chtDmpNdx].cl2;
            lba2 = lba2 << 8;
            lba2 = lba2 | chtBuf[chtDmpNdx].sn2;
            sprintf( trcDmpBuf, "      LBA28: before %lu=%lXH, after %lu=%lXH",
                                 lba1, lba1, lba2, lba2 );
         }
         else
         {
            // CHS before and after
            sprintf( trcDmpBuf, "      CHS: before %5u %2u %3u, after %5u %2u %3u ",
               (int) ( chtBuf[chtDmpNdx].ch1 << 8 ) | chtBuf[chtDmpNdx].cl1,
               (int) chtBuf[chtDmpNdx].dh1 & 0x0f,
               (int) chtBuf[chtDmpNdx].sn1,
               (int) ( chtBuf[chtDmpNdx].ch2 << 8 ) | chtBuf[chtDmpNdx].cl2,
               (int) chtBuf[chtDmpNdx].dh2 & 0x0f,
               (int) chtBuf[chtDmpNdx].sn2 );
         }
      }
      if ( chtBuf[chtDmpNdx].flg == TRC_FLAG_ATAPI )
      {
         if ( chtBuf[chtDmpNdx].cmd == 12 )
            sprintf( trcDmpBuf, "      CDB=%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X ",
               chtBuf[chtDmpNdx].cdb[0], chtBuf[chtDmpNdx].cdb[1],
               chtBuf[chtDmpNdx].cdb[2], chtBuf[chtDmpNdx].cdb[3],
               chtBuf[chtDmpNdx].cdb[4], chtBuf[chtDmpNdx].cdb[5],
               chtBuf[chtDmpNdx].cdb[6], chtBuf[chtDmpNdx].cdb[7],
               chtBuf[chtDmpNdx].cdb[8], chtBuf[chtDmpNdx].cdb[9],
               chtBuf[chtDmpNdx].cdb[10], chtBuf[chtDmpNdx].cdb[11] );
         else
            sprintf( trcDmpBuf, "      CDB=%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X  "
                                          "%02X %02X %02X %02X ",
               chtBuf[chtDmpNdx].cdb[0], chtBuf[chtDmpNdx].cdb[1],
               chtBuf[chtDmpNdx].cdb[2], chtBuf[chtDmpNdx].cdb[3],
               chtBuf[chtDmpNdx].cdb[4], chtBuf[chtDmpNdx].cdb[5],
               chtBuf[chtDmpNdx].cdb[6], chtBuf[chtDmpNdx].cdb[7],
               chtBuf[chtDmpNdx].cdb[8], chtBuf[chtDmpNdx].cdb[9],
               chtBuf[chtDmpNdx].cdb[10], chtBuf[chtDmpNdx].cdb[11],
               chtBuf[chtDmpNdx].cdb[12], chtBuf[chtDmpNdx].cdb[13],
               chtBuf[chtDmpNdx].cdb[14], chtBuf[chtDmpNdx].cdb[15] );
      }
   }
   chtDmpLine = 4;   // done with this entry
   chtDmpNdx ++ ;
   if ( chtDmpNdx >= MAX_CHT )
      chtDmpNdx = 0;
   return trcDmpBuf;
}

//**********************************************************

// Low-level trace buffer

#define MAX_LLT 500

static struct
{
   unsigned char addr;
   unsigned char data;
   unsigned char type;
   unsigned char rep;
} lltBuf[MAX_LLT];

static int lltCur = 0;
static int lltDmpLine = 0;
static int lltDmpNdx = 0;

static unsigned char lltAddr = 0;
static unsigned char lltData = 0;
static unsigned char lltType = 0;
static unsigned long lltRep = 0L;

static struct
{
   unsigned char typeId;      // trace entry type
   unsigned char * typeNm;    // trace entry name
} type_nm[]
   =
   {
                     //<opr> <--register---> <data - note>
      TRC_LLT_INB   , "INB   " ,
      TRC_LLT_OUTB  , "OUTB  " ,
      TRC_LLT_INW   , "INW   " ,
      TRC_LLT_OUTW  , "OUTW  " ,
      TRC_LLT_INSB  , "INSB  " ,
      TRC_LLT_OUTSB , "OUTSB " ,
      TRC_LLT_INSW  , "INSW  " ,
      TRC_LLT_OUTSW , "OUTSW " ,
      TRC_LLT_INSD  , "INSD  " ,
      TRC_LLT_OUTSD , "OUTSD " ,
      TRC_LLT_S_CFG , "===== Start Dev Cnfg  ",
      TRC_LLT_S_RST , "===== Start Reset     ",
      TRC_LLT_S_ND  , "===== Start ND cmd    ",
      TRC_LLT_S_PDI , "===== Start PDI cmd   ",
      TRC_LLT_S_PDO , "===== Start PDO cmd   ",
      TRC_LLT_S_PI  , "===== Start PI cmd    ",
      TRC_LLT_S_RWD , "===== Start R/W DMA   ",
      TRC_LLT_S_PID , "===== Start PI DMA    ",
      TRC_LLT_WINT  , "..... Wait for INTRQ  ",
      TRC_LLT_PNBSY , "..... Poll for BSY=0  ",
      TRC_LLT_PRDY  , "..... Poll for DRDY=1 ",
      TRC_LLT_TOUT  , "..... T I M E O U T   ",
      TRC_LLT_ERROR , "..... E R R O R       ",
      TRC_LLT_P_CMD , "..... ATAPI Cmd Code  ",
      TRC_LLT_DELAY , "..... DELAY ~100ms    ",
      TRC_LLT_DEBUG , "..... DEBUG           ",
      TRC_LLT_DELAY2, "..... DELAY 0-55ms    ",
      TRC_LLT_E_CFG , "===== End Dev Cnfg    ",
      TRC_LLT_E_RST , "===== End Reset       ",
      TRC_LLT_E_ND  , "===== End ND cmd      ",
      TRC_LLT_E_PDI , "===== End PDI cmd     ",
      TRC_LLT_E_PDO , "===== End PDO cmd     ",
      TRC_LLT_E_PI  , "===== End PI cmd      ",
      TRC_LLT_E_RWD , "===== End R/W DMA     ",
      TRC_LLT_E_PID , "===== End PI DMA      ",
      TRC_LLT_DMA1  , "..... Enable DMA Ch   ",
      TRC_LLT_DMA2  , "..... Poll DMA TC bit ",
      TRC_LLT_DMA3  , "..... Disable DMA Ch  ",
      TRC_LLT_PSERV , "..... Poll for SERV=1 ",
      0             , "????? " ,
   } ;

static unsigned char * reg_nm[]  // register names for trace
   =
   {
  //<opr> <--register---> <data - note>
         "Data            " , // 0 data reg
         "Error/Feature   " , // 1 error & feature
         "SectorCount     " , // 2 sector count
         "SectorNumber    " , // 3 sector number
         "CylinderLow     " , // 4 cylinder low
         "CylinderHigh    " , // 5 cylinder high
         "DeviceHead      " , // 6 device head
         "Status/Cmd      " , // 7 primary status & command
         "AltStat/DevCtrl " , // 8 alternate status & device control
         "DevAddr         " , // 9 device address
   } ;

//*********************************************************

// place an entry into the low level trace buffer

void trc_llt( unsigned char addr,
              unsigned char data,
              unsigned char type )

{

   if ( ( addr == lltAddr )
        &&
        ( data == lltData )
        &&
        ( type == lltType )
      )
   {
      lltRep ++ ;
      return;
   }
   lltBuf[lltCur].addr = lltAddr;
   lltBuf[lltCur].data = lltData;
   lltBuf[lltCur].type = lltType;
   lltBuf[lltCur].rep = lltRep > 255L ? 255 : lltRep;
   lltRep = 0L;
   lltCur ++ ;
   if ( lltCur >= MAX_LLT )
      lltCur = 0;
   lltBuf[lltCur].addr = lltAddr = addr;
   lltBuf[lltCur].data = lltData = data;
   lltBuf[lltCur].type = lltType = type;
   lltBuf[lltCur].rep  = 0;
}

//**************************************************************

// clear the low level trace buffer

void trc_llt_dump0( void )

{

   for ( lltCur = 0; lltCur < MAX_LLT; lltCur ++ )
      lltBuf[lltCur].type = 0;
   lltCur = 0;
}

//**************************************************************

// start a dump of the low level trace buffer

void trc_llt_dump1( void )

{

   trc_llt( 0, 0, TRC_LLT_NONE );
   lltDmpLine = 0;
   lltDmpNdx = lltCur + 1;
   if ( lltDmpNdx >= MAX_LLT )
      lltDmpNdx = 0;
}

//**************************************************************

// return one line of the low level trace,
// returns NULL at end.

unsigned char * trc_llt_dump2( void )

{
   int ndx;

   lltDmpLine ++ ;
   if ( lltDmpLine == 1 )     // 1st line is heading
   {
                        //0        1         2         3
                        //123456789012345678901234567890123456
      strcpy( trcDmpBuf, "<opr> <--register---> <data - note>" );
      return trcDmpBuf;
   }
   while ( 1 )
   {
      if ( lltDmpNdx == lltCur )
         return NULL;
      if ( lltBuf[lltDmpNdx].type != 0 )
         break;
      lltDmpNdx ++ ;
      if ( lltDmpNdx >= MAX_LLT )
         lltDmpNdx = 0;
   }
   ndx = 0;
   while ( type_nm[ndx].typeId )
   {
      if ( lltBuf[lltDmpNdx].type == type_nm[ndx].typeId )
         break;
      ndx ++ ;
   }
   strcpy( trcDmpBuf, type_nm[ ndx ].typeNm );
   if ( lltBuf[lltDmpNdx].type < TRC_LLT_S_CFG )
   {
      strcat( trcDmpBuf, reg_nm[ lltBuf[lltDmpNdx].addr ] );
      if ( lltBuf[lltDmpNdx].addr == CB_DATA )
         strcpy( prtBuf, "-- " );
      else
         sprintf( prtBuf, "%02X ", lltBuf[lltDmpNdx].data );
      strcat( trcDmpBuf, prtBuf );
      if (    ( lltBuf[lltDmpNdx].addr == CB_DC )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_OUTB )
         )
      {
         if ( lltBuf[lltDmpNdx].data & CB_DC_SRST )
         {
            strcat( trcDmpBuf, "START COMMAND: " );
            strcat( trcDmpBuf, trc_get_cmd_name( TRC_FLAG_SRST, 0 ) );
         }
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & CB_DC_NIEN )
                       ? " nIEN=1" : " nIEN=0" );
      }
      if (    ( lltBuf[lltDmpNdx].addr == CB_CMD )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_OUTB )
         )
      {
         strcat( trcDmpBuf, "START COMMAND: " );
         strcat( trcDmpBuf, trc_get_cmd_name( TRC_FLAG_ATA, lltBuf[lltDmpNdx].data ) );
      }
      if (    ( lltBuf[lltDmpNdx].addr == CB_DH )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_OUTB )
         )
      {
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x10 )
                       ? "DEV=1" : "DEV=0" );
         strcat( trcDmpBuf, ( lltBuf[lltDmpNdx].data & 0x40 )
                       ? " LBA=1" : " LBA=0" );
      }
      if (    (    ( lltBuf[lltDmpNdx].addr == CB_STAT )
                || ( lltBuf[lltDmpNdx].addr == CB_ASTAT )
              )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_INB )
         )
         strcat( trcDmpBuf, trc_get_st_bit_name( lltBuf[lltDmpNdx].data ) );
      if (    ( lltBuf[lltDmpNdx].addr == CB_ERR )
           && ( lltBuf[lltDmpNdx].type == TRC_LLT_INB )
         )
         strcat( trcDmpBuf, trc_get_er_bit_name( lltBuf[lltDmpNdx].data ) );
   }
   if ( lltBuf[lltDmpNdx].type == TRC_LLT_ERROR )
      strcat( trcDmpBuf, trc_get_err_name( lltBuf[lltDmpNdx].data ) );
   if ( ( lltBuf[lltDmpNdx].type == TRC_LLT_DEBUG )
        ||
        ( lltBuf[lltDmpNdx].type == TRC_LLT_P_CMD )
      )
   {
      sprintf( prtBuf, "%02X ", lltBuf[lltDmpNdx].data );
      strcat( trcDmpBuf, prtBuf );
   }
   if ( lltBuf[lltDmpNdx].rep )
   {
      if ( lltBuf[lltDmpNdx].rep == 255 )
         strcpy( prtBuf, "repeated 255 or more times " );
      else
         sprintf( prtBuf, "repeated %u more times ", lltBuf[lltDmpNdx].rep );
      strcat( trcDmpBuf, prtBuf );
   }
   lltDmpNdx ++ ;
   if ( lltDmpNdx >= MAX_LLT )
      lltDmpNdx = 0;
   return trcDmpBuf;
}

// end ataiotrc.c
