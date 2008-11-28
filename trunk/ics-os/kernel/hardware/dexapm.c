//dex32 Advanced power mangement support
 

typedef struct __attribute((packed)) _APMinfo {
  DWORD pmodecodeseg;
  DWORD rmodecodeseg;
  DWORD entrypoint;
  DWORD apmdataseg;
  WORD cseglength32,cseglength16;
  DWORD dseglength;
} APMinfo;

APMinfo *apmloc=0x500;

extern void offcomputer(DWORD offset);

void dex32apm_init()
 {

   setgdt(APM_CS32,apmloc->pmodecodeseg << 1,
                 apmloc->cseglength32,0x9A,0x40);
   setgdt(APM_CS16,apmloc->rmodecodeseg << 1,
                 apmloc->cseglength16,0x9A,0);
   setgdt(APM_DS,apmloc->apmdataseg << 1,
                 apmloc->dseglength,0x92,0x40);

 };

void dex32apm_off()
 {
  saveregs r;
  char tmpstr[255],tmpstr1[255],tmpstr2[255],tmpstr3[255];
  r.CS=APM_CS32;
  r.DS=APM_DS;
  r.ES=APM_DS;
  r.SS=SYS_STACK_SEL;
  r.FS=APM_DS;
  r.GS=APM_DS;
  r.EAX=0x5307;
  r.EBX=1;
  r.ECX=3;
  printf("APM information: CS32 base:%s  CS16 base:%s DS base:%s offset:%s\n",
         itoa(apmloc->pmodecodeseg << 1,tmpstr,16),
         itoa(apmloc->rmodecodeseg << 1,tmpstr1,16),
         itoa(apmloc->apmdataseg << 1,tmpstr2,16),
         itoa(apmloc->entrypoint ,tmpstr3,16));
  getch();
  dex32_asyncproc(&r,apmloc->entrypoint,"dex32apm",5000);
 ;};
