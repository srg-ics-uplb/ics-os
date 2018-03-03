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


//A node in the environment variables data structure
//which is a doubly-linked list
typedef struct _env_strings {
   char *name;                   //name of the variable 
   char *value;                  //value of the variable
   struct _env_strings *next;    //pointer to the next node
   struct _env_strings *prev;    //pointer to the previous node
} env_strings;          

int env_busywait = 0;            //used for synchronization
env_strings *env_head = 0;       //head of the list

/* function prototypes*/
void env_showenv();
char *env_getenv(const char *name, char *buf);
int env_setenv(const char *name, const char *value, int replace);
#endif
