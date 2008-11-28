/*
  Name: DEX32 Event / Signal manager
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 05/12/03 22:20
  Description: 
*/

typedef struct _event_data 
{
    int size;
    int version;
    int event_type;
    DWORD data,data1;
    void(*event_handler)(int event_param);
} event_data;


