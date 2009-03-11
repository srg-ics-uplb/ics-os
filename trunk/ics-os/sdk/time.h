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

void get_date_time(dex32_datetime*); //gets the date nd time
int time();
void delay(unsigned int ms);
#endif
