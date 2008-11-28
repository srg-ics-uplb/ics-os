//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOPIO.C
//
// by Hale Landis (hlandis@ata-atapi.com)
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
// This module contains inline assembler code so you'll
// also need Borland's Assembler.
//
// This C source contains the low level I/O port IN/OUT functions.
//********************************************************************

//*************************************************************
//
// Host adapter base addresses.
//
//*************************************************************

int pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;


//*************************************************************
//
// Set the host adapter i/o base addresses.
//
//*************************************************************

void pio_set_iobase_addr( unsigned int base1, unsigned int base2 )

{

   pio_base_addr1 = base1;
   pio_base_addr2 = base2;
   pio_memory_seg = 0;
   pio_reg_addrs[ CB_DATA ] = pio_base_addr1 + 0;  // 0
   pio_reg_addrs[ CB_FR   ] = pio_base_addr1 + 1;  // 1
   pio_reg_addrs[ CB_SC   ] = pio_base_addr1 + 2;  // 2
   pio_reg_addrs[ CB_SN   ] = pio_base_addr1 + 3;  // 3
   pio_reg_addrs[ CB_CL   ] = pio_base_addr1 + 4;  // 4
   pio_reg_addrs[ CB_CH   ] = pio_base_addr1 + 5;  // 5
   pio_reg_addrs[ CB_DH   ] = pio_base_addr1 + 6;  // 6
   pio_reg_addrs[ CB_CMD  ] = pio_base_addr1 + 7;  // 7
   pio_reg_addrs[ CB_DC   ] = pio_base_addr2 + 6;  // 8
   pio_reg_addrs[ CB_DA   ] = pio_base_addr2 + 7;  // 9
}

//*************************************************************
//
// Set the host adapter memory base addresses.
//
//*************************************************************

void pio_set_memory_addr( unsigned int seg )

{

   pio_base_addr1 = 0;
   pio_base_addr2 = 8;
   pio_memory_seg = seg;
   pio_memory_dt_opt = PIO_MEMORY_DT_OPT0;
   pio_reg_addrs[ CB_DATA ] = pio_base_addr1 + 0;  // 0
   pio_reg_addrs[ CB_FR   ] = pio_base_addr1 + 1;  // 1
   pio_reg_addrs[ CB_SC   ] = pio_base_addr1 + 2;  // 2
   pio_reg_addrs[ CB_SN   ] = pio_base_addr1 + 3;  // 3
   pio_reg_addrs[ CB_CL   ] = pio_base_addr1 + 4;  // 4
   pio_reg_addrs[ CB_CH   ] = pio_base_addr1 + 5;  // 5
   pio_reg_addrs[ CB_DH   ] = pio_base_addr1 + 6;  // 6
   pio_reg_addrs[ CB_CMD  ] = pio_base_addr1 + 7;  // 7
   pio_reg_addrs[ CB_DC   ] = pio_base_addr2 + 6;  // 8
   pio_reg_addrs[ CB_DA   ] = pio_base_addr2 + 7;  // 9
}

//*************************************************************
//
// These functions do basic IN/OUT of byte and word values:
//
//    pio_inbyte()
//    pio_outbyte()
//    pio_inword()
//    pio_outword()
//
//*************************************************************

unsigned char pio_inbyte( unsigned int addr )

{
   unsigned int regAddr;
   unsigned char uc;
   unsigned char * ucp;

   regAddr = pio_reg_addrs[ addr ];
   uc = inportb(regAddr);
   pio_last_read[ addr ] = uc;
   return uc;
}

//*************************************************************

void pio_outbyte( unsigned int addr, unsigned char data )

{
   unsigned int regAddr;
   unsigned char  * ucp;

   regAddr = pio_reg_addrs[ addr ];
   outportb(regAddr,data);
   pio_last_write[ addr ] = data;
}

//*************************************************************

unsigned int pio_inword( unsigned int addr )

{
   unsigned int regAddr;
   unsigned int ui;
   unsigned int  * uip;

   regAddr = pio_reg_addrs[ addr ];
   ui = inportb(regAddr);
   return ui;
}

//*************************************************************
void pio_outword( unsigned int addr, unsigned int data )

{
   unsigned int regAddr;
   unsigned int  * uip;
   regAddr = pio_reg_addrs[ addr ];
   outportw(regAddr,data);
}

//*************************************************************
//
// These functions are normally used to transfer DRQ blocks:
//
// pio_drq_block_in()
// pio_drq_block_out()
//
//*************************************************************

// Note: pio_drq_block_in() is the primary way perform PIO
// Data In transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_drq_block_in( unsigned int addrDataReg,
                       unsigned int bufSeg, unsigned int bufOff,
                       long wordCnt )

{
   unsigned int dataRegAddr;
   volatile unsigned int  * uip1;
   unsigned int  * uip2;
   volatile unsigned char  * ucp1;
   unsigned char  * ucp2;
   long bCnt;
   int memDtOpt;
   unsigned int randVal;

   // Note: The maximum value of wordCnt is 65536
   // for an ATA or ATAPI PIO DRQ block transfer.

   // Data transfer using INS instruction.

      dataRegAddr = pio_reg_addrs[ addrDataReg ];

      if ( pio_xfer_width == 8 )
      {
         // do REP INS
         pio_rep_inbyte( addrDataReg, bufSeg, bufOff, wordCnt * 2L );
      }
      else
      if ( ( pio_xfer_width == 32 ) && ( ! ( wordCnt & 0x00000001L ) ) )
      {
         // do REP INSD
         pio_rep_indword( addrDataReg, bufSeg, bufOff, wordCnt / 2L );
      }
      else
      {
         // do REP INSW
         pio_rep_inword( addrDataReg, bufSeg, bufOff, wordCnt );
      }
   return;
}

//*************************************************************

// Note: pio_drq_block_out() is the primary way perform PIO
// Data Out transfers. It will handle 8-bit, 16-bit and 32-bit
// I/O based data transfers and 8-bit and 16-bit PCMCIA Memory
// mode transfers.

void pio_drq_block_out( unsigned int addrDataReg,
                        unsigned int bufSeg, unsigned int bufOff,
                        long wordCnt )

{
   unsigned int dataRegAddr;
   unsigned int  * uip1;
   unsigned int  * uip2;
   unsigned char  * ucp1;
   unsigned char  * ucp2;
   long bCnt;
   int memDtOpt;
   unsigned int randVal;

   // Note: The maximum value of wordCnt is 65536
   // for an ATA or ATAPI PIO DRQ block transfer.
  // Data transfer using OUTS instruction.

      dataRegAddr = pio_reg_addrs[ addrDataReg ];

      if ( pio_xfer_width == 8 )
      {
         // do REP OUTS
         pio_rep_outbyte( addrDataReg, bufSeg, bufOff, wordCnt * 2L );
      }
      else
      if ( ( pio_xfer_width == 32 ) && ( ! ( wordCnt & 0x00000001L ) ) )
      {
         // do REP OUTSD
         pio_rep_outdword( addrDataReg, bufSeg, bufOff, wordCnt / 2L );
      }
      else
      {
         // do REP OUTSW
         pio_rep_outword( addrDataReg, bufSeg, bufOff, wordCnt );
      }

   return;
}

//*************************************************************
//
// These functions do REP INS/OUTS data transfers
// (PIO data transfers in I/O mode):
//
// pio_rep_inbyte()
// pio_rep_outbyte()
// pio_rep_inword()
// pio_rep_outword()
// pio_rep_indword()
// pio_rep_outdword()
//
// These functions can be called directly but usually they
// are called by the pio_drq_block_in() and pio_drq_block_out()
// functions to perform I/O mode transfers. See the
// pio_xfer_width variable!
//
//*************************************************************

extern void reptrans(unsigned short int bufSeg, unsigned int bufOff,
                     unsigned int bCnt,unsigned short int dataRegAddr);

void pio_rep_inbyte( unsigned int addrDataReg,
                     unsigned int bufSeg, unsigned int bufOff,
                     long byteCnt )

{
   unsigned int dataRegAddr;
   unsigned int bCnt;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   // The maximum ATA DRQ block is 65536 bytes.
   // The maximun ATAPI DRQ block is 131072 bytes.
   // Because the REP INSB will fail if byteCnt
   // is greater than 65535 the transfer is split
   // into chunks of 16384 bytes or less.

   while ( byteCnt > 0L )
   {

      if ( byteCnt > 16384L )
         bCnt = 16384;
      else
         bCnt = (unsigned int) byteCnt;

      // READ THIS: If you get a compile error on the following
      // statement you are trying to use BASM (the assembler
      // built into Borland C. BASM can not assemble 386
      // instructions. You must use Borland TASM as is shown
      // in the EXAMPLE1.MAK or EXAMPLE2.MAK "make files".

      reptrans(bufSeg,bufOff,bCnt,dataRegAddr);
      byteCnt = byteCnt - (long) bCnt;
   }
}

//*************************************************************

extern void repoutbyte(unsigned short int bufSeg, unsigned int bufOff,
             unsigned int bCnt,unsigned short int dataRegAddr);

void pio_rep_outbyte( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      long byteCnt )

{
   unsigned int dataRegAddr;
   unsigned int bCnt;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   // The maximum ATA DRQ block is 65536 bytes.
   // The maximun ATAPI DRQ block is 131072 bytes.
   // Because the REP OUTSB will fail if byteCnt
   // is greater than 65535 the transfer is split
   // into chunks of 16384 bytes or less.

   while ( byteCnt > 0L )
   {

      if ( byteCnt > 16384L )
         bCnt = 16384;
      else
         bCnt = (unsigned int) byteCnt;
         
      repoutbyte(bufSeg,bufOff,bCnt,dataRegAddr);
      byteCnt = byteCnt - (long) bCnt;
   }
}

//*************************************************************

extern void repinword(unsigned short int bufSeg, unsigned int bufOff,
             unsigned int bCnt,unsigned short int dataRegAddr);

void pio_rep_inword( unsigned int addrDataReg,
                     unsigned int bufSeg, unsigned int bufOff,
                     unsigned int wordCnt )

{
   unsigned int dataRegAddr;
   unsigned int wCnt;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   // The maximum ATA DRQ block is 65536 bytes
   // or 32768 words.
   // The maximun ATAPI DRQ block is 131072 bytes
   // or 65536 words.
   // Because the REP INSW will fail if wordCnt
   // is greater than 65535 the transfer is split
   // into chunks of 32768 words or less.

   while ( wordCnt > 0L )
   {

      if ( wordCnt > 32768L )
         wCnt = 32768U;
      else
         wCnt = (unsigned int) wordCnt;

      repinword(bufSeg,bufOff,wCnt,dataRegAddr);
      wordCnt = wordCnt - (long) wCnt;
   }
}

//*************************************************************

extern void repoutword(unsigned short int bufSeg, unsigned int bufOff,
             unsigned int bCnt,unsigned short int dataRegAddr);


void pio_rep_outword( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      unsigned int wordCnt )

{
   unsigned int dataRegAddr;
   unsigned int wCnt;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   // The maximum ATA DRQ block is 65536 bytes
   // or 32768 words.
   // The maximun ATAPI DRQ block is 131072 bytes
   // or 65536 words.
   // Because the REP INSW will fail if wordCnt
   // is greater than 65535 the transfer is split
   // into chunks of 32768 words or less.

   while ( wordCnt > 0L )
   {

      if ( wordCnt > 32768L )
         wCnt = 32768U;
      else
         wCnt = (unsigned int) wordCnt;
      repoutword(bufSeg,bufOff,wCnt,dataRegAddr);
      wordCnt = wordCnt - (long) wCnt;
   }
}

//*************************************************************
extern void repindword(unsigned short int bufSeg, unsigned int bufOff,
             unsigned int bCnt,unsigned short int dataRegAddr);


void pio_rep_indword( unsigned int addrDataReg,
                      unsigned int bufSeg, unsigned int bufOff,
                      unsigned int dwordCnt )

{
   unsigned int dataRegAddr;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];

   // The maximum ATA DRQ block is 65536 bytes
   // or 32768 words or 16384 dwords.
   // The maximun ATAPI DRQ block is 131072 bytes
   // or 65536 words or 32768 dwords.
   // NOTE: The REP INSD below will fail if dwordCnt
   // is greater than 65535 !
   repindword(bufSeg,bufOff,dwordCnt,dataRegAddr);
}

//*************************************************************
extern void repoutdword(unsigned short int bufSeg, unsigned int bufOff,
             unsigned int bCnt,unsigned short int dataRegAddr);

void pio_rep_outdword( unsigned int addrDataReg,
                       unsigned int bufSeg, unsigned int bufOff,
                       unsigned int dwordCnt )

{
   unsigned int dataRegAddr;

   dataRegAddr = pio_reg_addrs[ addrDataReg ];
   // The maximum ATA DRQ block is 65536 bytes
   // or 32768 words or 16384 dwords.
   // The maximun ATAPI DRQ block is 131072 bytes
   // or 65536 words or 32768 dwords.
   // NOTE: The REP OUTSD below will fail if dwordCnt
   // is greater than 65535 !
   repoutdword(bufSeg,bufOff,dwordCnt,dataRegAddr);
}

// end ataiopio.c
