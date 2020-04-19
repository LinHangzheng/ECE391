#ifdef _USERSPACE

/* Alias the ioctl and open system calls to our own */
#define ioctl __mp1_ioctl
#define open __mp1_open

/* Stub out the kernel calls since we're in userspace */
#define mp1_copy_to_user __mp1_copy_to_user
#define mp1_copy_from_user __mp1_copy_from_user
#define __user 

#endif

#ifndef _ASM
#include <linux/rtc.h>
#include <linux/ioctl.h>
#include "mp1_userspace.h"

#define RTC_STARTGAME 	_IO('M', 0x13) 
#define RTC_ENDGAME 	_IO('M', 0x14)
#define RTC_KEYSTROKE 	_IO('M', 0x15)  
#define RTC_GETSTATUS 	_IO('M', 0x16)
#define RTC_VACCINATE 	_IO('M', 0x17)  


struct keystroke_args {
    unsigned short direction;	/* keystroke direction: 0 is left,   */
    				/*    1 is down, 2 is right, 3 is up */
    unsigned short selector;	/* selector position (0 to 4)        */
    unsigned char guess[5];	/* current DNA fragment guess        */
    unsigned char hint[5];	/* current DNA fragment hints        */
};

struct game_status {
    unsigned int pop;		/* human population remaining */
    unsigned int inf;		/* current number infected    */
};


extern void mp1_free(void*);
extern void* mp1_malloc(int);
extern void mp1_notify_user(void);

extern volatile unsigned long long rand_seed;

#endif
