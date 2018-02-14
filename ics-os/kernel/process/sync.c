/*
  Name: sync.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 18/01/04 06:27
  Description: Provides process synchornization functions
*/


//perform busy waiting
void sync_justwait(sync_sharedvar *var){
   while (var->busy && var->busy!=getprocessid())
      ;
};

//attempt to enter the critical section
void sync_entercrit(sync_sharedvar *var){
   while (var->busy && var->busy!=getprocessid())
      ;

   var->busy = getprocessid();

   var->wait++;
};

//leave the critical section
void sync_leavecrit(sync_sharedvar *var){
   var->wait--;

   if (var->wait<0) 
      printf("sync: warning wrong number of enter-leave pairs detected!\n");

   if (var->wait==0) 
      var->busy = 0;
};

