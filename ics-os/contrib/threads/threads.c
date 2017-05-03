
#include "../../sdk/dexsdk.h"


void *f();


int main() 
{
	int *stk=malloc(100);
	dexsdk_systemcall(0xB,(int)&f,(int)&stk,(int)20,0,0);
}

void *f(){
	FILE *fp=fopen("threads.txt","w");
	fputs(fp,"hello");
	fclose(fp);

}

