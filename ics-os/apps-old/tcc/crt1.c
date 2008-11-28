#include "dexsdk.h"

int _start() 
{
   char *s;
   int c=0;
   char *p[100];
   char params[500];
   
   getparameters(params);
   
   s=strtok(params," ");
   
   do {
        p[c]=s;
        c++;
        s=strtok(0," ");
   } while (s!=0);
   
   main(c,p);
   exit(0);
   return 0;
}
