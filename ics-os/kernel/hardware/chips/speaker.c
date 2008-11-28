void beep()
{
   char spk;
   spk=inportb(0x61);
   spk=spk|3;
   outportb(0x61,spk);
   delay(1);
   spk=inportb(0x61);
   spk=spk&252;
   outportb(0x61,spk);

};

