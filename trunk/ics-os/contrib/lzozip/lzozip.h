#ifndef _LZOZIP_H_
#define _LZOZIP_H_

#include "../../sdk/dexsdk.h"
#include "../../kernel/dextypes.h"

#include "lzodefs.h"
#include "minilzo.h"

typedef struct __attribute__((packed)) _compress_header {
	char magic_str[2];
	DWORD size;
	DWORD cur_size;
} compress_header;

#endif
