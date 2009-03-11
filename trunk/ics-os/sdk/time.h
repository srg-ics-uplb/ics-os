//DEX32-time.h
//Description: Time.h defines the functions related to obtaining the
//              system time and date
//             programmed by Josph Emmanuel Dayo


//the structue used to define date and time
#ifndef DEX_TIME_H

#define DEX_TIME_H

typedef struct _dex32_datetime {
    int month,year,day,hour,min,sec,ms,adj;
} dex32_datetime;

extern DWORD time_count,  //used to store the number of seconds since dex was booted
aux_time2;   //since the OS has the timer set to interrupt 200 times a second
               //an auxillary counter is required so that it increments time_count
               //if it reaches 200

typedef int clock_t; //for UNIX compatibility
dex32_datetime time_systime;

char *datetostr(dex32_datetime *d,char *str);
void getdatetime(dex32_datetime*); //gets the date nd time
void dex32_set_timer(DWORD rate);
int time();

#endif
