/*
  Name: bitmap
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 20/01/00 07:22
  Description: .BMP format interpreter
*/

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L


typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER { // bmih
   DWORD  biSize;
   long   biWidth;
   long   biHeight;
   WORD   biPlanes;
   WORD   biBitCount;
   DWORD  biCompression;
   DWORD  biSizeImage;
   long   biXPelsPerMeter;
   long   biYPelsPerMeter;
   DWORD  biClrUsed;
   DWORD  biClrImportant;
} BITMAPINFOHEADER;


typedef struct tagRGBQUAD { // rgbq
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved;
} RGBQUAD;

void bmp_normaldecode(BYTE *source,int height,int width,int sx,int sy)
{
 for (y = 0; y < height; y++)
   for (x=0;x < width;x++)
        putpixel( x+sx, y+sy, source[ (width)*(height-y-1)+x]);
};
       
void bmp_RLEdecodeToScreen(BYTE *source,DWORD bmHeight,DWORD srclength,int sx, int sy)
 {
   DWORD i=0,d=0,yadj=bmHeight-1;
   int x=0,y=0;
   for (i=0;i<srclength;i++)
    {

       if (source[i]!=0)
         {
           int i2;
           DWORD numrepeat=source[i];
           BYTE valtorepeat;
           i++;
           valtorepeat=source[i];

           for (i2=0;i2<numrepeat;i2++)
             {
                  putpixel(x++,yadj-y,valtorepeat);
             };
         }
           else
         {
           i++;
           if (source[i]==0) //End Of Line
                   { y++;x=0;}
             else
           if (source[i]==1)  //End of Bitmap
                return;
              else
           if (source[i]==2)
               {
                  int dx,dy;
                  i++;
                  dx=source[i];
                  i++;
                  dy=source[i];
                  x+=dx;y+=dy;
               }
              else //Absolute mode
               {
                  int f,mx,i2;
                  f=source[i]; //get number of bytes that will follow
                  mx=f%2;      //get adjustment value since each
                               //run must be aligned to a word boundary
                  for (i2=0;i2<f;i2++)
                   {
                     i++;
                     putpixel(x++,yadj-y,source[i]);
                   };
                  i+=mx; //adjust pointer

               };

         };
    };
 };

