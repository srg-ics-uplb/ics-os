//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOREG.C
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

//*************************************************************
//
// extended error infomation,
// config information,
// atapi delay flag,
// atapi register data,
// slow transfer flag,
// atapi command packet.
//
//*************************************************************

void WAIT400NS()
{
    delay(10);
};

//*************************************************************
//
// reg_wait_poll() - wait for INT 7x or poll for BSY=0
//
//*************************************************************

static void reg_wait_poll( int we, int pe )

{
   unsigned char status;

   // Wait for INT 7x -or- wait for not BUSY -or- wait for time out.
   if ( we && int_use_intr_flag )
   {
      while ( 1 )
      {
         if ( int_intr_flag )                // wait for INT 7x
            break;
         if ( tmr_chk_timeout() )            // time out yet ?
         {
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = we;
            break;
         }
      }
   }
   else
   {
      while ( 1 )
      {
         status = pio_inbyte( CB_ASTAT );       // poll for not busy
         if ( ( status & CB_STAT_BSY ) == 0 )
            break;
         if ( tmr_chk_timeout() )               // time out yet ?
         {
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = pe;
            break;
         }
      }
   }

   // Reset the interrupt flag.

   if ( int_intr_flag > 1 )
      reg_cmd_info.failbits |= FAILBIT15;
   int_intr_flag = 0;
}

//*************************************************************
//
// reg_config() - Check the host adapter and determine the
//                number and type of drives attached.
//
// This process is not documented by any of the ATA standards.
//
// Infomation is returned by this function is in
// reg_config_info[] -- see ATAIO.H.
//
//*************************************************************

int reg_config( void )

{
   int numDev = 0;
   unsigned char sc;
   unsigned char sn;
   unsigned char cl;
   unsigned char ch;
   unsigned char st;
   unsigned char devCtrl;

   // setup register values

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );


   // assume there are no devices

   reg_config_info[0] = REG_CONFIG_TYPE_NONE;
   reg_config_info[1] = REG_CONFIG_TYPE_NONE;

   // set up Device Control register

   pio_outbyte( CB_DC, devCtrl );

   // lets see if there is a device 0

   pio_outbyte( CB_DH, CB_DH_DEV0 );
   
   //delay 4 milliseconds
   WAIT400NS();
   
   pio_outbyte( CB_SC, 0x55 );
   pio_outbyte( CB_SN, 0xaa );
   pio_outbyte( CB_SC, 0xaa );
   pio_outbyte( CB_SN, 0x55 );
   pio_outbyte( CB_SC, 0x55 );
   pio_outbyte( CB_SN, 0xaa );
   sc = pio_inbyte( CB_SC );
   sn = pio_inbyte( CB_SN );
   if ( ( sc == 0x55 ) && ( sn == 0xaa ) )
      reg_config_info[0] = REG_CONFIG_TYPE_UNKN;

   // lets see if there is a device 1

   pio_outbyte( CB_DH, CB_DH_DEV1 );
   WAIT400NS();
   pio_outbyte( CB_SC, 0x55 );
   pio_outbyte( CB_SN, 0xaa );
   pio_outbyte( CB_SC, 0xaa );
   pio_outbyte( CB_SN, 0x55 );
   pio_outbyte( CB_SC, 0x55 );
   pio_outbyte( CB_SN, 0xaa );
   sc = pio_inbyte( CB_SC );
   sn = pio_inbyte( CB_SN );
   if ( ( sc == 0x55 ) && ( sn == 0xaa ) )
      reg_config_info[1] = REG_CONFIG_TYPE_UNKN;

   // now we think we know which devices, if any are there,
   // so lets try a soft reset (ignoring any errors).

   pio_outbyte( CB_DH, CB_DH_DEV0 );
   WAIT400NS();
   reg_reset( 0, 0 );

   // lets check device 0 again, is device 0 really there?
   // is it ATA or ATAPI?

   pio_outbyte( CB_DH, CB_DH_DEV0 );
   WAIT400NS();
   sc = pio_inbyte( CB_SC );
   sn = pio_inbyte( CB_SN );
   if ( ( sc == 0x01 ) && ( sn == 0x01 ) )
   {
      reg_config_info[0] = REG_CONFIG_TYPE_UNKN;
      cl = pio_inbyte( CB_CL );
      ch = pio_inbyte( CB_CH );
      st = pio_inbyte( CB_STAT );
      if ( ( cl == 0x14 ) && ( ch == 0xeb ) )
         reg_config_info[0] = REG_CONFIG_TYPE_ATAPI;
      else
         if ( ( cl == 0x00 ) && ( ch == 0x00 ) && ( st != 0x00 ) )
            reg_config_info[0] = REG_CONFIG_TYPE_ATA;
   }

   // lets check device 1 again, is device 1 really there?
   // is it ATA or ATAPI?

   pio_outbyte( CB_DH, CB_DH_DEV1 );
   WAIT400NS();
   sc = pio_inbyte( CB_SC );
   sn = pio_inbyte( CB_SN );
   if ( ( sc == 0x01 ) && ( sn == 0x01 ) )
   {
      reg_config_info[1] = REG_CONFIG_TYPE_UNKN;
      cl = pio_inbyte( CB_CL );
      ch = pio_inbyte( CB_CH );
      st = pio_inbyte( CB_STAT );
      if ( ( cl == 0x14 ) && ( ch == 0xeb ) )
         reg_config_info[1] = REG_CONFIG_TYPE_ATAPI;
      else
         if ( ( cl == 0x00 ) && ( ch == 0x00 ) && ( st != 0x00 ) )
            reg_config_info[1] = REG_CONFIG_TYPE_ATA;
   }

   // If possible, select a device that exists, try device 0 first.

   if ( reg_config_info[1] != REG_CONFIG_TYPE_NONE )
   {
      pio_outbyte( CB_DH, CB_DH_DEV1 );
      WAIT400NS();
      numDev ++ ;
   }
   if ( reg_config_info[0] != REG_CONFIG_TYPE_NONE )
   {
      pio_outbyte( CB_DH, CB_DH_DEV0 );
      WAIT400NS();
      numDev ++ ;
   }

   // return the number of devices found

   return numDev;
}

//*************************************************************
//
// reg_reset() - Execute a Software Reset.
//
// See ATA-2 Section 9.2, ATA-3 Section 9.2, ATA-4 Section 8.3.
//
//*************************************************************

int reg_reset( int skipFlag, int devRtrn )

{
   unsigned char sc;
   unsigned char sn;
   unsigned char status;
   unsigned char devCtrl;

   // setup register values

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_SRST;
   reg_cmd_info.ct  = TRC_TYPE_ASR;

   // initialize the command timeout counter

   tmr_set_timeout();

   // Set and then reset the soft reset bit in the Device Control
   // register.  This causes device 0 be selected.

   if ( ! skipFlag )
   {
      pio_outbyte( CB_DC, devCtrl | CB_DC_SRST );
      WAIT400NS();
      pio_outbyte( CB_DC, devCtrl );
      WAIT400NS();
   }

   // If there is a device 0, wait for device 0 to set BSY=0.

   if ( reg_config_info[0] != REG_CONFIG_TYPE_NONE )
   {
      sub_atapi_delay( 0 );
      while ( 1 )
      {
         status = pio_inbyte( CB_STAT );
         if ( ( status & CB_STAT_BSY ) == 0 )
            break;
         if ( tmr_chk_timeout() )
         {
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 1;
            break;
         }
      }
   }

   // If there is a device 1, wait until device 1 allows
   // register access.

   if ( reg_config_info[1] != REG_CONFIG_TYPE_NONE )
   {
      sub_atapi_delay( 1 );
      while ( 1 )
      {
         pio_outbyte( CB_DH, CB_DH_DEV1 );
         WAIT400NS();
         sc = pio_inbyte( CB_SC );
         sn = pio_inbyte( CB_SN );
         if ( ( sc == 0x01 ) && ( sn == 0x01 ) )
            break;
         if ( tmr_chk_timeout() )
         {
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 2;
            break;
         }
      }

      // Now check if drive 1 set BSY=0.

      if ( reg_cmd_info.ec == 0 )
      {
         if ( pio_inbyte( CB_STAT ) & CB_STAT_BSY )
         {
            reg_cmd_info.ec = 3;
         }
      }
   }

   // RESET_DONE:

   // We are done but now we must select the device the caller
   // requested before we trace the command.  This will cause
   // the correct data to be returned in reg_cmd_info.

   pio_outbyte( CB_DH, devRtrn ? CB_DH_DEV1 : CB_DH_DEV0 );
   WAIT400NS();
//   sub_trace_command();

   // If possible, select a device that exists,
   // try device 0 first.

   if ( reg_config_info[1] != REG_CONFIG_TYPE_NONE )
   {
      pio_outbyte( CB_DH, CB_DH_DEV1 );
      WAIT400NS();
   }
   if ( reg_config_info[0] != REG_CONFIG_TYPE_NONE )
   {
      pio_outbyte( CB_DH, CB_DH_DEV0 );
      WAIT400NS();
   }

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//*************************************************************
//
// reg_non_data_lba() - Easy way to execute a non-data command
//                      using an LBA sector address.
//
//*************************************************************

int reg_non_data_lba( int dev, int cmd,
                      int fr, int sc,
                      long lba )

{
   unsigned int cyl;
   int head, sect;

   sect = (int) ( lba & 0x000000ffL );
   lba = lba >> 8;
   cyl = (int) ( lba & 0x0000ffff );
   lba = lba >> 16;
   head = ( (int) ( lba & 0x0fL ) ) | 0x40;
   return reg_non_data( dev, cmd,
                        fr, sc,
                        cyl, head, sect );
}

//*************************************************************
//
// reg_non_data() - Execute a non-data command.
//
// Note special handling for Execute Device Diagnostics
// command when there is no device 0.
//
// See ATA-2 Section 9.5, ATA-3 Section 9.5,
// ATA-4 Section 8.8 Figure 12.  Also see Section 8.5.
//
//*************************************************************

int reg_non_data( int dev, int cmd,
                   int fr, int sc,
                   unsigned int cyl, int head, int sect )

{
   unsigned char secCnt;
   unsigned char secNum;
   unsigned char devHead;
   unsigned char devCtrl;
   unsigned char cylLow;
   unsigned char cylHigh;
   unsigned char status;

   // setup register values

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );
   devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
   devHead = devHead | ( head & 0x4f );
   cylLow = cyl & 0x00ff;
   cylHigh = ( cyl & 0xff00 ) >> 8;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATA;
   reg_cmd_info.ct  = TRC_TYPE_AND;
   reg_cmd_info.cmd = cmd;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = sc;
   reg_cmd_info.sn1 = sect;
   reg_cmd_info.cl1 = cylLow;
   reg_cmd_info.ch1 = cylHigh;
   reg_cmd_info.dh1 = devHead;
   reg_cmd_info.dc1 = devCtrl;

   // Set command time out.

   tmr_set_timeout();

   // PAY ATTENTION HERE
   // If the caller is attempting a Device Reset command, then
   // don't do most of the normal stuff.  Device Reset has no
   // parameters, should not generate an interrupt and it is the
   // only command that can be written to the Command register
   // when a device has BSY=1 (a very strange command!).  Not
   // all devices support this command (even some ATAPI devices
   // don't support the command.

   if ( cmd != CMD_DEVICE_RESET )
   {
      // Select the drive - call the sub_select function.
      // Quit now if this fails.

      if ( sub_select( dev ) )
      {
         sub_trace_command();
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
   }

   // Start the command by setting the Command register.  The drive
   // should immediately set BUSY status.

   pio_outbyte( CB_CMD, cmd );

   // Waste some time by reading the alternate status a few times.
   // This gives the drive time to set BUSY in the status register on
   // really fast systems.  If we don't do this, a slow drive on a fast
   // system may not set BUSY fast enough and we would think it had
   // completed the command when it really had not even started the
   // command yet.

   WAIT400NS();

   if ( reg_config_info[0] == REG_CONFIG_TYPE_ATAPI )
      sub_atapi_delay( 0 );
   if ( reg_config_info[1] == REG_CONFIG_TYPE_ATAPI )
      sub_atapi_delay( 1 );

   // IF
   //    This is an Exec Dev Diag command (cmd=0x90)
   //    and there is no device 0 then
   //    there will be no interrupt. So we must
   //    poll device 1 until it allows register
   //    access and then do normal polling of the Status
   //    register for BSY=0.
   // ELSE
   // IF
   //    This is a Dev Reset command (cmd=0x08) then
   //    there should be no interrupt.  So we must
   //    poll for BSY=0.
   // ELSE
   //    Do the normal wait for interrupt or polling for
   //    completion.

   if ( ( cmd == CMD_EXECUTE_DEVICE_DIAGNOSTIC )
        &&
        ( reg_config_info[0] == REG_CONFIG_TYPE_NONE )
      )
   {
      while ( 1 )
      {
         pio_outbyte( CB_DH, CB_DH_DEV1 );
         WAIT400NS();
         secCnt = pio_inbyte( CB_SC );
         secNum = pio_inbyte( CB_SN );
         if ( ( secCnt == 0x01 ) && ( secNum == 0x01 ) )
            break;
         if ( tmr_chk_timeout() )
         {
            reg_cmd_info.to = 1;
            reg_cmd_info.ec = 24;
            break;
         }
      }
   }
   else
   {
      if ( cmd == CMD_DEVICE_RESET )
      {
         // Wait for not BUSY -or- wait for time out.

         reg_wait_poll( 0, 23 );
      }
      else
      {
         // Wait for INT 7x -or- wait for not BUSY -or- wait for time out.

         reg_wait_poll( 22, 23 );
      }
   }

   // Read the primary status register.  In keeping with the rules
   // stated above the primary status register is read only
   // ONCE.

   status = pio_inbyte( CB_STAT );

   // Error if BUSY, DEVICE FAULT, DRQ or ERROR status now.

   if ( reg_cmd_info.ec == 0 )
   {
      if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 21;
      }
   }

   // read the output registers and trace the command.

   sub_trace_command();

   // NON_DATA_DONE:

   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//*************************************************************
//
// reg_pio_data_in_lba() - Easy way to execute a PIO Data In command
//                         using an LBA sector address.
//
//*************************************************************

int reg_pio_data_in_lba( int dev, int cmd,
                         int fr, int sc,
                         long lba,
                         unsigned seg, unsigned off,
                         int numSect, int multiCnt )

{
   unsigned int cyl;
   int head, sect;

   sect = (int) ( lba & 0x000000ffL );
   lba = lba >> 8;
   cyl = (int) ( lba & 0x0000ffff );
   lba = lba >> 16;
   head = ( (int) ( lba & 0x0fL ) ) | 0x40;
   return reg_pio_data_in( dev, cmd,
                           fr, sc,
                           cyl, head, sect,
                           seg, off,
                           numSect, multiCnt );
}

//*************************************************************
//
// reg_pio_data_in() - Execute a PIO Data In command.
//
// See ATA-2 Section 9.3, ATA-3 Section 9.3,
// ATA-4 Section 8.6 Figure 10.
//
//*************************************************************

int reg_pio_data_in( int dev, int cmd,
                     int fr, int sc,
                     unsigned int cyl, int head, int sect,
                     unsigned seg, unsigned off,
                     int numSect, int multiCnt )

{
   unsigned char devHead;
   unsigned char devCtrl;
   unsigned char cylLow;
   unsigned char cylHigh;
   unsigned char status;
   unsigned int wordCnt;

   // setup register values and adjust parameters

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );
   devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
   devHead = devHead | ( head & 0x4f );
   cylLow = cyl & 0x00ff;
   cylHigh = ( cyl & 0xff00 ) >> 8;
   // these commands transfer only 1 sector
   if (    ( cmd == CMD_IDENTIFY_DEVICE )
        || ( cmd == CMD_IDENTIFY_DEVICE_PACKET )
        || ( cmd == CMD_READ_BUFFER )
      )
      numSect = 1;
   // only Read Multiple uses multiCnt
   if ( cmd != CMD_READ_MULTIPLE )
      multiCnt = 1;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATA;
   reg_cmd_info.ct  = TRC_TYPE_APDI;
   reg_cmd_info.cmd = cmd;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = sc;
   reg_cmd_info.sn1 = sect;
   reg_cmd_info.cl1 = cylLow;
   reg_cmd_info.ch1 = cylHigh;
   reg_cmd_info.dh1 = devHead;
   reg_cmd_info.dc1 = devCtrl;

   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the sub_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();
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

   WAIT400NS();
   // Loop to read each sector.

   while ( 1 )
   {
      // READ_LOOP:
      //
      // NOTE NOTE NOTE ...  The primary status register (1f7) MUST NOT be
      // read more than ONCE for each sector transferred!  When the
      // primary status register is read, the drive resets IRQ 14.  The
      // alternate status register (3f6) can be read any number of times.
      // After INT 7x read the the primary status register ONCE
      // and transfer the 256 words (REP INSW).  AS SOON as BOTH the
      // primary status register has been read AND the last of the 256
      // words has been read, the drive is allowed to generate the next
      // IRQ 14 (newer and faster drives could generate the next IRQ 14 in
      // 50 microseconds or less).  If the primary status register is read
      // more than once, there is the possibility of a race between the
      // drive and the software and the next IRQ 14 could be reset before
      // the system interrupt controller sees it.

      // Wait for INT 7x -or- wait for not BUSY -or- wait for time out.

      sub_atapi_delay( dev );
      reg_wait_poll( 34, 35 );

      // Read the primary status register.  In keeping with the rules
      // stated above the primary status register is read only
      // ONCE.

      status = pio_inbyte( CB_STAT );

      // If there was a time out error, go to READ_DONE.

      if ( reg_cmd_info.ec )
         break;   // go to READ_DONE

      // If BSY=0 and DRQ=1, transfer the data,
      // even if we find out there is an error later.

      if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == CB_STAT_DRQ )
      {
         // do the slow data transfer thing

         if ( reg_slow_xfer_flag )
         {
            if ( numSect <= reg_slow_xfer_flag )
            {
               sub_xfer_delay();
               reg_slow_xfer_flag = 0;
            }
         }

         // increment number of DRQ packets

         reg_cmd_info.drqPackets ++ ;

         // determine the number of sectors to transfer

         wordCnt = multiCnt ? multiCnt : 1;
         if ( wordCnt > numSect )
            wordCnt = numSect;
         wordCnt = wordCnt * 256;

         // Do the REP INSW to read the data for one block.

         reg_cmd_info.totalBytesXfer += ( wordCnt << 1 );
         pio_rep_inword( CB_DATA, seg, off, wordCnt );

         WAIT400NS();

         // Note: The drive should have dropped DATA REQUEST by now.  If there
         // are more sectors to transfer, BUSY should be active now (unless
         // there is an error).

         // Decrement the count of sectors to be transferred
         // and increment buffer address.

         numSect = numSect - ( multiCnt ? multiCnt : 1 );
         

         off = off + ( 512 * ( multiCnt ? multiCnt : 1 ) );
      }

      // So was there any error condition?

      if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 31;
         break;   // go to READ_DONE
      }

      // DRQ should have been set -- was it?

      if ( ( status & CB_STAT_DRQ ) == 0 )
      {
         reg_cmd_info.ec = 32;
         break;   // go to READ_DONE
      }

      // If all of the requested sectors have been transferred, make a
      // few more checks before we exit.

      if ( numSect < 1 )
      {
         // Since the drive has transferred all of the requested sectors
         // without error, the drive should not have BUSY, DEVICE FAULT,
         // DATA REQUEST or ERROR active now.

         sub_atapi_delay( dev );
         status = pio_inbyte( CB_STAT );
         if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
         {
            reg_cmd_info.ec = 33;
            break;   // go to READ_DONE
         }

         // All sectors have been read without error, go to READ_DONE.

         break;   // go to READ_DONE

      }

      // This is the end of the read loop.  If we get here, the loop is
      // repeated to read the next sector.  Go back to READ_LOOP.

   }

   // READ_DONE:

   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();


   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//*************************************************************
//
// reg_pio_data_out_lba() - Easy way to execute a PIO Data In command
//                          using an LBA sector address.
//
//*************************************************************

int reg_pio_data_out_lba( int dev, int cmd,
                          int fr, int sc,
                          long lba,
                          unsigned seg, unsigned off,
                          int numSect, int multiCnt )

{
   unsigned int cyl;
   int head, sect;

   sect = (int) ( lba & 0x000000ffL );
   lba = lba >> 8;
   cyl = (int) ( lba & 0x0000ffff );
   lba = lba >> 16;
   head = ( (int) ( lba & 0x0fL ) ) | 0x40;
   return reg_pio_data_out( dev, cmd,
                            fr, sc,
                            cyl, head, sect,
                            seg, off,
                            numSect, multiCnt );
}

//*************************************************************
//
// reg_pio_data_out() - Execute a PIO Data Out command.
//
// See ATA-2 Section 9.4, ATA-3 Section 9.4,
// ATA-4 Section 8.7 Figure 11.
//
//*************************************************************

int reg_pio_data_out( int dev, int cmd,
                      int fr, int sc,
                      unsigned int cyl, int head, int sect,
                      unsigned seg, unsigned off,
                      int numSect, int multiCnt )

{
   unsigned char devHead;
   unsigned char devCtrl;
   unsigned char cylLow;
   unsigned char cylHigh;
   unsigned char status;
   int loopFlag = 1;
   unsigned int wordCnt;


   // setup register values and adjust parameters

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );
   devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
   devHead = devHead | ( head & 0x4f );
   cylLow = cyl & 0x00ff;
   cylHigh = ( cyl & 0xff00 ) >> 8;
   // these commands transfer only 1 sector
   if ( cmd == CMD_WRITE_BUFFER )
      numSect = 1;
   // only Write Multiple and CFA Write Multiple W/O Erase uses multCnt
   if (    ( cmd != CMD_WRITE_MULTIPLE )
        && ( cmd != CMD_CFA_WRITE_MULTIPLE_WO_ERASE )
      )
      multiCnt = 1;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATA;
   reg_cmd_info.ct  = TRC_TYPE_APDO;
   reg_cmd_info.cmd = cmd;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = sc;
   reg_cmd_info.sn1 = sect;
   reg_cmd_info.cl1 = cylLow;
   reg_cmd_info.ch1 = cylHigh;
   reg_cmd_info.dh1 = devHead;
   reg_cmd_info.dc1 = devCtrl;

   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the sub_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();

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

   WAIT400NS();

   // Wait for not BUSY or time out.
   // Note: No interrupt is generated for the
   // first sector of a write command.  Well...
   // that's not really true we are working with
   // a PCMCIA PC Card ATA device.

   sub_atapi_delay( dev );
   while ( 1 )
   {
      status = pio_inbyte( CB_ASTAT );
      if ( ( status & CB_STAT_BSY ) == 0 )
         break;
      if ( tmr_chk_timeout() )
      {
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 47;
         loopFlag = 0;
         break;
      }
   }

   // If we are using interrupts and we just got an interrupt, this is
   // wrong.  The drive must not generate an interrupt at this time.

   if ( loopFlag && int_use_intr_flag && int_intr_flag )
   {
      reg_cmd_info.ec = 46;
      loopFlag = 0;
   }

   // This loop writes each sector.

   while ( loopFlag )
   {
      // WRITE_LOOP:
      //
      // NOTE NOTE NOTE ...  The primary status register (1f7) MUST NOT be
      // read more than ONCE for each sector transferred!  When the
      // primary status register is read, the drive resets IRQ 14.  The
      // alternate status register (3f6) can be read any number of times.
      // For correct results, transfer the 256 words (REP OUTSW), wait for
      // INT 7x and then read the primary status register.  AS
      // SOON as BOTH the primary status register has been read AND the
      // last of the 256 words has been written, the drive is allowed to
      // generate the next IRQ 14 (newer and faster drives could generate
      // the next IRQ 14 in 50 microseconds or less).  If the primary
      // status register is read more than once, there is the possibility
      // of a race between the drive and the software and the next IRQ 14
      // could be reset before the system interrupt controller sees it.

      // If BSY=0 and DRQ=1, transfer the data,
      // even if we find out there is an error later.

      if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == CB_STAT_DRQ )
      {
         // do the slow data transfer thing

         if ( reg_slow_xfer_flag )
         {
            if ( numSect <= reg_slow_xfer_flag )
            {
               sub_xfer_delay();
               reg_slow_xfer_flag = 0;
            }
         }

         // increment number of DRQ packets

         reg_cmd_info.drqPackets ++ ;

         // determine the number of sectors to transfer

         wordCnt = multiCnt ? multiCnt : 1;
         if ( wordCnt > numSect )
            wordCnt = numSect;
         wordCnt = wordCnt * 256;

         // Do the REP OUTSW to write the data for one block.

         reg_cmd_info.totalBytesXfer += ( wordCnt << 1 );
         pio_rep_outword( CB_DATA, seg, off, wordCnt );
         
         WAIT400NS();
         // Note: The drive should have dropped DATA REQUEST and
         // raised BUSY by now.

         // Decrement the count of sectors to be transferred
         // and increment buffer address.
         numSect = numSect - ( multiCnt ? multiCnt : 1 );
         off = off + ( 512 * ( multiCnt ? multiCnt : 1 ) );
      }

      // So was there any error condition?

      if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 41;
         break;   // go to WRITE_DONE
      }

      // DRQ should have been set -- was it?

      if ( ( status & CB_STAT_DRQ ) == 0 )
      {
         reg_cmd_info.ec = 42;
         break;   // go to WRITE_DONE
      }

      // Wait for INT 7x -or- wait for not BUSY -or- wait for time out.

      sub_atapi_delay( dev );
      reg_wait_poll( 44, 45 );

      // Read the primary status register.  In keeping with the rules
      // stated above the primary status register is read only ONCE.

      status = pio_inbyte( CB_STAT );

      // If there was a time out error, go to WRITE_DONE.

      if ( reg_cmd_info.ec )
         break;   // go to WRITE_DONE

      // If all of the requested sectors have been transferred, make a
      // few more checks before we exit.

      if ( numSect < 1 )
      {
         // Since the drive has transferred all of the sectors without
         // error, the drive MUST not have BUSY, DEVICE FAULT, DATA REQUEST
         // or ERROR status at this time.

         if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
         {
            reg_cmd_info.ec = 43;
            break;   // go to WRITE_DONE
         }

         // All sectors have been written without error, go to WRITE_DONE.

         break;   // go to WRITE_DONE

      }

      //
      // This is the end of the write loop.  If we get here, the loop
      // is repeated to write the next sector.  Go back to WRITE_LOOP.

   }

   // read the output registers and trace the command.

   sub_trace_command();

   // WRITE_DONE:

   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//*************************************************************
//
// reg_packet() - Execute an ATAPI Packet (A0H) command.
//
// See ATA-4 Section 9.10, Figure 14.
//
//*************************************************************

int reg_packet( int dev,
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
   unsigned char frReg;
   unsigned char scReg;
   unsigned char snReg;
   unsigned char status;
   unsigned char reason;
   unsigned char lowCyl;
   unsigned char highCyl;
   unsigned int byteCnt;
   unsigned int wordCnt;
   int ndx;
   unsigned long dpaddr;
   unsigned char * cp;
   unsigned char * cfp;
   int slowXferCntr = 0;
   int prevFailBit7 = 0;


   // setup register values

   devCtrl = CB_DC_HD15 | ( int_use_intr_flag ? 0 : CB_DC_NIEN );
   reg_atapi_reg_dh = reg_atapi_reg_dh & 0x0f;
   devHead = ( dev ? CB_DH_DEV1 : CB_DH_DEV0 ) | reg_atapi_reg_dh;
   cylLow = dpbc & 0x00ff;
   cylHigh = ( dpbc & 0xff00 ) >> 8;
   frReg = reg_atapi_reg_fr;
   scReg = reg_atapi_reg_sc;
   snReg = reg_atapi_reg_sn;
   reg_atapi_reg_fr = 0;
   reg_atapi_reg_sc = 0;
   reg_atapi_reg_sn = 0;
   reg_atapi_reg_dh = 0;

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATAPI;
   reg_cmd_info.ct  = dir ? TRC_TYPE_PPDO : TRC_TYPE_PPDI;
   reg_cmd_info.cmd = CMD_PACKET;
   reg_cmd_info.fr1 = frReg;
   reg_cmd_info.sc1 = scReg;
   reg_cmd_info.sn1 = snReg;
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
   cfp = cpoff;
   for ( ndx = 0; ndx < cpbc; ndx ++ )
   {
      * cp = * cfp;
      cp ++ ;
      cfp ++ ;
   }

   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the sub_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();
      reg_atapi_max_bytes = 32768L;    // reset max bytes
      return 1;
   }

   // Set up all the registers except the command register.

   pio_outbyte( CB_DC, devCtrl );
   pio_outbyte( CB_FR, frReg );
   pio_outbyte( CB_SC, scReg );
   pio_outbyte( CB_SN, snReg );
   pio_outbyte( CB_CL, cylLow );
   pio_outbyte( CB_CH, cylHigh );
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

   WAIT400NS();

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

   while ( 1 )
   {
      status = pio_inbyte( CB_ASTAT );       // poll for not busy
      if ( ( status & CB_STAT_BSY ) == 0 )
         break;
      if ( tmr_chk_timeout() )               // time out yet ?
      {
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 51;
         dir = -1;   // command done
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
         reg_cmd_info.ec = 52;
         dir = -1;   // command done
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

         pio_rep_outword( CB_DATA, cpseg, cpoff, cpbc >> 1 );

         WAIT400NS();
      }
   }

   // Data transfer loop...
   // If there is no error, enter the data transfer loop.
   // First adjust the I/O buffer address so we are able to
   // transfer large amounts of data (more than 64K).

   
   dpaddr = dpoff;

   while ( reg_cmd_info.ec == 0 )
   {
      // Data transfer loop...
      // Wait for INT 7x -or- wait for not BUSY -or- wait for time out.

      sub_atapi_delay( dev );
      reg_wait_poll( 53, 54 );

      // Data transfer loop...
      // If there was a time out error, exit the data transfer loop.

      if ( reg_cmd_info.ec )
      {
         dir = -1;   // command done
         break;
      }

      // Data transfer loop...
      // Read the primary status register.

      status = pio_inbyte( CB_STAT );
      reason = pio_inbyte( CB_SC );
      lowCyl = pio_inbyte( CB_CL );
      highCyl = pio_inbyte( CB_CH );

      // Data transfer loop...
      // Exit the read data loop if the device indicates this
      // is the end of the command.

      if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == 0 )
      {
         dir = -1;   // command done
         break;
      }

      // Data transfer loop...
      // The device must want to transfer data...
      // check status: must have BSY=0, DRQ=1 now.

      if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) != CB_STAT_DRQ )
      {
         reg_cmd_info.ec = 55;
         dir = -1;   // command done
         break;
      }
      else
      {
         // Data transfer loop...
         // Check for protocol failures...
         // check: C/nD=0, IO=1 (read) or IO=0 (write).

         if ( ( reason &  ( CB_SC_P_TAG | CB_SC_P_REL ) )
              || ( reason &  CB_SC_P_CD )
            )
            reg_cmd_info.failbits |= FAILBIT4;
         if ( ( reason & CB_SC_P_IO ) && dir )
            reg_cmd_info.failbits |= FAILBIT5;
      }

      // do the slow data transfer thing

      if ( reg_slow_xfer_flag )
      {
         slowXferCntr ++ ;
         if ( slowXferCntr <= reg_slow_xfer_flag )
         {
            sub_xfer_delay();
            reg_slow_xfer_flag = 0;
         }
      }

      // Data transfer loop...
      // get the byte count, check for zero...

      byteCnt = ( highCyl << 8 ) | lowCyl;
      if ( byteCnt < 1 )
      {
         reg_cmd_info.ec = 60;
         dir = -1;   // command done
         break;
      }

      // Data transfer loop...
      // and check protocol failures...

      if ( byteCnt > dpbc )
         reg_cmd_info.failbits |= FAILBIT6;
      reg_cmd_info.failbits |= prevFailBit7;
      prevFailBit7 = 0;
      if ( byteCnt & 0x0001 )
         prevFailBit7 = FAILBIT7;

      // Data transfer loop...
      // make sure we don't overrun the caller's buffer

      if ( ( reg_cmd_info.totalBytesXfer + byteCnt ) > reg_atapi_max_bytes )
      {
         reg_cmd_info.ec = 59;
         dir = -1;   // command done
         break;
      }

      // increment number of DRQ packets

      reg_cmd_info.drqPackets ++ ;

      // Data transfer loop...
      // transfer the data and update the i/o buffer address
      // and the number of bytes transfered.

      wordCnt = ( byteCnt >> 1 ) + ( byteCnt & 0x0001 );
      reg_cmd_info.totalBytesXfer += ( wordCnt << 1 );

      dpoff = (unsigned int)dpaddr;
      
      if ( dir )
         pio_rep_outword( CB_DATA, dpseg, dpoff, wordCnt );
      else
         pio_rep_inword( CB_DATA, dpseg, dpoff, wordCnt );
      dpaddr = dpaddr + byteCnt;

      WAIT400NS();    // delay so device can get the status updated
   }

   // End of command...
   // Wait for interrupt or poll for BSY=0,
   // but don't do this if there was any error or if this
   // was a commmand that did not transfer data.

   if ( ( reg_cmd_info.ec == 0 ) && ( dir >= 0 ) )
   {
      sub_atapi_delay( dev );
      reg_wait_poll( 56, 57 );
   }

   // Final status check, only if no previous error.

   if ( reg_cmd_info.ec == 0 )
   {
      // Final status check...
      // Read the primary status register and other regs.

      status = pio_inbyte( CB_STAT );
      reason = pio_inbyte( CB_SC );
      lowCyl = pio_inbyte( CB_CL );
      highCyl = pio_inbyte( CB_CH );

      // Final status check...
      // check for any error.

      if ( status & ( CB_STAT_BSY | CB_STAT_DRQ | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 58;
      }
   }

   // Final status check...
   // Check for protocol failures...
   // check: C/nD=1, IO=1.

   if ( ( reason & ( CB_SC_P_TAG | CB_SC_P_REL ) )
        || ( ! ( reason & CB_SC_P_IO ) )
        || ( ! ( reason & CB_SC_P_CD ) )
      )
      reg_cmd_info.failbits |= FAILBIT8;

   // Done...
   // Read the output registers and trace the command.

   if ( ! reg_cmd_info.totalBytesXfer )
      reg_cmd_info.ct = TRC_TYPE_PND;
   sub_trace_command();

   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();

   // mark end of PI cmd in low level trace

 
   // All done.  The return values of this function are described in
   // ATAIO.H.

   reg_atapi_max_bytes = 32768L;    // reset max bytes
   if ( reg_cmd_info.ec )
      return 1;
   return 0;
};

/*
int reg_pio_data_in_lba28( int dev, int cmd,
                           unsigned int fr, unsigned int sc,
                           unsigned long lba,
                           unsigned int seg, unsigned int off,
                           long numSect, int multiCnt )

{

   // Reset error return data.

   sub_zero_return_data();
   reg_cmd_info.flg = TRC_FLAG_ATA;
   reg_cmd_info.ct  = TRC_TYPE_APDI;
   reg_cmd_info.cmd = cmd;
   reg_cmd_info.fr1 = fr;
   reg_cmd_info.sc1 = sc;
   reg_cmd_info.dh1 = CB_DH_LBA | (dev ? CB_DH_DEV1 : CB_DH_DEV0 );
   reg_cmd_info.dc1 = int_use_intr_flag ? 0 : CB_DC_NIEN;
   reg_cmd_info.lbaSize = LBA28;
   reg_cmd_info.lbaHigh1 = 0L;
   reg_cmd_info.lbaLow1 = lba;

   // these commands transfer only 1 sector
   if (    ( cmd == CMD_IDENTIFY_DEVICE )
        || ( cmd == CMD_IDENTIFY_DEVICE_PACKET )
        || ( cmd == CMD_READ_BUFFER )
      )
      numSect = 1;

   // only Read Multiple uses multiCnt
   if ( cmd != CMD_READ_MULTIPLE )
      multiCnt = 1;

   reg_cmd_info.ns  = numSect;
   reg_cmd_info.mc  = multiCnt;

   return exec_pio_data_in_cmd( dev, seg, off, numSect, multiCnt );
}

int exec_pio_data_in_cmd( int dev,
                            unsigned int seg, unsigned int off,
                            long numSect, int multiCnt )
{
   unsigned char status;
   long wordCnt;
   unsigned int saveSeg = seg;
   unsigned int saveOff = off;

   // mark start of PDI cmd in low level trace


   // Set command time out.

   tmr_set_timeout();

   // Select the drive - call the sub_select function.
   // Quit now if this fails.

   if ( sub_select( dev ) )
   {
      sub_trace_command();
      return 1;
   }

   // Set up all the registers except the command register.

   sub_setup_command();

   // For interrupt mode,
   // Take over INT 7x and initialize interrupt controller
   // and reset interrupt flag.

   int_save_int_vect();

   // Start the command by setting the Command register.  The drive
   // should immediately set BUSY status.

   pio_outbyte( CB_CMD, reg_cmd_info.cmd );

   // Waste some time by reading the alternate status a few times.
   // This gives the drive time to set BUSY in the status register on
   // really fast systems.  If we don't do this, a slow drive on a fast
   // system may not set BUSY fast enough and we would think it had
   // completed the command when it really had not even started the
   // command yet.

   WAIT400NS();
   // Loop to read each sector.

   while ( 1 )
   {
      // READ_LOOP:
      //
      // NOTE NOTE NOTE ...  The primary status register (1f7) MUST NOT be
      // read more than ONCE for each sector transferred!  When the
      // primary status register is read, the drive resets IRQ 14.  The
      // alternate status register (3f6) can be read any number of times.
      // After INT 7x read the the primary status register ONCE
      // and transfer the 256 words (REP INSW).  AS SOON as BOTH the
      // primary status register has been read AND the last of the 256
      // words has been read, the drive is allowed to generate the next
      // IRQ 14 (newer and faster drives could generate the next IRQ 14 in
      // 50 microseconds or less).  If the primary status register is read
      // more than once, there is the possibility of a race between the
      // drive and the software and the next IRQ 14 could be reset before
      // the system interrupt controller sees it.

      // Wait for INT 7x -or- wait for not BUSY -or- wait for time out.

      sub_atapi_delay( dev );
      reg_wait_poll( 34, 35 );

      // Read the primary status register.  In keeping with the rules
      // stated above the primary status register is read only
      // ONCE.

      status = pio_inbyte( CB_STAT );

      // If there was a time out error, go to READ_DONE.

      if ( reg_cmd_info.ec )
         break;   // go to READ_DONE

      // If BSY=0 and DRQ=1, transfer the data,
      // even if we find out there is an error later.

      if ( ( status & ( CB_STAT_BSY | CB_STAT_DRQ ) ) == CB_STAT_DRQ )
      {
         // do the slow data transfer thing

         if ( reg_slow_xfer_flag )
         {
            if ( numSect <= reg_slow_xfer_flag )
            {
               sub_xfer_delay();
               reg_slow_xfer_flag = 0;
            }
         }

         // increment number of DRQ packets

         reg_cmd_info.drqPackets ++ ;

         // determine the number of sectors to transfer

         wordCnt = multiCnt ? multiCnt : 1;
         if ( wordCnt > numSect )
            wordCnt = numSect;
         wordCnt = wordCnt * 256;

         // Quit if buffer overrun.

         if ( ( ! reg_drq_block_call_back )
              &&
              ( ( reg_cmd_info.totalBytesXfer + ( wordCnt << 1 ) )
                > reg_buffer_size )
            )
         {
            reg_cmd_info.ec = 61;
            break;   // go to READ_DONE
         }

         // Do the REP INSW to read the data for one DRQ block.
         reg_cmd_info.totalBytesXfer += ( wordCnt << 1 );
         pio_drq_block_in( CB_DATA, seg, off, wordCnt );

         WAIT400NS();

         // Note: The drive should have dropped DATA REQUEST by now.  If there
         // are more sectors to transfer, BUSY should be active now (unless
         // there is an error).

         // Decrement the count of sectors to be transferred
         // and increment buffer address.

         numSect = numSect - ( multiCnt ? multiCnt : 1 );
         seg = seg + ( 32 * ( multiCnt ? multiCnt : 1 ) );
      }

      // So was there any error condition?

      if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_ERR ) )
      {
         reg_cmd_info.ec = 31;
         trc_llt( 0, reg_cmd_info.ec, TRC_LLT_ERROR );
         break;   // go to READ_DONE
      }

      // DRQ should have been set -- was it?

      if ( ( status & CB_STAT_DRQ ) == 0 )
      {
         reg_cmd_info.ec = 32;
         break;   // go to READ_DONE
      }

      // If all of the requested sectors have been transferred, make a
      // few more checks before we exit.

      if ( numSect < 1 )
      {
         // Since the drive has transferred all of the requested sectors
         // without error, the drive should not have BUSY, DEVICE FAULT,
         // DATA REQUEST or ERROR active now.

         sub_atapi_delay( dev );
         status = pio_inbyte( CB_STAT );
         if ( status & ( CB_STAT_BSY | CB_STAT_DF | CB_STAT_DRQ | CB_STAT_ERR ) )
         {
            reg_cmd_info.ec = 33;
            break;   // go to READ_DONE
         }

         // All sectors have been read without error, go to READ_DONE.

         break;   // go to READ_DONE

      }

      // This is the end of the read loop.  If we get here, the loop is
      // repeated to read the next sector.  Go back to READ_LOOP.

   }

   // read the output registers and trace the command.

   sub_trace_command();

   // READ_DONE:

   // For interrupt mode, restore the INT 7x vector.

   int_restore_int_vect();
  // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}
*/
// end ataioreg.c
