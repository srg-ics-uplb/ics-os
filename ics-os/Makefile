# ics-os Makefile
#
# written by Joseph Anthony C. Hermocilla
#

vmdex:
	make -C kernel/

.PHONY: clean
clean:
	rm -f vmdex
	make -C kernel/ clean

install: vmdex
	cp grub.img ics-os-floppy.img
	rm -fr mnt
	mkdir mnt
	mount ics-os-floppy.img mnt -tmsdos -oloop
	cp -r vmdex mnt
	cp  base/* mnt
	mkdir -p mnt/apps
	mkdir -p mnt/tcc
	mkdir -p mnt/lib
	cp apps/* mnt/apps/
	cp sdk/* mnt/tcc/
	cp lib/* mnt/lib/
	umount mnt
	chmod 666 ics-os-floppy.img

floppy: install
	dd if=ics-os-floppy.img of=/dev/fd0 bs=1440

livecd: install


