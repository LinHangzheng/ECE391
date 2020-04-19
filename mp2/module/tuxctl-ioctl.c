/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

/* check whether the TUX are working*/
int ack;

/* 32-bit integer to represents the condition of the LEDs*/
static unsigned long LED_condition; 

/*
 * the condition of buttons
 *	7  |  6 |  5 |4 |3|2|1|  0   
 *right|left|down|up|c|b|a|start
 */
static unsigned long BUTTON_condition;

/* Set up a lock for the button*/
static spinlock_t button_lock = SPIN_LOCK_UNLOCKED;

/* My helper functions*/
int tux_initial(struct tty_struct* tty);
int tux_button(struct tty_struct* tty, unsigned long arg);
int tux_led(struct tty_struct* tty, unsigned long arg);

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
	unsigned long flag;
	char RDLU, CBAS, RLDU;
    a = packet[0]; /* Avoid printk() sign extending the 8-bit */
    b = packet[1]; /* values when printing them. */
    c = packet[2];
    /*printk("packet : %x %x %x\n", a, b, c); */
	switch (a)
	{
	case MTCP_ACK:
		/* Once recive the ACK, set ack to 1. */
		ack = 1;
		break;
	case MTCP_BIOC_EVENT:
		/* Enter the critical section
		 * R | L | D | U | C | B | A | S
		 * Revise the data because the button pressed will set the bit to 0
		 * Also change the place of the RDLU into RLDU
		 */
		RDLU = (~c)&four_mask;
		RLDU = (((RDLU&L_mask)<<1)|((RDLU&D_mask)>>1)|(RDLU&RU_mask)) << RLDU_off;
		CBAS = ((~b)&four_mask);
		spin_lock_irqsave(&button_lock,flag);
		BUTTON_condition = RLDU|CBAS;
		spin_unlock_irqrestore(&button_lock,flag);
		/*Exit the critical section*/
		break;
	case MTCP_RESET:
		tux_initial(tty);
		tux_led(tty, LED_condition);
		break;
	default:
		return;
	}
	return;
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		return tux_initial(tty);
	case TUX_BUTTONS:
		return tux_button(tty,arg);
	case TUX_SET_LED:
		return tux_led(tty,arg);
	case TUX_LED_ACK:
		return 0;
	case TUX_LED_REQUEST:
		return 0;
	case TUX_READ_LED:
		return 0;
	default:
		return -EINVAL;
    }
}

/*
 * tux_initial
 *   DESCRIPTION: initialize the tux condition 
 *   INPUTS: tty-struct tty_struct*
 *   OUTPUTS: none
 *   RETURN VALUE:0 for success and -EINVAL for fail
 *   SIDE EFFECTS: set the initial value to all the variable of TUX
 */ 
int tux_initial(struct tty_struct* tty){
	/* Initialize the LED and BUTTON condition to 0*/
	unsigned long flag;
	char command1 = MTCP_BIOC_ON;
	char command2 = MTCP_LED_USR;
	LED_condition = 0;
	ack = 0;

	/* Enter the critical section */
	spin_lock_irqsave(&button_lock,flag);
	BUTTON_condition = 0;
	spin_unlock_irqrestore(&button_lock,flag);

	/* Exit the critical section */
	/* Initialize the controller*/

	if(tuxctl_ldisc_put(tty,&command1,1) || tuxctl_ldisc_put(tty,&command2,1)){
	 	return -EINVAL;
	}

	return 0;
}

/*
 * tux_button
 *   DESCRIPTION: initialize the tux condition 
 *   INPUTS: tty -struct tty_struct*
 * 			 arg - unsigned long, a pointer to a 32-bit integer, which represents the state of button
 *   OUTPUTS: none
 *   RETURN VALUE:0 for success and -EINVAL for fail
 *   SIDE EFFECTS: set the tux_buttons for the later use
 */ 
int tux_button(struct tty_struct* tty, unsigned long arg){
	int ret;
	unsigned long flag;

	/* If the pointer is NULL, return -EINVAL immidiately*/
	if((uint32_t*)arg == NULL){return -EINVAL;}

	/* Enter the critcal section*/
	spin_lock_irqsave(&button_lock,flag);
	ret = copy_to_user((uint32_t*)arg,(uint32_t*)(&BUTTON_condition),sizeof(uint32_t));
	spin_unlock_irqrestore(&button_lock,flag);

	/* If the return value of copy_to_user >0, fail and return -EINVAL. */
	if(ret){
		return -EINVAL;
	}else{
		return 0;
	}
}

/*0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , A , B, C , D , E , F */
unsigned char SEGMENTS[16] = {0xE7, 0x06, 0xCB, 0x8F, 0x2E, 0xAD, 0xED, 0x86, 0xEF, 0xAF, 0xEE, 0x6D, 0xE1, 0x4F, 0xE9, 0xE8};

/*
 * tux_led
 *   DESCRIPTION: initialize the tux condition 
 *   INPUTS: tty -struct tty_struct*
 * 			 arg - unsigned long,  32-bit integer, which represents the state of LED
 *   OUTPUTS: none
 *   RETURN VALUE:0 for success and -EINVAL for fail
 *   SIDE EFFECTS: Set the condition of the LEDs
 */ 
int tux_led(struct tty_struct* tty, unsigned long arg){
	unsigned int LED_idx, HEX_frag, decimal_check,  decimal, Input_size;
	unsigned char cur_byte;
	unsigned char LED_buff[LED_SET_SIZE];
	/* If the ack is 0, return */
	if (!ack){
	 	return 0;
	}
	ack = 0;

	/* Set up the LED_buff*/
	LED_buff[0] = MTCP_LED_USR;
	tuxctl_ldisc_put(tty,LED_buff,1);
	LED_buff[0] = MTCP_LED_SET;
	LED_buff[1] = (arg >> (2*Byte)) & four_mask;

	decimal = (arg>>decimal_off_32) & four_mask;


	/*The size of buff input to the tux*/
	Input_size = 2;
	
	/* Set up the led buffer for each LED*/
	for (LED_idx=0;LED_idx<LED_SIZE;LED_idx++){
		if((LED_buff[1]>>LED_idx)&0x01){
			HEX_frag = SEGMENTS[(arg>>(LED_idx*LED_SIZE))&four_mask];
			decimal_check = ((decimal & (1<<LED_idx)) > 0);
			cur_byte = HEX_frag | (decimal_check<<decimal_off_8);
			LED_buff[Input_size] = cur_byte;
			/* The LED_buff's size is dependent on how many led is working*/
			Input_size += 1;
		}
	}

	if(tuxctl_ldisc_put(tty,LED_buff,Input_size)){
		/* If fail*/
		return -EINVAL;
	}
	LED_condition = arg;
	
	return 0;

}






