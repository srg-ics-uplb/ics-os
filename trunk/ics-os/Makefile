# ics-os Makefile
#
# written by Joseph Anthony C. Hermocilla
#

vmdex:
	make -C kernel/

clean:
	rm ics-os.img
	rm vmdex
	make -C kernel/ clean
	rm bochslog

install: vmdex
	cp grub.img ics-os.img
	mount ics-os.img mnt -tmsdos -oloop
	cp -r vmdex mnt
	cp  base/* mnt
	mkdir -p mnt/apps
	mkdir -p mnt/tcc
	mkdir -p mnt/lib
	cp apps/* mnt/apps/
	cp sdk/* mnt/tcc/
	cp lib/* mnt/lib/
	umount mnt
	chmod 666 ics-os.img

disk: install
	dd if=ics-os.img of=/dev/fd0 bs=1440

