/* keyboard.h - Defines used in interactions with the keyboard interrupt
 */
#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "../types.h"
#include "i8259.h"

/*refers to https://wiki.osdev.org/Interrupts*
 *Irq Num for keyboard should be 1 and correspond to
 *0x21 in IDT
 */
#define KEYBOARD_IRQ_NUM   1

/*ports for the keyboard*/
#define KEYBOARD_PORT_DATA       0x60
#define KEYBOARD_PORT_CMD        0x64


#define LEFT_SHIFT_PRESS         0x2A
#define LEFT_SHIFT_RELEASE       0xAA

#define RIGHT_SHIFT_PRESS        0x36
#define RIGHT_SHIFT_RELEASE      0xB6

#define ALT_PRESS                0x38
#define ALT_RELEASE              0xB8

#define CAPSLOCK_PRESS           0x3A
#define CAPSLOCK_RELEASE         0xBA

#define LEFT_CTRL_PRESS          0x1D
#define LEFT_CTRL_RELEASE        0x9D

#define ESC                      0x01

#define TAB_PRESS               0x0f

#define BACKSPACE                0x0E

#define F1                      0x3B
#define F2                      0x3C
#define F3                      0x3D

#define ASCILL_BACKSPACE         8

#define LOW_8_BITS              0xFF

#define MAX_BUF_SIZE                128

/*init the keyboard and begin to accept interrupt*/
void keyboard_init(void);
/*The interrupt handler for keystroke*/
void keyboard_irq_handler(void);
/* judge whether the key is a special key */
int32_t handle_special_key(uint8_t scancode);
/* read the value stored in the key board buf */
int32_t keyboard_read (int32_t fd,  uint32_t offset, void* buf, int32_t nbytes);

/* clear the keyboard buf */
void keyboard_clean(void);
/* print character on current terminal */
void printkey_on_curr_terminal(uint8_t keystroke);
/* print string on current terminal */
void printf_on_curr_terminal(int8_t* string);
/* auto complete the current command/argument */
void keyboard_auto_complete(void);

extern char* keyboard_buf;
extern uint8_t buf_pos;

#endif /* _I8259_H */


