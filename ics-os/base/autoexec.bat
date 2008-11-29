@echo off
@echo Loading Microsoft C runtime library...
loadmod /boot/lib/msvcrt.dll
rem @echo Initializing RAM Disk...
rem loadmod /boot/lib/ramdisk.dll -blocks 9000
rem mount fat ramdisk /ramdisk
rem copy /boot/apps/ed.exe /ramdisk
rem pcut rd: /ramdisk/
cls
cd start
@echo 
@echo Welcome to the ICS Operating System
echo
@echo Institute of Computer Science
@echo University of the Philippines, Los Banos
@echo
@echo Type "help" on the command prompt to
@echo display available commands.
@echo 


