# ics-os Makefile
#
# written by Joseph Anthony C. Hermocilla
#

vmdex:
	make -C kernel/

.PHONY: clean
clean:
	rm -f vmdex
	rm -f ics-os-livecd.iso
	rm -fr tmp/*
	make -C kernel/ clean

prep_image: vmdex
	rm -fr tmp
	mkdir tmp
	cp -r vmdex tmp
	cp  base/* tmp
	mkdir -p tmp/apps
	mkdir -p tmp/tcc
	mkdir -p tmp/lib
	cp apps/* tmp/apps/
	cp sdk/* tmp/tcc/
	cp lib/* tmp/lib/

floppy: prep_image
	cp grub.img ics-os-floppy.img                #copy an image with grub
	sudo rm -fr mnt
	sudo mkdir mnt
	sudo mount ics-os-floppy.img mnt -tmsdos -oloop
	sudo cp -r tmp/* mnt/
	sudo umount mnt
	sudo chmod 666 ics-os-floppy.img
	rm -fr tmp/

livecd: prep_image
	cp -r iso/* tmp/
	cp tmp/lib/* tmp/
	grub-mkrescue -o ics-os-livecd.iso tmp/
	rm -fr tmp/

run-on-qemu:
	qemu-system-i386 -fda ics-os-floppy.img -boot a -m 16M
