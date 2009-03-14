

// commands for PS2
#define ENABLE_PS2   0xA8
#define KBD_STAT   0x64
#define MOUSE      0xD4
#define MOUSE_STREAM   0xF4
#define MOUSE_DISABLE   0xF5
#define KBD_CMD      0x60
#define DISABLE_KBD   0xAD
#define ENABLE_KBD   0xAE

// checks the port
void CheckPort()
{
   unsigned char temp;
   while( 1 )
   {
      temp = inportb( KBD_STAT );
      if( (  temp & 2 ) == 0 )
         break;
   }
}

// waits for the ouse buffer to be full
void MouseBufferFull()
{
   unsigned char temp;
   while( 1 )
   {
      temp = inportb( KBD_STAT );
      if( (  temp & 0x20 ) == 0 )
         break;
   }
}

// checks the mouse
char CheckMouse()
{
   unsigned char temp;
   temp = inportb( KBD_STAT );

   if( temp & 1 )
      return 0;
   else
      return 1;
}

// set the ps2 bytes
void PS2Set()
{
   // mouse enabled
   outportb( KBD_STAT, ENABLE_PS2 );
   CheckPort();
}

// waits for the mouse
void WaitMouse()
{
   outportb( KBD_STAT, MOUSE );
   CheckPort();
}

// sets the streaming mode
void StartStream()
{
   WaitMouse();
   outportb( KBD_CMD, MOUSE_STREAM );
   CheckPort();
   CheckMouse();
}

// disables the keyboard
void DisableKeyboard()
{
   outportb( KBD_STAT, DISABLE_KBD );
   CheckPort();
}

// enables the keyboard
void EnableKeyboard()
{
   outportb( KBD_STAT, ENABLE_KBD );
   CheckPort();
}

// wait for the mouse - this is in the OSFAQ (http://www.osdev.org/osfaq2)
void MouseWait( char thetype )
{
   long timeout = 100000;
   if( thetype == 0 )
   {
      while( timeout-- )
      {
         if( inportb( 0x64 ) & 1 )
         {
            return;
         }
      }
      return;
   }
   if( thetype == 1 )
   {
      while( timeout-- )
      {
         if( ( inportb( 0x64 ) & 2 ) == 0 )
         {
            return;
         }
      }
      return;
   }
}

// get a byte from the port
char GetByte()
{
   MouseWait( 0 );
   char ret = inportb( 0x60 );

   return ret;
}

// info
int readable = 0;

// data pointer
MOUSEDATA LocalData;

// irq handler for the mouse
void mouse_irq(void)
{
   DWORD flags;
   dex32_stopints(&flags);

   LocalData.status = GetByte();
   LocalData.xcoord = GetByte();
   LocalData.ycoord = GetByte();

   readable = 1;
   dex32_restoreints(&flags);
}

// installs the IRQ
void InstallMouseIRQ()
{
   // enable the irq
   outportb( 0x64, 0x20 );
   //KeyContWaitReady();
   unsigned char c = inportb( 0x60 ) | 2;
   outportb( 0x64, 0x60 );
   //KeyContWaitReady();
   outportb( 0x60, c );

   // install the irq handler
   //irq_install_handler( 12, MouseIRQ );
}

// get data from the mouse
void GetMouseData( MOUSEDATA* data )
{
   // is there data?
   if( ! readable )
   {
      // there's no data yet, so just make no movement
      data->status = 0;
      data->xcoord = 0;
      data->ycoord = 0;
      
      // go back to caller
      return;
   }
   
   // disable interrupts
   __asm__ __volatile__ ( "cli" );

   // readable is false now
   readable = 0;

   // get the important stuff
   LocalData.leftbut = LocalData.status & 1;
   LocalData.rightbut = ( LocalData.status & 2 ) >> 1;
   LocalData.midbut = ( LocalData.status & 4 ) >> 2;
   LocalData.xsign = ( LocalData.status & 16 ) >> 4;
   LocalData.ysign = ( LocalData.status & 32 ) >> 5;
   LocalData.xover = ( LocalData.status & 64 ) >> 6;
   LocalData.yover = ( LocalData.status & 128 ) >> 7;

   // return the data
   *data = LocalData;
   
   // enable interrupts
   __asm__ __volatile__ ( "sti" );
}

// init the mouse
void InitMouse()
{
   PS2Set();
   //InstallMouseIRQ();
   StartStream();
}
