
#include "../../sdk/dexsdk.h"



int i=10;

void f(){
   i++; 
}

int main() 
{
    printf("i=%d\n",i);
    printf("Creating and starting thread...\n");
    thread_create((void *)f);
    printf("i=%d\n",i);
   
}


