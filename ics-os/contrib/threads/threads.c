
#include "../../sdk/dexsdk.h"


#define SSIZE 200000

void *f();


int main() 
{
    printf("Creating and starting thread...");
    return thread_create(&f);
}

void *f(){
    printf("Thread!");
    
/*
	FILE *fp=fopen("threads.txt","w");
	fputs(fp,"hello");
	fclose(fp);
*/
}

