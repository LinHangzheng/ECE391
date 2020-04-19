// All necessary declarations for the Tux Controller driver must be in this file

#ifndef TUXCTL_H
#define TUXCTL_H

#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT _IO('E', 0x13)
#define TUX_LED_REQUEST _IO('E', 0x14)
#define TUX_LED_ACK _IO('E', 0x15)

/* offset of LED data*/
#define LED_SET_SIZE 6
#define LED_SIZE    4
#define RLDU_off    4
#define LED_offset  2
#define Byte        8
#define L_mask      0x02
#define D_mask      0x04
#define RU_mask     0x09
#define four_mask   0x0F
#define eight_mask  0xFF
#define decimal_off_32 24 
#define decimal_off_8 4




#endif

