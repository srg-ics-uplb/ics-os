rem The Microsoft C runtime library is needed by tcc
@echo Loading Microsoft C runtime library...
loadmod /icsos/lib/msvcrt.dll
@echo Initializing RAM Disk...
loadmod /icsos/lib/ramdisk.dll -blocks 9000
mount fat ramdisk /ramdisk
copy /boot/apps/ed.exe /ramdisk
pcut rd: /ramdisk/
cls
cd icsos
@echo 
@echo Welcome to the ICS Operating System
echo
@echo Institute of Computer Science
@echo University of the Philippines, Los Banos
@echo
@echo Type "help" on the command prompt to
@echo display available commands.
@echo 


