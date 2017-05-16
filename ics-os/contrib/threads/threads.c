
#include "../../sdk/dexsdk.h"



int f();

int i=10;

int main() 
{
    printf("i=%d\n",i);
    printf("Creating and starting thread...\n");
    thread_create(&f);
    printf("i=%d\n",i);
   
}

int f(){
   i++; 
   return 0;
}

