//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOPD.H (driver's private data)
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
// This C source file is the internal header file for the for this
// driver and is used in the ATAIOxxx.C files.  It describes data that
// is internal to the driver.  This file should NOT be included by
// any program using this driver code.
//********************************************************************

// This macro provides a small delay that is used in several
// places in the ATA command protocols:
// 1) It is recommended that the host delay 400ns after
//    writing the command register.
// 2) ATA-4 has added a new requirement that the host delay
//    400ns if the DEV bit in the Device/Head register is
//    changed.  This was not recommended or required in ATA-1,
//    ATA-2 or ATA-3.  This is the easy way to do that since it
//    works in all PIO modes.
// 3) ATA-4 has added another new requirement that the host delay
//    after the last word of a data transfer before checking the
//    status register.  This was not recommended or required in
//    ATA-1, ATA-2 or ATA-3.  This is the easy to do that since it
//    works in all PIO modes.

#define DELAY400NS  { pio_inbyte( CB_ASTAT ); pio_inbyte( CB_ASTAT );  \
                      pio_inbyte( CB_ASTAT ); pio_inbyte( CB_ASTAT ); }

//**************************************************************

// Private data in ATAIOINT.C

// Interrupt 7x flag (!= 0 if there was an INT)

extern volatile int int_intr_flag;

//**************************************************************

// Private functions in ATAIOINT.C

extern void int_save_int_vect( void );

extern void int_restore_int_vect( void );

//**************************************************************

// Private data in ATAIOSUB.C

//**************************************************************

// Private functions in ATAIOSUB.C

extern void sub_zero_return_data( void );
extern void sub_trace_command( void );
extern int sub_select( int dev );
extern void sub_atapi_delay( int dev );
extern void sub_xfer_delay( void );

//**************************************************************

// Private data in ATAIOTRC.C

// low level trace entry type values

#define TRC_LLT_NONE     0  // unused entry
#define TRC_LLT_INB      1  // in byte
#define TRC_LLT_OUTB     2  // out byte
#define TRC_LLT_INW      3  // in word
#define TRC_LLT_OUTW     4  // out word
#define TRC_LLT_INSB     5  // rep in byte
#define TRC_LLT_OUTSB    6  // rep out byte
#define TRC_LLT_INSW     7  // rep in word
#define TRC_LLT_OUTSW    8  // rep out word
#define TRC_LLT_INSD     9  // rep in dword
#define TRC_LLT_OUTSD   10  // rep out dword
#define TRC_LLT_S_CFG   11  // start config
#define TRC_LLT_S_RST   12  // start reset
#define TRC_LLT_S_ND    13  // start ND cmd
#define TRC_LLT_S_PDI   14  // start PDI cmd
#define TRC_LLT_S_PDO   15  // start PDO cmd
#define TRC_LLT_S_PI    16  // start packet cmd
#define TRC_LLT_S_RWD   17  // start ata R/W DMA cmd
#define TRC_LLT_S_PID   18  // start packet DMA cmd
#define TRC_LLT_WINT    21  // wait for int
#define TRC_LLT_PNBSY   22  // poll for not busy
#define TRC_LLT_PRDY    23  // poll for ready
#define TRC_LLT_TOUT    24  // timeout
#define TRC_LLT_ERROR   25  // error detected
#define TRC_LLT_P_CMD   26  // packet cmd code
#define TRC_LLT_DELAY   27  // delay ~110ms
#define TRC_LLT_DEBUG   28  // debug data
#define TRC_LLT_DELAY2  29  // delay 0-55ms
#define TRC_LLT_E_CFG   31  // end   config
#define TRC_LLT_E_RST   32  // end   reset
#define TRC_LLT_E_ND    33  // end   ND cmd
#define TRC_LLT_E_PDI   34  // end   PDI cmd
#define TRC_LLT_E_PDO   35  // end   PDO cmd
#define TRC_LLT_E_PI    36  // end   packet CMD
#define TRC_LLT_E_RWD   37  // end   ata R/W DMA cmd
#define TRC_LLT_E_PID   38  // end   packet DMA cmd
#define TRC_LLT_DMA1    41  // enable/start DMA channel
#define TRC_LLT_DMA2    42  // poll the DMA TC bit
#define TRC_LLT_DMA3    43  // disable/stop DMA channel
#define TRC_LLT_PSERV   51  // poll for SERV=1
#define TRC_LLT_S_SC    52  // start Service command
#define TRC_LLT_E_SC    53  // end Service command

//**************************************************************

// Private functions in ATAIOTRC.C

extern void trc_cht( void );

extern void trc_llt( unsigned char addr,
                     unsigned char data,
                     unsigned char type );

//**************************************************************

// end ataiopd.h
