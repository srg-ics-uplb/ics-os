//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOSUB.C
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
//
// This C source contains the low level interrupt set up and
// interrupt handler functions.
//********************************************************************


//*************************************************************
//
// sub_zero_return_data() -- zero the return data areas.
//
//*************************************************************

void sub_zero_return_data( void )

{
   int ndx;

   for ( ndx = 0; ndx < sizeof( reg_cmd_info ); ndx ++ )
      ( (unsigned char *) & reg_cmd_info )[ndx] = 0;
}

//*************************************************************
//
// sub_setup_command() -- setup the command parameters
//                        in FR, SC, SN, CL, CH and DH.
//
//*************************************************************

void sub_setup_command( void )

{
   unsigned char fr48[2];
   unsigned char sc48[2];
   unsigned char lba48[8];

   // WARNING: THIS CODE IS DESIGNED FOR A STUPID PROCESSOR
   // LIKE INTEL X86 THAT IS Little-Endian, THAT IS, A
   // PROCESSOR THAT STORES DATA IN MEMORY IN THE WRONG
   // BYTE ORDER !!!

   * (unsigned int *) fr48 = reg_cmd_info.fr1;
   * (unsigned int *) sc48 = reg_cmd_info.sc1;
   * (unsigned long *) ( lba48 + 4 ) = reg_cmd_info.lbaHigh1;
   * (unsigned long *) ( lba48 + 0 ) = reg_cmd_info.lbaLow1;

   pio_outbyte( CB_DC, reg_cmd_info.dc1 );

   if ( reg_cmd_info.lbaSize == LBA28 )
   {
      // in ATA LBA28 mode
      pio_outbyte( CB_FR, fr48[0] );
      pio_outbyte( CB_SC, sc48[0] );
      reg_cmd_info.sn1 = lba48[0];
      pio_outbyte( CB_SN, lba48[0] );
      reg_cmd_info.cl1 = lba48[1];
      pio_outbyte( CB_CL, lba48[1] );
      reg_cmd_info.ch1 = lba48[2];
      pio_outbyte( CB_CH, lba48[2] );
      pio_outbyte( CB_DH, ( reg_cmd_info.dh1 & 0xf0 ) | ( lba48[3] & 0x0f ) );
   }
   else
   if ( reg_cmd_info.lbaSize == LBA48 )
   {
      // in ATA LBA48 mode
      pio_outbyte( CB_FR, fr48[1] );
      pio_outbyte( CB_SC, sc48[1] );
      pio_outbyte( CB_SN, lba48[3] );
      pio_outbyte( CB_CL, lba48[4] );
      pio_outbyte( CB_CH, lba48[5] );
      pio_outbyte( CB_FR, fr48[0] );
      pio_outbyte( CB_SC, sc48[0] );
      reg_cmd_info.sn1 = lba48[0];
      pio_outbyte( CB_SN, lba48[0] );
      reg_cmd_info.cl1 = lba48[1];
      pio_outbyte( CB_CL, lba48[1] );
      reg_cmd_info.ch1 = lba48[2];
      pio_outbyte( CB_CH, lba48[2] );
      pio_outbyte( CB_DH, reg_cmd_info.dh1  );
   }
   else
   {
      // in ATA CHS or ATAPI LBA32 mode
      pio_outbyte( CB_FR, reg_cmd_info.fr1  );
      pio_outbyte( CB_SC, reg_cmd_info.sc1  );
      pio_outbyte( CB_SN, reg_cmd_info.sn1  );
      pio_outbyte( CB_CL, reg_cmd_info.cl1  );
      pio_outbyte( CB_CH, reg_cmd_info.ch1  );
      pio_outbyte( CB_DH, reg_cmd_info.dh1  );
   }
}

//*************************************************************
//
// sub_trace_command() -- trace the end of a command.
//
//*************************************************************

void sub_trace_command( void )

{
   unsigned long lba;
   unsigned char sc48[2];
   unsigned char lba48[8];

   reg_cmd_info.st2 = pio_inbyte( CB_STAT );
   reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
   reg_cmd_info.er2 = pio_inbyte( CB_ERR );
   if ( reg_cmd_info.lbaSize == LBA48 )
   {
      // read back ATA LBA48...
      sc48[0]  = pio_inbyte( CB_SC );
      lba48[0] = pio_inbyte( CB_SN );
      lba48[1] = pio_inbyte( CB_CL );
      lba48[2] = pio_inbyte( CB_CH );
      pio_outbyte( CB_DC, CB_DC_HOB );
      sc48[1]  = pio_inbyte( CB_SC );
      lba48[3] = pio_inbyte( CB_SN );
      reg_cmd_info.sn2 = lba48[3];
      lba48[4] = pio_inbyte( CB_CL );
      reg_cmd_info.cl2 = lba48[4];
      lba48[5] = pio_inbyte( CB_CH );
      reg_cmd_info.ch2 = lba48[5];
      lba48[6] = 0;
      lba48[7] = 0;
      reg_cmd_info.sc2 = * (unsigned int *) sc48;
      reg_cmd_info.lbaHigh2 = * (unsigned long *) ( lba48 + 4 );
      reg_cmd_info.lbaLow2  = * (unsigned long *) ( lba48 + 0 );
      reg_cmd_info.dh2 = pio_inbyte( CB_DH );
   }
   else
   {
      // read back ATA CHS, ATA LBA28 or ATAPI LBA32
      reg_cmd_info.sc2 = pio_inbyte( CB_SC );
      reg_cmd_info.sn2 = pio_inbyte( CB_SN );
      reg_cmd_info.cl2 = pio_inbyte( CB_CL );
      reg_cmd_info.ch2 = pio_inbyte( CB_CH );
      reg_cmd_info.dh2 = pio_inbyte( CB_DH );
      reg_cmd_info.lbaHigh2 = 0;
      reg_cmd_info.lbaLow2 = 0;
      if ( reg_cmd_info.lbaSize == LBA28 )
      {
         lba = reg_cmd_info.dh2 & 0x0f;
         lba = lba << 8;
         lba = lba | reg_cmd_info.ch2;
         lba = lba << 8;
         lba = lba | reg_cmd_info.cl2;
         lba = lba << 8;
         lba = lba | reg_cmd_info.sn2;
         reg_cmd_info.lbaLow2 = lba;
      }
   }
}

//*************************************************************
//
// sub_select() - function used to select a drive.
//
// Function to select a drive. This subroutine waits for not BUSY,
// selects a drive and waits for READY and SEEK COMPLETE status.
//
//**************************************************************

int sub_select( int dev )

{
   unsigned char status;

   // PAY ATTENTION HERE
   // The caller may want to issue a command to a device that doesn't
   // exist (for example, Exec Dev Diag), so if we see this,
   // just select that device, skip all status checking and return.
   // We assume the caller knows what they are doing!

   if ( reg_config_info[ dev ] < REG_CONFIG_TYPE_ATA )
   {
      // select the device and return

      pio_outbyte( CB_DH, dev ? CB_DH_DEV1 : CB_DH_DEV0 );
      delay(4);
      return 0;
   }

   // The rest of this is the normal ATA stuff for device selection
   // and we don't expect the caller to be selecting a device that
   // does not exist.
   // We don't know which drive is currently selected but we should
   // wait for it to be not BUSY.  Normally it will be not BUSY
   // unless something is very wrong!

   while ( 1 )
   {
      status = pio_inbyte( CB_STAT );
      if ( ( status & CB_STAT_BSY ) == 0 )
         break;
      if ( tmr_chk_timeout() )
      {
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 11;
         reg_cmd_info.st2 = status;
         reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
         reg_cmd_info.er2 = pio_inbyte( CB_ERR );
         reg_cmd_info.sc2 = pio_inbyte( CB_SC );
         reg_cmd_info.sn2 = pio_inbyte( CB_SN );
         reg_cmd_info.cl2 = pio_inbyte( CB_CL );
         reg_cmd_info.ch2 = pio_inbyte( CB_CH );
         reg_cmd_info.dh2 = pio_inbyte( CB_DH );
         return 1;
      }
   }

   // Here we select the drive we really want to work with by
   // putting 0xA0 or 0xB0 in the Drive/Head register (1f6).

   pio_outbyte( CB_DH, dev ? CB_DH_DEV1 : CB_DH_DEV0 );
   delay(4);

   // If the selected device is an ATA device,
   // wait for it to have READY and SEEK COMPLETE
   // status.  Normally the drive should be in this state unless
   // something is very wrong (or initial power up is still in
   // progress).  For any other type of device, just wait for
   // BSY=0 and assume the caller knows what they are doing.

   while ( 1 )
   {
      status = pio_inbyte( CB_STAT );
      if ( reg_config_info[ dev ] == REG_CONFIG_TYPE_ATA )
      {
           if ( ( status & ( CB_STAT_BSY | CB_STAT_RDY | CB_STAT_SKC ) )
                     == ( CB_STAT_RDY | CB_STAT_SKC ) )
         break;
      }
      else
      {
         if ( ( status & CB_STAT_BSY ) == 0 )
            break;
      }
      if ( tmr_chk_timeout() )
      {
         reg_cmd_info.to = 1;
         reg_cmd_info.ec = 12;
         reg_cmd_info.st2 = status;
         reg_cmd_info.as2 = pio_inbyte( CB_ASTAT );
         reg_cmd_info.er2 = pio_inbyte( CB_ERR );
         reg_cmd_info.sc2 = pio_inbyte( CB_SC );
         reg_cmd_info.sn2 = pio_inbyte( CB_SN );
         reg_cmd_info.cl2 = pio_inbyte( CB_CL );
         reg_cmd_info.ch2 = pio_inbyte( CB_CH );
         reg_cmd_info.dh2 = pio_inbyte( CB_DH );
         return 1;
      }
   }

   // All done.  The return values of this function are described in
   // ATAIO.H.

   if ( reg_cmd_info.ec )
      return 1;
   return 0;
}

//*************************************************************
//
// sub_atapi_delay() - delay for at least two ticks of the bios
//                     timer (or at least 110ms).
//
//*************************************************************

void sub_atapi_delay( int dev )

{
   int ndx;
   long lw;

   if ( reg_config_info[dev] != REG_CONFIG_TYPE_ATAPI )
      return;
   if ( ! reg_atapi_delay_flag )
      return;
   for ( ndx = 0; ndx < 3; ndx ++ )
   {
      lw = tmr_read_bios_timer();
      while ( lw == tmr_read_bios_timer() )
         /* do nothing */ ;
   }
}

//*************************************************************
//
// sub_xfer_delay() - delay until the bios timer ticks
//                    (from 0 to 55ms).
//
//*************************************************************

void sub_xfer_delay( void )

{
   long lw;
   lw = tmr_read_bios_timer();
   while ( lw == tmr_read_bios_timer() )
      /* do nothing */ ;
}

// end ataiosub.c
