#include "../../sdk/dexsdk.h"
#include "../../kernel/dextypes.h"


#include "lzozip.h"

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(workmem,LZO1X_1_MEM_COMPRESS);

void lzo_decompress(char *src, char *dst){
	char *buf,*outbuf;
	FILE *fp_src,*fp_dst;
	unsigned long size_src,size_new;
	compress_header header;
	int r;
	
	fp_src=fopen(src,"r");
	if (fp_src==NULL){
		printf("%s not found.\n",src);
		exit(1);
	}
	if (lzo_init() != LZO_E_OK){
		printf("Failed to initialize LZO Zip.\n");
		exit(1)	;
	}

	fread(&header, sizeof(header),1,fp_src);
	if ( header.magic_str[0]!='L' || header.magic_str[1]!='Z'){
		printf("%s is not a valid LZO Zip file!\n",src);
		exit(1);
	}
	buf=(char *)malloc(header.cur_size);
	outbuf=(char *)malloc(header.size);
	fread(buf,header.cur_size,1,fp_src);
	r=lzo1x_decompress(buf, header.cur_size,outbuf,&size_new,workmem);
	if (r != LZO_E_OK){
		printf("Failed to decompress!\n");
		exit(1)	;
	}
	fp_dst=fopen(dst,"w");
	if (fp_dst==NULL){
		printf("Failed to create %s!\n",dst);
		exit(1);
	}
	r=fwrite(outbuf, size_new, 1, fp_dst);
	if (r==0){
		printf("Decompression failed!\n");
	}
	else{
		printf("Decompressed to %s: %d bytes\n",dst,size_new);			}
	free(outbuf);
	free(buf);
	fclose(fp_src);
	fclose(fp_dst);

}

void lzo_compress(char *src, char *dst){
	char *buf,*outbuf;
	FILE *fp_src,*fp_dst;
	unsigned long size_src,size_new;
	compress_header header;
	int r;
	
	fp_src=fopen(src,"r");
	if (fp_src==NULL){
		printf("%s not found.\n",src);
		exit(1);
	}
	if (lzo_init() != LZO_E_OK){
		printf("Failed to initialize LZO Zip.\n");
		exit(1)	;
	}
	fseek(fp_src, 0, SEEK_END);
	size_src = ftell(fp_src);
	fseek(fp_src, 0, SEEK_SET);

	printf("%s: %d bytes", src, size_src);

	buf=(char *)malloc(size_src);
	outbuf=(char *)malloc(size_src+(size_src/2));
	fread(buf, size_src,1,fp_src);
	header.magic_str[0]='L';
	header.magic_str[1]='Z';
	header.size=size_src;
	
	r=lzo1x_1_compress(buf, size_src, outbuf, &size_new, workmem);
	if (r != LZO_E_OK){
		printf("Failed to initialize LZO Zip.\n");
		exit(1)	;
	}

	header.cur_size = size_new;
	fp_dst=fopen(dst,"w");
	if (fp_dst==NULL){
		printf("Failed to create %s!\n",dst);
		exit(1);
	}
	fwrite(&header, sizeof(header),1,fp_dst);
	r=fwrite(outbuf, size_new, 1, fp_dst);
	if (r==0){
		printf("Compresssion failed!\n");
	}
	else{
		printf("Compressed to %s: %d bytes\n",dst,size_new);			}
	free(outbuf);
	free(buf);
	fclose(fp_src);
	fclose(fp_dst);
}


int main(int argc, char **argv) 
{
	printf("LZO Zip 1.0 for ICSOS\n");
	if (argc == 4){
		if (argv[1][0]=='c'){
			lzo_compress(argv[2],argv[3]);
		}
		else if (argv[1][0]=='x'){
			lzo_decompress(argv[2],argv[3]);
		}else{
			printf("Usage: lzozip.exe [x/c] <source> <destination>\n");	
		}
	}else{
		printf("Usage: lzozip.exe [x/c] <source> <destination>\n");	
	}
}
