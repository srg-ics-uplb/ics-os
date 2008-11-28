DWORD time_count = 0,  //used to store the number of seconds since dex was booted
aux_time2=0;   //since the OS has the timer set to interrupt 200 times a second
               //an auxillary counter is required so that it increments time_count
               //if it reaches 200
               
int time_monthdays[12]= {0,31,59,90,120,151,181,212,243,273,304,334,365}; 

//the tme returned by the timer chip is in BCD, so we have to
//perform some conversions to binary
DWORD bcdtobinary(DWORD b)
  {

   DWORD x= b & 0xff,c,r;
   r = x & 0xF;
   c = x >> 4;
   return (c*10+r);

  ;};

char *getmonthname(int month,char *str)
  {
     switch (month)
       {
         case 1 : strcpy(str,"January"); break;
         case 2 : strcpy(str,"Febuary"); break;
         case 3 : strcpy(str,"March");break;
         case 4 : strcpy(str,"April");break;
         case 5 : strcpy(str,"May");break;
         case 6 : strcpy(str,"June");break;
         case 7 : strcpy(str,"July");break;
         case 8 : strcpy(str,"August");break;
         case 9 : strcpy(str,"September");break;
         case 10: strcpy(str,"October");break;
         case 11: strcpy(str,"November");break;
         case 12: strcpy(str,"Decemeber");break;
       };
    return str;
  };

char *datetostr(dex32_datetime *d,char *str)
  {
     char temp1[20],temp2[20],temp3[20];
     sprintf(str,"%s/%s/%s",itoa(d->month,temp1,10),
            itoa(d->day,temp2,10),itoa(d->year,temp3,10));
     return str;
  };

void getdatetime(dex32_datetime *d) //gets the date nd time
  {
     DWORD x;

     //seconds
     outportb(0x70,0);
     //delay(1);
     x=inportb(0x71);
     d->sec=bcdtobinary(x); //convert to binary

     //minutes
     outportb(0x70,2);
     //delay(1);
     x=inportb(0x71);
     d->min=bcdtobinary(x);

     //hours
     outportb(0x70,4);
     //delay(1);
     x=inportb(0x71);
     d->hour=bcdtobinary(x);

     outportb(0x70,0x7);
     //delay(1);
     x=inportb(0x71);
     d->day=bcdtobinary(x);

     outportb(0x70,0x8);
     //delay(1);
     x=inportb(0x71);
     d->month=bcdtobinary(x);

     outportb(0x70,9);
     //delay(1);
     x=inportb(0x71);
     d->year=bcdtobinary(x);
     if (d->year<80) d->year+=2000; //adjust for the year 2000
  };
  
//returns time in milliseconds
DWORD time_gettime()
{
return time_count;
};

//returns time in milliseconds
DWORD getprecisetime()
{
 return (time_count*100+(aux_time2/2));
};

int time()
   {
     int totaldays = (time_systime.year - 1970)*365;
     int totalseconds,totalminutes,totalhours;
     if (time_systime.year%4 !=0 || time_systime.month>2)
     totaldays+=time_systime.year/4;
        else
     { 
     totaldays+=(time_systime.year/4) - 1;   
     };
     
     if (time_systime.month>2&&time_systime.year%4==0) totaldays+=1;
     totaldays+=time_monthdays[time_systime.month-1];
     totaldays+=time_systime.day;
     totalhours = totaldays*24 + time_systime.hour;
     totalminutes = totalhours*60 + time_systime.min;
     totalseconds = totalminutes * 60;
     totalseconds += time_systime.sec;
     return totalseconds;
   };    

int time_getmycputime()
{
    return current_process->totalcputime;
};
//increments the system time by one millisecond
void time_incrementtime()
{
    time_systime.adj++;
    if (time_systime.adj>context_switch_rate/100)
      {
       time_systime.ms ++;
       time_systime.adj = 0;
      }; 
    if (time_systime.ms>=100)
      {
         time_systime.ms = 0;
         time_systime.sec++;
         if (time_systime.sec>=60)
           {
               time_systime.sec=0;
               time_systime.min++;
               if (time_systime.min>=60)
               {
               time_systime.min=0;
               time_systime.hour++;
               if (time_systime.hour>=24)
                   time_systime.hour=0;
               };
           };
      };
};
   
//the timer handler used by the task switcher
void time_handler()
 {

   //update the real-time clock
   //DEX32 is programmed to switch process every
   // 1/200 of a second so we use a counter that counts
   //up to 200 and then increments time_count which
   //holds the time elasped in seconds since the
   //system has started
   
   aux_time2++;
   
   
   if (aux_time2>=context_switch_rate)
         {
             aux_time2=0;
             time_count++;

             //synchronize with the clock every 10 minutes
             if (time_systime.min%10==0)
                 getdatetime(&time_systime);
         };

   ticks++;
   time_incrementtime();
   
   fdctimer(); //Enables the floppy disk driver to shut down
               //the drive motor after a certain period of time
               //see floppy.c for details
   outportb(0x20,0x20); //renable the timer                
;};

//delays the execution of a program for a specified number of milliseconds
void delay(DWORD w)
 {
   DWORD t1, cpuflags;
   storeflags(&cpuflags);
   stopints();
   
   t1 = ticks+w*2;
   restoreflags(cpuflags);
      
   while (ticks<t1);
   

 };

//sets the rate of context switch
//It is best to set the value between 100-300 to prevent
//erratic behavior

void dex32_set_timer(DWORD rate)
{
    WORD time_val;
    BYTE time_val_high,time_val_low;
    DWORD flags;
    storeflags(&flags);
    stopints(); //stop interrupts

    time_val= 1193180 / rate;
    time_val_low = time_val & 0xFF;
    time_val_high = time_val >> 8;

    outportb(0x43,0x36); //tell which timer to reprogram
    outportb(0x40,time_val_low);
    outportb(0x40,time_val_high);
    restoreflags(flags);
};


void time_init()
{
    //update system time
    getdatetime(&time_systime);
};
