//Dump data in hex format
//jachermocilla@gmail.com

#include "../../sdk/dexsdk.h"

//https://gist.githubusercontent.com/ccbrown/9722406/raw/05202cd8f86159ff09edc879b70b5ac6be5d25d0/DumpHex.c
void DumpHex(const void* data, size_t size) {
   char ascii[17];
   size_t i, j;
   ascii[16] = '\0';
   char ch;

   int lines=0;


   for (i = 0; i < size; ++i) {

      printf("%02X ", ((unsigned char*)data)[i]);

      if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
         ascii[i % 16] = ((unsigned char*)data)[i];
      } else {
         ascii[i % 16] = '.';
      }

      if ((i+1) % 8 == 0 || i+1 == size) {

         printf(" ");
         if ((i+1) % 16 == 0) {
            printf("|  %s \n", ascii);

            if ((i+1)%16==0)
               lines++;
            if ((lines%20)==0){
               printf("Press any key to continue. 'q' to quit.\n");
               if ((ch=getch())=='q')
                  break; 
               
            }

         } else if (i+1 == size) {
            ascii[(i+1) % 16] = '\0';
            if ((i+1) % 16 <= 8) {
               printf(" ");
            }
            for (j = (i+1) % 16; j < 16; ++j) {
               printf("   ");
            }
            printf("|  %s \n", ascii);

            if ((i+1)%16==0)
               lines++;
            if ((lines%20)==0){
               printf("Press any key to continue. 'q' to quit.\n");
               if ((ch=getch())=='q')
                  break; 
            }

         }
      }

   }
}

int main(int argc, char *argv[]) {
   FILE *fp;
   int fsize;
   char *buff;

   printf("Hex Dump by jachermocilla@gmail.com\n");
   if (argc < 2){
      printf("Usage: hxdmp.exe <filename>\n");
      exit(1); 
   }

   if ((fp = fopen(argv[1],"r"))==NULL){
      printf("File not found.\n");
      exit(1);
   }

   fseek(fp, 0, SEEK_SET);
   fseek(fp, 0, SEEK_END);
   fsize=ftell(fp);
   fseek(fp, 0, SEEK_SET);
   buff = (char *)malloc(fsize);
   fread(buff, fsize,1,fp);
   fclose(fp);
   DumpHex((char *)buff,fsize);
   free(buff);
   return 0;
}
