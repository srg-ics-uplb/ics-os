#include "pci.h"
//#include <string.h>
//#include <printf.h>


/*
 * Read 2 bytes
 * */
WORD inports(WORD _port) {
    WORD rv;
    asm volatile ("inw %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}


DWORD pci_size_map[100];
pci_dev_t dev_zero= {0};
/*
 * Given a pci device(32-bit vars containing info about bus, device number, and function number), a field(what u want to read from the config space)
 * Read it for me !
 * */
DWORD icsos_pci_read(pci_dev_t dev, DWORD field) {
printf("in pci_read\n");	
   // Only most significant 6 bits of the field
	dev.field_num = (field & 0xFC) >> 2;
	dev.enable = 1;
	outportl(PCI_CONFIG_ADDRESS, dev.bits);

	// What size is this field supposed to be ?
	DWORD size = pci_size_map[field];
	if(size == 1) {
		// Get the first byte only, since it's in little endian, it's actually the 3rd byte
		uint8_t t =inportb(PCI_CONFIG_DATA + (field & 3));
		return t;
	}
	else if(size == 2) {
		WORD t = inports(PCI_CONFIG_DATA + (field & 2));
		return t;
	}
	else if(size == 4){
		// Read entire 4 bytes
		DWORD t = inportl(PCI_CONFIG_DATA);
		return t;
	}
	return 0xffff;
}

/*
 * Write pci field
 * */
void icsos_pci_write(pci_dev_t dev, DWORD field, DWORD value) {
	dev.field_num = (field & 0xFC) >> 2;
	dev.enable = 1;
	// Tell where we want to write
	outportl(PCI_CONFIG_ADDRESS, dev.bits);
	// Value to write
	outportl(PCI_CONFIG_DATA, value);
}

/*
 * Get device type (i.e, is it a bridge, ide controller ? mouse controller? etc)
 * */
DWORD icsos_get_device_type(pci_dev_t dev) {
	DWORD t = icsos_pci_read(dev, PCI_CLASS) << 8;
	return t | icsos_pci_read(dev, PCI_SUBCLASS);
}

/*
 * Get secondary bus from a PCI bridge device
 * */
DWORD icsos_get_secondary_bus(pci_dev_t dev) {
	return icsos_pci_read(dev, PCI_SECONDARY_BUS);
}

/*
 * Is current device an end point ? PCI_HEADER_TYPE 0 is end point
 * */
DWORD icsos_pci_reach_end(pci_dev_t dev) {
	DWORD t = icsos_pci_read(dev, PCI_HEADER_TYPE);
	return !t;
}

/*
 * The following three functions are basically doing recursion, enumerating each and every device connected to pci
 * We start with the primary bus 0, which has 8 function, each of the function is actually a bus
 * Then, each bus can have 8 devices connected to it, each device can have 8 functions
 * When we gets to enumerate the function, check if the vendor id and device id match, if it does, we've found our device !
 **/

/*
 * Scan function
 * */
pci_dev_t icsos_pci_scan_function(WORD vendor_id, WORD device_id, DWORD bus, DWORD device, DWORD function, int device_type) {
	pci_dev_t dev = {0};
	dev.bus_num = bus;
	dev.device_num = device;
	dev.function_num = function;
	// If it's a PCI Bridge device, get the bus it's connected to and keep searching
	if(icsos_get_device_type(dev) == PCI_TYPE_BRIDGE) {
		icsos_pci_scan_bus(vendor_id, device_id, icsos_get_secondary_bus(dev), device_type);
	}
	// If type matches, we've found the device, just return it
	if(device_type == -1 || device_type == icsos_get_device_type(dev)) {
		DWORD devid  = icsos_pci_read(dev, PCI_DEVICE_ID);
		DWORD vendid = icsos_pci_read(dev, PCI_VENDOR_ID);
		if(devid == device_id && vendor_id == vendid)
			return dev;
	}
	return dev_zero;
}

/*
 * Scan device
 * */
pci_dev_t icsos_pci_scan_device(WORD vendor_id, WORD device_id, DWORD bus, DWORD device, int device_type) {
	printf("in pci_scan_device\n");
   pci_dev_t dev = {0};
	dev.bus_num = bus;
	dev.device_num = device;

	if(icsos_pci_read(dev,PCI_VENDOR_ID) == PCI_NONE)
		return dev_zero;

	pci_dev_t t = icsos_pci_scan_function(vendor_id, device_id, bus, device, 0, device_type);
	if(t.bits)
		return t;

	if(icsos_pci_reach_end(dev))
		return dev_zero;

	for(int function = 1; function < FUNCTION_PER_DEVICE; function++) {
		if(icsos_pci_read(dev,PCI_VENDOR_ID) != PCI_NONE) {
			t = icsos_pci_scan_function(vendor_id, device_id, bus, device, function, device_type);
			if(t.bits)
				return t;
		}
	}
	return dev_zero;
}
/*
 * Scan bus
 * */
pci_dev_t icsos_pci_scan_bus(WORD vendor_id, WORD device_id, DWORD bus, int device_type) {
	printf("in pci_scan_bus\n");
   for(int device = 0; device < DEVICE_PER_BUS; device++) {
		pci_dev_t t = icsos_pci_scan_device(vendor_id, device_id, bus, device, device_type);
		if(t.bits)
			return t;
	}
	return dev_zero;
}

/*
 * Device driver use this function to get its device object(given unique vendor id and device id)
 * */
pci_dev_t icsos_pci_get_device(WORD vendor_id, WORD device_id, int device_type) {

	pci_dev_t t = icsos_pci_scan_bus(vendor_id, device_id, 0, device_type);
	if(t.bits)
		return t;

	// Handle multiple pci host controllers

	if(icsos_pci_reach_end(dev_zero)) {
		printf("PCI Get device failed...\n");
	}else{
		printf("PCI Get device success...\n");
   }
   
	for(int function = 1; function < FUNCTION_PER_DEVICE; function++) {
		pci_dev_t dev = {0};
		dev.function_num = function;

		if(icsos_pci_read(dev, PCI_VENDOR_ID) == PCI_NONE)
			break;
		t = icsos_pci_scan_bus(vendor_id, device_id, function, device_type);
		if(t.bits)
			return t;
	}
	return dev_zero;
}

/*
 * PCI Init, filling size for each field in config space
 * */
void icsos_pci_init() {
	// Init size map
	pci_size_map[PCI_VENDOR_ID] =	2;
	pci_size_map[PCI_DEVICE_ID] =	2;
	pci_size_map[PCI_COMMAND]	=	2;
	pci_size_map[PCI_STATUS]	=	2;
	pci_size_map[PCI_SUBCLASS]	=	1;
	pci_size_map[PCI_CLASS]		=	1;
	pci_size_map[PCI_CACHE_LINE_SIZE]	= 1;
	pci_size_map[PCI_LATENCY_TIMER]		= 1;
	pci_size_map[PCI_HEADER_TYPE] = 1;
	pci_size_map[PCI_BIST] = 1;
	pci_size_map[PCI_BAR0] = 4;
	pci_size_map[PCI_BAR1] = 4;
	pci_size_map[PCI_BAR2] = 4;
	pci_size_map[PCI_BAR3] = 4;
	pci_size_map[PCI_BAR4] = 4;
	pci_size_map[PCI_BAR5] = 4;
	pci_size_map[PCI_INTERRUPT_LINE]	= 1;
	pci_size_map[PCI_SECONDARY_BUS]		= 1;
}
