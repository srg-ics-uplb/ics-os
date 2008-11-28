//********************************************************************
// ATA LOW LEVEL I/O DRIVER -- ATAIOINT.C
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
// Global interrupt mode data
//
//*************************************************************

int int_use_intr_flag = 0;    // use INT mode if != 0

int int_irq_number = 0;       // IRQ number in use -- only
                              // IRQ's 8-15 are supported.

int int_int_vector = 0;       // INT vector in use -- only
                              // INT vectors 70H to 77H
                              // are supported.

volatile int int_intr_flag;   // interrupt flag -- incremented
                              // each time there is an INT

//*************************************************************
//
// Local data
//
//*************************************************************

static int full_time_mode = 0;   // full time mode flag

static int got_it_now = 0;       // have INT 7x now flag

#define  PIC0_ADDR 0x21          // PIC0 address
#define  PIC1_ADDR 0xA1          // PIC1 address

#define  PIC0_ENABLE_PIC1 0xFB   // mask to enable PIC1

static pic1_enable_irq[8] =      // mask to enable
   { 0xFE, 0xFD, 0xFB, 0xF7,     // IRQ 8-15
     0xEF, 0xDF, 0xBF, 0x7F  };  // in PIC 1

//*************************************************************
//
// INT 7x (IRQ 8-15) Interrupt Handler.
//
// Increment the interrupt flag and send End of Interrupt
// to the PIC's.
//
// NOTE: THIS CODE WORKS ONLY WITH ATA CONTROLLERS THAT ARE
// IN LEGACY MODE.
//
//*************************************************************

static void irqhandler( void )

{
   #define IH1_CONTROL 0x20      // int controller 1 address
   #define IH2_CONTROL 0xa0      // int controller 2 address
   #define IH_EOI      0x20      // end of interrupt

   // Increment the interrupt flag;

   int_intr_flag ++ ;

   // Send End-of-Interrupt to the system interrupt controllers.

   outportb( IH1_CONTROL, IH_EOI );
   outportb( IH2_CONTROL, IH_EOI );
}

//*************************************************************
//
// Enable interrupt mode -- get the IRQ number we are using.
//
// The function MUST be called before interrupt mode can
// be used!
//
// If this function is called then the int_disable_irq()
// function MUST be called before exiting to DOS.
//
//*************************************************************

int int_enable_irq( int irqNum )

{
   /*Not needed in DEX*/ 
   return 0;
}

//*************************************************************
//
// Disable interrupt mode.
//
// If the int_enable_irq() function has been called,
// this function MUST be called before exiting to DOS.
//
//*************************************************************

void int_disable_irq( void )

{
    /*Not needed in DEX*/
}

//*************************************************************
//
// Take over the INT 7x vector.
//
// Interrupt mode MUST be setup by calling int_enable_irq()
// before calling this function.
//
//*************************************************************

void int_save_int_vect( void )

{
    /*Not needed in DEX*/
}

//*************************************************************
//
// Restore the INT 7x vector.
//
// Interrupt mode MUST be setup by calling int_enable_irq()
// before calling this function.
//
//*************************************************************

void int_restore_int_vect( void )

{
    /*Some clean-up function?*/
}

// end ataioint.c
