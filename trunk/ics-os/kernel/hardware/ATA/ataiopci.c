//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOPCI.C
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
// This C source contains the main body of the driver code:
// Determine what devices are present, issue ATA Soft Reset,
// execute Non-Data, PIO data in and PIO data out commands.
//
// Note: ATA-4 has added a few timing requirements for the
// host software that were not previously in ATA-2 or ATA-3.
// The new requirements are not implemented in this code
// and are probably never required for anything other than
// ATAPI devices.
//********************************************************************

#include <dos.h>

#include "ataio.h"

#include "ataiopd.h"

//***********************************************************
//
// pci bus master registers and PRD list buffer,
// see SFF-8038 and dma_pci_config().
//
//***********************************************************

// public data

unsigned int dma_pci_sff_reg_addr;     // SFF-8038 reg address
unsigned int dma_pci_prd_addr;         // segment part of PRD address

// private data

static unsigned int busMasterRegs = 0; // address bus master regs

#define BM_COMMAND_REG    0            // offset to command reg
#define BM_CR_MASK_READ    0x00           // read from memory
#define BM_CR_MASK_WRITE   0x08           // write to memory
#define BM_CR_MASK_START   0x01           // start transfer
#define BM_CR_MASK_STOP    0x00           // stop transfer

#define BM_STATUS_REG     2            // offset to status reg
#define BM_SR_MASK_SIMPLEX 0x80           // simplex only
#define BM_SR_MASK_DRV1    0x40           // drive 1 can do dma
#define BM_SR_MASK_DRV0    0x20           // drive 0 can do dma
#define BM_SR_MASK_INT     0x04           // INTRQ signal asserted
#define BM_SR_MASK_ERR     0x02           // error
#define BM_SR_MASK_ACT     0x01           // active

#define BM_PRD_ADDR_LOW   4            // offset to prd addr reg low 16 bits
#define BM_PRD_ADDR_HIGH  6            // offset to prd addr reg high 16 bits

static unsigned char prdBuf[64];       // prd list goes in here

static unsigned long far * prdBufPtr;  // PRD address (seg:off, off is 0)
static unsigned int prdBufPtrHigh16;   // 32-bit prd ptr upper 16 bits
static unsigned int prdBufPtrLow16;    // 32-bit prd ptr lower 16 bits

static unsigned char statReg;          // save BM status reg bits
static unsigned char rwControl;        // read/write control bit setting

//***********************************************************
//
// pci bus master control macros
//
//***********************************************************

#define enableChan()  outportb( busMasterRegs + BM_COMMAND_REG, \
                                rwControl | BM_CR_MASK_START )
#define disableChan() outportb( busMasterRegs + BM_COMMAND_REG, \
                                BM_CR_MASK_STOP )
#define clearChan() outportb( busMasterRegs + BM_STATUS_REG, \
                              statReg | BM_SR_MASK_INT | BM_SR_MASK_ERR )

//***********************************************************
//
// set_up_xfer() -- set up 1, 2 or 3 PRD
//
//***********************************************************

static void set_up_xfer( int dir, long count, unsigned seg, unsigned off );

static void set_up_xfer( int dir, long count, unsigned seg, unsigned off )

{
   int numPrd;             // number of PRD required
   unsigned long addr;     // physical memory address
   unsigned long addr1;    // physical address for 1st prd area
   unsigned long addr2;    // physical address for 2nd prd area
   unsigned long addr3;    // physical address for 3rd prd area
   long count1;            // byte count for 1st prd area
   long count2;            // byte count for 2nd prd area
   long count3;            // byte count for 3nd prd area
   long temp;
   unsigned long far * lwp;

   // disable/stop the dma channel, clear interrupt and error bits

   trc_llt( 0, 0, TRC_LLT_DMA3 );
   disableChan();
   clearChan();

   // convert transfer address from seg:off to an absolute memory address

   addr = (unsigned long) seg;
   addr = addr << 4;
   addr = addr + (unsigned long) off;

   // the address for each prd is easy to compute.
   // note: the address must be an even number but that
   //       is not checked here.

   addr1 = addr;
   addr2 = ( addr1 & 0xffff0000L ) + 0x00010000L;
   addr3 = ( addr2 & 0xffff0000L ) + 0x00010000L;

   // set up the 1st, 2nd and 3rd prd's as required.
   // note: count must be an even number but that is
   //       not checked here!

   numPrd = 1;                      // assume only 1 prd
   count1 = count;
   count2 = 0L;
   count3 = 0L;
   temp = addr2 - addr1;            // size of 1st prd area
   if ( count > temp )              // does it fit?
   {
      count = count - temp;         // no...
      numPrd = 2;                   // need 2 prd
      count1 = temp;
      count2 = count;
      temp = addr3 - addr2;         // size of 2nd prd area
      if ( count > temp )           // does it fit?
      {
         count = count - temp;      // no...
         numPrd = 3;                // need 3 prd
         count2 = temp;
         count3 = count;
      }
   }

   // set the end bit in the prd list

   if ( numPrd == 3 )
      count3 = count3 | 0x80000000L;
   else
   if ( numPrd == 2 )
      count2 = count2 | 0x80000000L;
   else
      count1 = count1 | 0x80000000L;

   // build the prd list in the prd buffer

   lwp = prdBufPtr;
   * lwp = addr1;
   lwp ++ ;
   * lwp = count1 & 0x8000ffffL;
   lwp ++ ;
   * lwp = addr2;
   lwp ++ ;
   * lwp = count2 & 0x8000ffffL;
   lwp ++ ;
   * lwp = addr3;
   lwp ++ ;
   * lwp = count3 & 0x8000ffffL;

   // set the prd list address

   outport( busMasterRegs + BM_PRD_ADDR_LOW, prdBufPtrLow16 );
   outport( busMasterRegs + BM_PRD_ADDR_HIGH, prdBufPtrHigh16 );

   // set the read/write control:
   // PCI reads for ATA Write DMA commands,
   // PCI writes for ATA Read DMA commands.

   if ( dir )
      rwControl = BM_CR_MASK_READ;     // ATA Write DMA
   else
      rwControl = BM_CR_MASK_WRITE;    // ATA Read DMA
   outportb( busMasterRegs + BM_COMMAND_REG, rwControl );
}

//***********************************************************
//
// chk_cmd_done() -- check for command completion
//
//***********************************************************

static int chk_cmd_done( void );

static int chk_cmd_done( void )

{
   int doneStatus;
   int ndx;

   // check for interrupt or poll status

   if ( int_use_intr_flag )
   {
      if ( int_intr_flag )                      // check for INT 76
      {
         doneStatus = pio_inbyte( CB_ASTAT );   // read status
         return 1;                              // cmd done
      }
   }
   else
   {
      for ( ndx = 0; ndx < 100; ndx ++ )     // kill some time here
         tmr_read_bios_timer();              // so that some dma gets done
      doneStatus = pio_inbyte( CB_ASTAT );   // poll for not busy & DRQ/errors
      if ( ( doneStatus & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == 0 )
         return 1;                  // cmd done
      if ( ( doneStatus & ( CB_STAT_BSY | CB_STAT_DF ) ) == CB_STAT_DF )
         return 1;                  // cmd done
      if ( ( doneStatus & ( CB_STAT_BSY | CB_STAT_ERR ) ) == CB_STAT_ERR )
         return 1;                  // cmd done
   }
   return 0;                        // not done yet
}

//***********************************************************
//
// dma_pci_config() - configure/setup for Read/Write DMA
//
// The caller must call this function before attempting
// to use any ATA or ATAPI commands in PCI DMA mode.
//
//***********************************************************

int dma_pci_config( unsigned int regAddr )

{
   unsigned int off;
   unsigned int seg;
   long lw;

   // check reg address

   if ( regAddr & 0x0007 )       // error if regs addr
      return 1;                  // are not xxx0h or xxx8h

   // save the address of the bus master (SFF-8038) regs

   dma_pci_sff_reg_addr = busMasterRegs = regAddr;

   // disable if reg address is zero

   if ( ! regAddr )              // if zero,
      return 0;                  // PCI DMA is disabled.

   // the bus master must be setup now

   if ( ! ( inportb( busMasterRegs + BM_STATUS_REG )
            &
            ( BM_SR_MASK_DRV1 | BM_SR_MASK_DRV0 )
          )
      )
      return 2;

   // set up the PRD buffer address

   seg = FP_SEG( (unsigned char far *) prdBuf );
   off = FP_OFF( (unsigned char far *) prdBuf );
   off = off + 16;
   off = off >> 4;
   seg = seg + off;
   dma_pci_prd_addr = seg;
   prdBufPtr = MK_FP( seg, 0 );
   lw = seg;
   lw = lw << 4;
   prdBufPtrLow16 = (unsigned int) ( lw & 0x0000ffffL );
   prdBufPtrHigh16 = (unsigned int) ( ( lw & 0xffff0000L ) >> 16 );

   // read the BM status reg and save the upper 3 bits.

   statReg = inportb( busMasterRegs + BM_STATUS_REG );
   statReg = statReg & 0xe0;

   return 0;
}

//***********************************************************
//
// dma_pci_ata_lba() - DMA in PCI Multiword for ATA R/W DMA
//
//***********************************************************

int dma_pci_ata_lba( int dev, int cmd,
                            int fr, int sc,
                            long lba,
                            unsigned seg, unsigned off )

{
   unsigned int cyl;
   int head, sect;

   sect = (int) ( lba & 0x000000ffL );
   lba = lba >> 8;
   cyl = (int) ( lba & 0x0000ffffL );
   lba = lba >> 16;
   head = ( (int) ( lba & 0x0fL ) ) | 0x40;

   return dma_pci_ata( dev, cmd,
                       fr, sc,
                       cyl, head, sect,
                       seg, off );
}

//***********************************************************
//
// dma_pci_ata() - PCI Bus Master for ATA R/W DMA commands
//
//***********************************************************

int dma_pci_ata( int dev, int cmd,
                 int fr, int sc,
                 unsigned int cyl, int head, int sect,
                 unsigned seg, unsigned off )

{
   unsigned char devHead;
   unsigned char devCtrl;
   unsigned char cylLow;
   unsigned char cylHigh;
   unsigned char status;
   int ns;

   // mark start of a R/W DMA command in low level trace

   trc_llt( 0, 0, TRC_LLT_S_RWD );

   // setup register values

   devCtrl = 0;      // in the old days we would set CB_DC_HD15 = 1
   devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
   devHead = devHead | ( head & 0x4f );
   cylLow = cyl & 0x00ff;
   cylHigh = ( cyl & 0xff00 ) >> 8;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATA;
   reg_cmd_info.ct  = ( cmd == CMD_WRITE_DMA )
                      ? TRC_TYPE_ADMAO : TRC_TYPE_ADMAI;
   reg_cmd_info.cmd = cmd;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = sc;
   reg_cmd_info.sn1 = sect;
   reg_cmd_info.cl1 = cylLow;
   reg_cmd_info.ch1 = cylHigh;
   reg_cmd_info.dh1 = devHead;
   reg_cmd_info.dc1 = devCtrl;

   // Quit now if the command is incorrect.

   if ( ( cmd != CMD_READ_DMA ) && ( cmd != CMD_WRITE_DMA ) )
   {
      reg_cmd_info.ec = 77;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_RWD );
      return 1;
   }

   // Quit now if no dma channel set up.

   if ( ! busMasterRegs )
   {
      reg_cmd_info.ec = 70;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_RWD );
      return 1;
   }

   // set up the dma transfer

   ns = sc;
   if ( ! ns )
      ns = 256;
   set_up_xfer( cmd == CMD_WRITE_DMA, ns * 512L, seg, off );

   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the sub_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_RWD );
      return 1;
   }

   // Set up all the registers except the command register.

   pio_outbyte( CB_DC, devCtrl );
   pio_outbyte( CB_FR, fr );
   pio_outbyte( CB_SC, sc );
   pio_outbyte( CB_SN, sect );
   pio_outbyte( CB_CL, cylLow );
   pio_outbyte( CB_CH, cylHigh );
   pio_outbyte( CB_DH, devHead );

   // For interrupt mode,
   // Take over INT 7x and initialize interrupt controller
   // and reset interrupt flag.

   int_save_int_vect();

   // Start the command by setting the Command register.  The drive
   // should immediately set BUSY status.

   pio_outbyte( CB_CMD, cmd );

   // Waste some time by reading the alternate status a few times.
   // This gives the drive time to set BUSY in the status register on
   // really fast systems.  If we don't do this, a slow drive on a fast
   // system may not set BUSY fast enough and we would think it had
   // completed the command when it really had not even started the
   // command yet.

   DELAY400NS;

   // enable/start the dma channel.

   trc_llt( 0, 0, TRC_LLT_DMA1 );
   enableChan();

   // Data transfer...
   // the device and dma channel transfer the data here while we start
   // checking for command completion...

   // End of command...
   // if no error,
   // wait for drive to signal command completion
   // -or-
   // time out if this takes to long.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( int_use_intr_flag )
         trc_llt( 0, 0, TRC_LLT_WINT );
      else
         trc_llt( 0, 0, TRC_LLT_PNBSY );
      while ( 1 )
      {
         if ( chk_cmd_done() )               // done yet ?
            break;                           // yes
         if ( tmr_chk_timeout() )            // time out yet ?
         {
            trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 73;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
      }
   }

   #if 0
   trc_llt( 0,
            inportb( busMasterRegs + BM_STATUS_REG ),
            TRC_LLT_DEBUG );     // for debugging
   #endif

   // End of command...
   // now wait for the PCI BM DMA channel to flush the data.

   while ( 1 )
   {
      if ( ( inportb( busMasterRegs + BM_STATUS_REG )
             & BM_SR_MASK_INT )
         )                                // done yet ?
         break;                           // yes
      if ( tmr_chk_timeout() )            // time out yet ?
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 71;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
         break;
      }
   }

   #if 0
   trc_llt( 0,
            inportb( busMasterRegs + BM_STATUS_REG ),
            TRC_LLT_DEBUG );     // for debugging
   #endif

   // End of command...
   // disable/stop the dma channel

   trc_llt( 0, 0, TRC_LLT_DMA3 );
   disableChan();

   // End of command...
   // Read the primary status register.  In keeping with the rules
   // stated above the primary status register is read only
   // ONCE.

   status = pio_inbyte( CB_STAT );

   // Final status check...
   // if no error, check final status...
   // Error if BUSY, DEVICE FAULT, DRQ or ERROR status now.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 74;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      }
   }

   // Final status check...
   // if no error, check make sure the device asserted the INTRQ signal.

   if ( ( reg_cmd_info.ec == 0 )
        &&
        ( ( inportb( busMasterRegs + BM_STATUS_REG ) & BM_SR_MASK_INT ) == 0 )
      )
   {
      reg_cmd_info.ec = 71;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
   }

   // Final status check...
   // if any error, update total bytes transferred.

   if ( reg_cmd_info.ec == 0 )
      reg_cmd_info.totalBytesXfer = ns * 512L;
   else
      reg_cmd_info.totalBytesXfer = 0L;

   // Done...
   // Read the output registers and trace the command.

   sub_trace_command();

   // Done...
   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();

   // Done...
   // mark end of a R/W DMA command in low level trace

   trc_llt( 0, 0, TRC_LLT_E_RWD );

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//***********************************************************
//
// dma_pci_packet() - PCI Bus Master for ATAPI Packet command
//
//***********************************************************

int dma_pci_packet( int dev,
                    unsigned int cpbc,
                    unsigned int cpseg, unsigned int cpoff,
                    int dir,
                    long dpbc,
                    unsigned int dpseg, unsigned int dpoff )

{
   unsigned char devCtrl;
   unsigned char devHead;
   unsigned char cylLow;
   unsigned char cylHigh;
   unsigned char fr;
   unsigned char status;
   unsigned char reason;
   unsigned char lowCyl;
   unsigned char highCyl;
   int ndx;
   unsigned char * cp;
   unsigned char far * cfp;

   // mark start of isa dma PI cmd in low level trace

   trc_llt( 0, 0, TRC_LLT_S_PID );

   // setup register values

   devCtrl = 0;      // in the old days we would set CB_DC_HD15 = 1
   devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
   cylLow = dpbc & 0x00ff;
   cylHigh = ( dpbc & 0xff00 ) >> 8;
   fr = 0x01;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATAPI;
   reg_cmd_info.ct  = dir ? TRC_TYPE_PDMAO : TRC_TYPE_PDMAI;
   reg_cmd_info.cmd = CMD_PACKET;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = 0;
   reg_cmd_info.sn1 = 0;
   reg_cmd_info.cl1 = cylLow;
   reg_cmd_info.ch1 = cylHigh;
   reg_cmd_info.dh1 = devHead;
   reg_cmd_info.dc1 = devCtrl;

   // Make sure the command packet size is either 12 or 16
   // and save the command packet size and data.

   cpbc = cpbc < 12 ? 12 : cpbc;
   cpbc = cpbc > 12 ? 16 : cpbc;
   reg_atapi_cp_size = cpbc;
   cp = reg_atapi_cp_data;
   cfp = MK_FP( cpseg, cpoff );
   for ( ndx = 0; ndx < cpbc; ndx ++ )
   {
      * cp = * cfp;
      cp ++ ;
      cfp ++ ;
   }

   // Quit now if no dma channel set up

   if ( ! busMasterRegs )
   {
      reg_cmd_info.ec = 70;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_PID );
      reg_atapi_max_bytes = 32768L;    // reset max bytes
      return 1;
   }

   // set up the dma transfer(s) --
   // the data packet byte count must be even
   // and must not be zero

   if ( dpbc == 0xffff )
      dpbc = 0xfffe;
   if ( dpbc < 2 )
      dpbc = 2;
   if ( dpbc & 0x0001 )
      dpbc ++ ;
   set_up_xfer( dir, dpbc, dpseg, dpoff );

   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the reg_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();
      trc_llt( 0, 0, TRC_LLT_E_PID );
      reg_atapi_max_bytes = 32768L;    // reset max bytes
      return 1;
   }

   // Set up all the registers except the command register.

   pio_outbyte( CB_DC, devCtrl );
   pio_outbyte( CB_FR, fr );
   pio_outbyte( CB_SC, 0 );
   pio_outbyte( CB_SN, 0 );
   pio_outbyte( CB_CL, 0 );
   pio_outbyte( CB_CH, 0 );
   pio_outbyte( CB_DH, devHead );

   // For interrupt mode,
   // Take over INT 7x and initialize interrupt controller
   // and reset interrupt flag.

   int_save_int_vect();

   // Start the command by setting the Command register.  The drive
   // should immediately set BUSY status.

   pio_outbyte( CB_CMD, CMD_PACKET );

   // Waste some time by reading the alternate status a few times.
   // This gives the drive time to set BUSY in the status register on
   // really fast systems.  If we don't do this, a slow drive on a fast
   // system may not set BUSY fast enough and we would think it had
   // completed the command when it really had not even started the
   // command yet.

   DELAY400NS;

   // Command packet transfer...
   // Check for protocol failures,
   // the device should have BSY=1 or
   // if BSY=0 then either DRQ=1 or CHK=1.

   sub_atapi_delay( dev );
   status = pio_inbyte( CB_ASTAT );
   if ( status & CB_STAT_BSY )
   {
      // BSY=1 is OK
   }
   else
   {
      if ( status & ( CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         // BSY=0 and DRQ=1 is OK
         // BSY=0 and ERR=1 is OK
      }
      else
      {
         reg_cmd_info.failbits |= FAILBIT0;  // not OK
      }
   }

   // Command packet transfer...
   // Poll Alternate Status for BSY=0.

   trc_llt( 0, 0, TRC_LLT_PNBSY );
   while ( 1 )
   {
      status = pio_inbyte( CB_ASTAT );       // poll for not busy
      if ( ( status & CB_STAT_BSY ) == 0 )
         break;
      if ( tmr_chk_timeout() )               // time out yet ?
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );      // yes
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 75;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
         break;
      }
   }

   // Command packet transfer...
   // Check for protocol failures... no interrupt here please!
   // Clear any interrupt the command packet transfer may have caused.

   if ( int_intr_flag )
      reg_cmd_info.failbits |= FAILBIT1;
   int_intr_flag = 0;

   // Command packet transfer...
   // If no error, transfer the command packet.

   if ( reg_cmd_info.ec == 0 )
   {

      // Command packet transfer...
      // Read the primary status register and the other ATAPI registers.

      status = pio_inbyte( CB_STAT );
      reason = pio_inbyte( CB_SC );
      lowCyl = pio_inbyte( CB_CL );
      highCyl = pio_inbyte( CB_CH );

      // Command packet transfer...
      // check status: must have BSY=0, DRQ=1 now

      if (    ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
           != CB_STAT_DRQ
         )
      {
         reg_cmd_info.ec = 76;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      }
      else
      {
         // Command packet transfer...
         // Check for protocol failures...
         // check: C/nD=1, IO=0.

         if ( ( reason &  ( CB_SC_P_TAG | CB_SC_P_REL | CB_SC_P_IO ) )
              || ( ! ( reason &  CB_SC_P_CD ) )
            )
            reg_cmd_info.failbits |= FAILBIT2;
         if ( ( lowCyl != cylLow ) || ( highCyl != cylHigh ) )
            reg_cmd_info.failbits |= FAILBIT3;

         // Command packet transfer...
         // trace cdb byte 0,
         // xfer the command packet (the cdb)

         trc_llt( 0, * (unsigned char far *) MK_FP( cpseg, cpoff ), TRC_LLT_P_CMD );
         pio_rep_outword( CB_DATA, cpseg, cpoff, cpbc >> 1 );
      }
   }

   // enable/start the dma channel.

   trc_llt( 0, 0, TRC_LLT_DMA1 );
   enableChan();

   // Data transfer...
   // the device and dma channel transfer the data here while we start
   // checking for command completion...

   // End of command...
   // if no error,
   // wait for drive to signal command completion
   // -or-
   // time out if this takes to long.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( int_use_intr_flag )
         trc_llt( 0, 0, TRC_LLT_WINT );
      else
         trc_llt( 0, 0, TRC_LLT_PNBSY );
      while ( 1 )
      {
         if ( chk_cmd_done() )               // done yet ?
            break;                           // yes
         if ( tmr_chk_timeout() )            // time out yet ?
         {
            trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 73;
            trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
            break;
         }
      }
   }

   #if 0
   trc_llt( 0,
            inportb( busMasterRegs + BM_STATUS_REG ),
            TRC_LLT_DEBUG );     // for debugging
   #endif

   // End of command...
   // now wait for the PCI BM DMA channel to flush the data.

   while ( 1 )
   {
      if ( ( inportb( busMasterRegs + BM_STATUS_REG )
             & BM_SR_MASK_INT )
         )                                // done yet ?
         break;                           // yes
      if ( tmr_chk_timeout() )            // time out yet ?
      {
         trc_llt( 0, 0, TRC_LLT_TOUT );   // yes
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 71;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
         break;
      }
   }

   #if 0
   trc_llt( 0,
            inportb( busMasterRegs + BM_STATUS_REG ),
            TRC_LLT_DEBUG );     // for debugging
   #endif

   // End of command...
   // disable/stop the dma channel

   trc_llt( 0, 0, TRC_LLT_DMA3 );
   disableChan();

   // End of command...
   // Read the primary status register.  In keeping with the rules
   // stated above the primary status register is read only
   // ONCE.

   status = pio_inbyte( CB_STAT );

   // Final status check...
   // if no error, check final status...
   // Error if BUSY, DEVICE FAULT, DRQ or ERROR status now.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 74;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
      }
   }

   // Final status check...
   // Check for protocol failures...
   // check: C/nD=1, IO=1.

   reason = pio_inbyte( CB_SC );
   if ( ( reason & ( CB_SC_P_TAG | CB_SC_P_REL ) )
        || ( ! ( reason & CB_SC_P_IO ) )
        || ( ! ( reason & CB_SC_P_CD ) )
      )
      reg_cmd_info.failbits |= FAILBIT8;

   // Final status check...
   // if no error, check make sure the device asserted the INTRQ signal.

   if ( ( reg_cmd_info.ec == 0 )
        &&
        ( ( inportb( busMasterRegs + BM_STATUS_REG ) & BM_SR_MASK_INT ) == 0 )
      )
   {
      reg_cmd_info.ec = 71;
      trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
   }

   // Final status check...
   // if any error, update total bytes transferred.

   if ( reg_cmd_info.ec == 0 )
      reg_cmd_info.totalBytesXfer = dpbc;
   else
      reg_cmd_info.totalBytesXfer = 0L;

   // Done...
   // Read the output registers and trace the command.

   sub_trace_command();

   // Done...
   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();

   // Done...
   // mark end of isa dma PI cmd in low level trace

   trc_llt( 0, 0, TRC_LLT_E_PID );

   // All done.  The return values of this function are described in
   // ATAIO.H.

   reg_atapi_max_bytes = 32768L;    // reset max bytes
   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

// end ataiopci.c
