#ifndef PCI_H
#define PCI_H
//#include <system.h>

// I love bit fields So much better than ugly big twidling :)
typedef union _pci_dev {
    DWORD bits;
    struct {
        DWORD always_zero    : 2;
        DWORD field_num      : 6;
        DWORD function_num   : 3;
        DWORD device_num     : 5;
        DWORD bus_num        : 8;
        DWORD reserved       : 7;
        DWORD enable         : 1;
    };
} pci_dev_t;

// Ports
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// Config Address Register

// Offset
#define PCI_VENDOR_ID            0x00
#define PCI_DEVICE_ID            0x02
#define PCI_COMMAND              0x04
#define PCI_STATUS               0x06
#define PCI_REVISION_ID          0x08
#define PCI_PROG_IF              0x09
#define PCI_SUBCLASS             0x0a
#define PCI_CLASS                0x0b
#define PCI_CACHE_LINE_SIZE      0x0c
#define PCI_LATENCY_TIMER        0x0d
#define PCI_HEADER_TYPE          0x0e
#define PCI_BIST                 0x0f
#define PCI_BAR0                 0x10
#define PCI_BAR1                 0x14
#define PCI_BAR2                 0x18
#define PCI_BAR3                 0x1C
#define PCI_BAR4                 0x20
#define PCI_BAR5                 0x24
#define PCI_INTERRUPT_LINE       0x3C
#define PCI_SECONDARY_BUS        0x09

// Device type
#define PCI_HEADER_TYPE_DEVICE  0
#define PCI_HEADER_TYPE_BRIDGE  1
#define PCI_HEADER_TYPE_CARDBUS 2
#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA   0x0106
#define PCI_NONE 0xFFFF


#define DEVICE_PER_BUS           32
#define FUNCTION_PER_DEVICE      32

DWORD icsos_pci_read(pci_dev_t dev, DWORD field);
void pci_write(pci_dev_t dev, DWORD field, DWORD value);
DWORD icsos_get_device_type(pci_dev_t dev);
DWORD icsos_get_secondary_bus(pci_dev_t dev);
DWORD icsos_pci_reach_end(pci_dev_t dev);
pci_dev_t icsos_pci_scan_function(WORD vendor_id, WORD device_id, DWORD bus, DWORD device, DWORD function, int device_type);
pci_dev_t icsos_pci_scan_device(WORD vendor_id, WORD device_id, DWORD bus, DWORD device, int device_type);
pci_dev_t icsos_pci_scan_bus(WORD vendor_id, WORD device_id, DWORD bus, int device_type);
pci_dev_t icsos_pci_get_device(WORD vendor_id, WORD device_id, int device_type);
void icsos_pci_init();

#endif
