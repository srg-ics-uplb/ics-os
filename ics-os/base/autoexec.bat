@echo off
@echo Loading Microsoft C runtime library...
loadmod /icsos/lib1/msvcrt.dll
@echo Initializing RAM Disk...
loadmod /icsos/lib1/ramdisk.dll -blocks 10000
mount fat ramdisk /ramdisk
rem copy /icsos/apps/ed.exe /ramdisk
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

