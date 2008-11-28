/*
  Name: sample.c
  Copyright: 
  Author: 
  Date: 21/04/04 18:09
  Description: A sample C program for tcc and for creating DEX applications
*/

#include "dexsdk.h"

int main() 
{
   int i;
   clrscr();

   for (i=0; i< 5; i++)
      {
       textcolor(i+1);
       printf("Hello Glorious World!\n");
      };
   printf("Welcome to the dex-os platform....\n");
   getch();
   return 0;
}
