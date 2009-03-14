typedef struct tagMOUSEDATA
{
   char xover;
   char yover;
   char xsign;
   char ysign;
   char midbut;
   char rightbut;
   char leftbut;
   char status;
   char xcoord;
   char ycoord;
} MOUSEDATA;

// init the mouse
void InitMouse();

// get data from the mouse
void GetMouseData( MOUSEDATA* data );
