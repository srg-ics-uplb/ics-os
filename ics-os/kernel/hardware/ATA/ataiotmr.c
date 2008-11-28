//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOTMR.C
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
// This C source file contains functions to access the BIOS
// Time of Day information and to set and check the command
// time out period.
//********************************************************************


//**************************************************************

long tmr_time_out = 20L;      // max command execution time in seconds

long tmr_cmd_time_out_time;   // command timeout time - see the
                              // tmr_set_timeout() and
                              // tmr_chk_timeout() functions.

//**************************************************************

static long prevTime = -1;    // previous time

//*************************************************************
//
// tmr_read_bios_timer() - function to read the BIOS timer
//
//**************************************************************

long tmr_read_bios_timer( void )

{
   long curTime;
   // loop so we get a valid value without
   // turning interrupts off and on again
   curTime = (time()*18)/100 ;
   return curTime;
}

//*************************************************************
//
// tmr_set_timeout() - function used to set command timeout time
//
// The command time out time is computed as follows:
//
//    timer + ( tmr_time_out * 18 )
//
//**************************************************************

void tmr_set_timeout( void )

{

   // first time initialize
   if ( prevTime < 0 )
      prevTime = tmr_read_bios_timer();

   // get value of BIOS timer
   tmr_cmd_time_out_time = tmr_read_bios_timer();

   // add command timeout value
   tmr_cmd_time_out_time = tmr_cmd_time_out_time + ( tmr_time_out * 18L );
}

//*************************************************************
//
// tmr_chk_timeout() - function used to check for command timeout.
//
// Gives non-zero return if command has timed out.
//
//**************************************************************

int tmr_chk_timeout( void )

{
   long curTime;

   #define MIDNIGHT 1573040L

   // get current time
   curTime = tmr_read_bios_timer();

   // if we just pasted midnight, adjust command time out time
   if ( ( curTime < prevTime ) && ( tmr_cmd_time_out_time >= MIDNIGHT ) )
      tmr_cmd_time_out_time = tmr_cmd_time_out_time - MIDNIGHT;

   // save the new time value for next call
   prevTime = curTime;

   // timed out yet ?
   if ( curTime >= tmr_cmd_time_out_time )
      return 1;      // yes

   // no timeout yet
   return 0;
}

// end ataiotmr.c
