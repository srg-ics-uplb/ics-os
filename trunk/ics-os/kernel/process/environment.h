/*
  Name: environment.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 04/01/04 05:04
  Description: Handles environment strings with concurrency support
    As of the moment, implementation is on a global basis only, a per process
    implementation will be implemented in the future.
*/

#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

typedef struct _env_strings {
char *name;
char *value;
struct _env_strings *next;
struct _env_strings *prev; 
} env_strings;

int env_busywait = 0;
env_strings *env_head = 0;

/* function prototypes*/
void env_showenv();
#endif
