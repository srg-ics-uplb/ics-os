rem This is a script file for installing DEX to another disk. You still
rem have to use the GRUB setup (fd0) command though.
@echo DEX-OS installer 1.0
@echo Please wait, copying system data to temporary location...
@loadmod /boot/ramdisk.dll -name tempdisk
@mount fat tempdisk /temp
@copy /boot/kernel32.bin /temp
@copy /boot/boot/grub/stage1 /temp
@copy /boot/boot/grub/stage2 /temp
@copy /boot/boot/grub/menu.lst /temp
@echo The installer will now unmount /boot, please remove the disk
@echo from the drive and then press any key to continue.
@pause
@echo unmounting /boot..
@umount /boot    
@mount fat floppy /newdisk
@echo Please wait... installing files..
mkdir /newdisk/boot
mkdir /newdisk/boot/grub
copy /temp/stage1 /newdisk/boot/grub
copy /temp/stage2 /newdisk/boot/grub
copy /temp/menu.lst /newdisk/boot/grub
copy /temp/kernel32.bin /newdisk
umount /newdisk
@echo Preparations complete. Please remove the disk from the drive
@echo and insert the boot disk. 
@pause
umount /temp  
mount fat floppy /boot
  
