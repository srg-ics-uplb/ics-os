
#include "../../sdk/dexsdk.h"


void *f();

unsigned int stk;

int main() 
{
	dexsdk_systemcall(0xB,(unsigned int)&f,(unsigned int)&stk,(unsigned int)20,0,0);
}

void *f(){
/*
	FILE *fp=fopen("threads.txt","w");
	fputs(fp,"hello");
	fclose(fp);
*/
}

