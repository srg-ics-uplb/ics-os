/*
lotto.c - workload for the ICS-OS lottery scheduling lab.

usage:  lotto <ticketsA> <ticketsB> [seconds]

Starts two CPU-bound threads, A and B, gives them the number of lottery
tickets you ask for (by running your kernel's "tickets" command on them),
lets them compete for the CPU for [seconds] (default 10), then reports how
much work each one got done.

If the scheduler is fair in proportion to tickets, A's share of the work
should be close to ticketsA / (ticketsA + ticketsB).

Examples:   lotto 10 10      (equal tickets: expect about 50% / 50%)
            lotto 30 10      (expect about 75% / 25%)
            lotto 10 40      (expect about 20% / 80%)

Note: until you have implemented the "tickets" command, the kernel answers
"Command or executable not found" for the two ticket assignments. That is
expected, and the work is simply split evenly by the round-robin scheduler.
*/

#include "../../sdk/dexsdk.h"

/* Each worker counts in units of 1000 loop iterations, so the counters
   stay far below the 32-bit limit even on a fast machine. */
volatile unsigned int workA = 0, workB = 0;
volatile unsigned int sinkA = 0, sinkB = 0;

void workerA(){
   int j;
   while (1){
      for (j = 0; j < 1000; j++) sinkA++;
      workA++;
   }
}

void workerB(){
   int j;
   while (1){
      for (j = 0; j < 1000; j++) sinkB++;
      workB++;
   }
}

/* run a kernel console command, exactly as if you had typed it */
void console_cmd(char *cmd){
   char buf[80];
   strcpy(buf, cmd);
   dexsdk_systemcall(0x95, (int)buf, 0, 0, 0, 0);
}

/* seconds since 1970, from the kernel clock (the SDK time() wrapper
   forgets to return its value, so we call the system call ourselves) */
int now(){
   return (int)dexsdk_systemcall(0x55, 0, 0, 0, 0, 0);
}

int main(int argc, char *argv[]){
   int ticketsA = 10, ticketsB = 10, seconds = 10;
   int tidA, tidB, start;
   unsigned int a, b, total;
   char cmd[64];

   /* argv[0] is the program name; the numbers follow */
   if (argc > 1) ticketsA = atoi(argv[1]);
   if (argc > 2) ticketsB = atoi(argv[2]);
   if (argc > 3) seconds  = atoi(argv[3]);
   if (ticketsA <= 0 || ticketsB <= 0 || seconds <= 0){
      printf("usage: lotto <ticketsA> <ticketsB> [seconds]\n");
      return 1;
   }

   tidA = thread_create((void*)workerA);
   tidB = thread_create((void*)workerB);
   printf("lotto: thread A = pid %d, thread B = pid %d\n", tidA, tidB);

   sprintf(cmd, "tickets %d %d", tidA, ticketsA);
   console_cmd(cmd);
   sprintf(cmd, "tickets %d %d", tidB, ticketsB);
   console_cmd(cmd);

   printf("lotto: running for %d seconds...\n", seconds);
   start = now();
   while (now() - start < seconds)
      ;

   /* freeze the numbers before printing (printing takes time) */
   a = workA;
   b = workB;
   total = a + b;
   if (total == 0){
      printf("lotto: the threads did no work.\n");
      return 1;
   }

   printf("tickets   A = %d   B = %d\n", ticketsA, ticketsB);
   printf("work      A = %u   B = %u   (units of 1000 loops)\n", a, b);
   printf("share     A = %d%%   B = %d%%\n",
            (int)(a * 100 / total), (int)(b * 100 / total));
   printf("expected  A = %d%%   B = %d%%\n",
            ticketsA * 100 / (ticketsA + ticketsB),
            ticketsB * 100 / (ticketsA + ticketsB));
   return 0;
}
