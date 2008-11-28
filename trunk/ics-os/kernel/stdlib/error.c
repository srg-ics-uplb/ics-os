/*
  Name: dex_error.c 
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 10/02/04 20:43
  Description: This module gives additional info about an error that has just
               occured in this process.
*/


int error_getlasterror()
{
    return current_process->lasterror;
};

void error_seterror(int error)
{
    current_process->lasterror = error;
};
