typedef struct __attribute__((packed)) _far_pointer 
{
    DWORD offset;
    WORD selector;
} far_pointer;

far_pointer pcibiosentry;
far_pointer pcibios;
