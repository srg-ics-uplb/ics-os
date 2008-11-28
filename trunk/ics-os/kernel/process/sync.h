/*
  Name: sync.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 18/01/04 06:27
  Description: Provides kernel synchornization functions
*/
#ifndef SYNC_H
#define SYNC_H

typedef struct _sync_sharedvar {
    int busy;
    int ready;
    int wait;
} sync_sharedvar;

void sync_entercrit(sync_sharedvar *var);
void sync_leavecrit(sync_sharedvar *var);

#endif
