/*
  Name: DEX32 low-level keyboard device handler
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 28/11/03 15:22
  Description: This module provides low-level keyboard functions like getch()
*/

//The size of the keyboard bufer
#define BUF_SIZE 256


#define CAPS_LOCK 4
#define NUM_LOCK 2
#define SCROLL_LOCK 1


#define	RAW1_LEFT_CTRL		0x1D
#define	RAW1_LEFT_SHIFT		0x2A
#define	RAW1_CAPS_LOCK		0x3A
#define	RAW1_LEFT_ALT		0x38
#define	RAW1_RIGHT_ALT		0x38	/* same as left */
#define	RAW1_RIGHT_CTRL		0x1D	/* same as left */
#define	RAW1_RIGHT_SHIFT	0x36
#define	RAW1_SCROLL_LOCK	0x46
#define	RAW1_NUM_LOCK		0x45
#define	RAW1_DEL		0x53


/*----------------------------------------------------------------------------
SCANCODE CONVERSION
----------------------------------------------------------------------------*/
#define CTRL_ALT    400
#define ALT_TAB     59

/* "ASCII" values for non-ASCII keys. All of these are user-defined.
function keys: */
#define	KEY_F1		0x80
#define	KEY_F2		(KEY_F1 + 1)
#define	KEY_F3		(KEY_F2 + 1)
#define	KEY_F4		(KEY_F3 + 1)
#define	KEY_F5		(KEY_F4 + 1)
#define	KEY_F6		(KEY_F5 + 1)
#define	KEY_F7		(KEY_F6 + 1)
#define	KEY_F8		(KEY_F7 + 1)
#define	KEY_F9		(KEY_F8 + 1)
#define	KEY_F10		(KEY_F9 + 1)
#define	KEY_F11		(KEY_F10 + 1)
#define	KEY_F12		(KEY_F11 + 1)
/* cursor keys */
#define	KEY_INS		0x90
#define	KEY_DEL		(KEY_INS + 1)
#define	KEY_HOME	(KEY_DEL + 1)
#define	KEY_END		(KEY_HOME + 1)
#define	KEY_PGUP	(KEY_END + 1)
#define	KEY_PGDN	(KEY_PGUP + 1)
#define	KEY_LFT		(KEY_PGDN + 1)
#define	KEY_UP		(KEY_LFT + 1)
#define	KEY_DN		(KEY_UP + 1)
#define	KEY_RT		(KEY_DN + 1)
/* print screen/sys rq and pause/break */
#define	KEY_PRNT	(KEY_RT + 1)
#define	KEY_PAUSE	(KEY_PRNT + 1)
/* these return a value but they could also act as additional meta keys */
#define	KEY_LWIN	(KEY_PAUSE + 1)
#define	KEY_RWIN	(KEY_LWIN + 1)
#define	KEY_MENU	(KEY_RWIN + 1)

#define SOFT_RESET  0xAFFF

/* "meta bits"
0x0100 is reserved for non-ASCII keys, so start with 0x200 */
#define	KBD_META_ALT	0x0200	/* Alt is pressed */
#define	KBD_META_CTRL	0x0400	/* Ctrl is pressed */
#define	KBD_META_SHIFT	0x0800	/* Shift is pressed */
#define	KBD_META_ANY	(KBD_META_ALT | KBD_META_CTRL | KBD_META_SHIFT)
#define	KBD_META_CAPS	0x1000	/* CapsLock is on */
#define	KBD_META_NUM	0x2000	/* NumLock is on */
#define	KBD_META_SCRL	0x4000	/* ScrollLock is on */
/*****************************************************************************
*****************************************************************************/


int start_ps=0;
unsigned char statusbits=0,ostatus=0;
unsigned kbd_status;

int deq_busy=0; int busy=0;

typedef struct
{
	unsigned int *data;
	unsigned size, in_ptr, out_ptr;
} queue_t;

static unsigned int _kbd_buf[BUF_SIZE];


static queue_t _q =
{
	_kbd_buf, BUF_SIZE, 0, 0
};

//This structure is used for defining hotkeys
typedef struct _kb_hotkey_info {
    int id;
    unsigned int key, //The ascii equivalent of the keystroke
             status;   /*possible status bits (SHIFT pressed, CAPS LOCk etc.) 
                         of the keyboard, set to 0xFF for any status*/
    void (*handler)(); /*The function that will be called if this hotkey is pressed*/
    
    //This is a doubly linked list so .....
    struct _kb_hotkey_info *prev;
    struct _kb_hotkey_info *next;
    
} kb_hotkey_info;

kb_hotkey_info *hotkey_list = 0;
int kb_totalhotkeys = 0;

sync_sharedvar kb_busywait;

void write_kbd(unsigned adr, unsigned data);
static int set1_scancode_to_ascii(unsigned code);
void settogglebits(unsigned char b);
static int inq(queue_t *q, unsigned int data);
static int deq(queue_t *q, unsigned int *data);
static int empty(queue_t *q);
void sendtokeyb(const char *s,queue_t *q);
int signal_foreground();
void setkeyfocus(DWORD id);
void kb_removehotkey(int id);
int kb_dohotkey(WORD key, WORD status);
int kb_addhotkey(WORD key,WORD status,void (*handler)());
int kb_keypressed();
int kb_dequeue(int *val);
int kill_foreground();
void  kbd_irq(void);
static int read_kbd(void);
void keyboardflush();
void write_kbd(unsigned adr, unsigned data);
static int write_kbd_await_ack(unsigned val);
static int init_kbd(unsigned ss, unsigned typematic, unsigned xlat);
static int set1_scancode_to_ascii(unsigned code);
char pause();
int kb_ready();
char getch();
unsigned int getchw();
void installkeyboard();


