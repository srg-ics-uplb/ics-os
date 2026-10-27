#ifndef __VERSION_H_
#define __VERSION_H_

#define KERNEL_VERSION "0.01"
#define KERNEL_NAME    "vmdex"

#define OS_NAME        "ICS Operating System"
#define OS_VERSION     "(master)"


char *dex32_versionstring=OS_NAME;

#ifndef ICSOS_VERSION
#define ICSOS_VERSION "unknown"
#endif
#ifndef ICSOS_GIT_HASH
#define ICSOS_GIT_HASH "unknown"
#endif

const char kernel_version[] = "ICS-OS " ICSOS_VERSION " (" ICSOS_GIT_HASH ")";



#endif
