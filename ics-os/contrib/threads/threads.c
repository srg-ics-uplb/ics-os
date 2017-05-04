
#include "../../sdk/dexsdk.h"


#define SSIZE 16

void *f();

unsigned char stk;

int main() 
{
    stk=malloc(SSIZE);
    printf("Creating and starting thread...");
	dexsdk_systemcall(0xB,(void *)f,&stk,SSIZE,0,0);
}

void *f(){
    printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    
/*
	FILE *fp=fopen("threads.txt","w");
	fputs(fp,"hello");
	fclose(fp);
*/
}

