#define ASM     1

#include "Interrupt_wrapper.h"


.globl keyboard_irq_wrap,mouse_irq_wrap, rtc_irq_wrap, pit_irq_wrap

/* keyboard_irq_wrap
 *      DESCIRPTION:    wrapper for keyboard_irq_handler, store all the registers and restore them
 *                      it will be put in the IDT
 *      INPUT:  none
 *      OUTPUT: execute the keyboard interupt handler
 *      RETURN: none
 */
keyboard_irq_wrap:
    pushal                          # store all the registers
    pushfl
    call keyboard_irq_handler       # call the interrupt handler                 
    popfl      
    popal                           # restore the regesters
    iret

/* mouse_irq_wrap
 *      DESCIRPTION:    wrapper for mouse, store all the registers and restore them
 *                      it will be put in the IDT
 *      INPUT:  none
 *      OUTPUT: execute the mouse interupt handler
 *      RETURN: none
 */
mouse_irq_wrap:
    pushal                          # store all the registers
    pushfl
    call mouse_irq_handler          # call the interrupt handler                 
    popfl      
    popal                           # restore the regesters
    iret


/* rtc_irq_wrap
 *      DESCIRPTION:    wrapper for rtc_irq_handler, store all the registers and restore them
 *                      it will be put in the IDT
 *      INPUT:  none
 *      OUTPUT: execute the rtc interupt handler
 *      RETURN: none
 */
rtc_irq_wrap:
    pushal                          # store all the registers
    pushfl
    call RTC_interrupt              # call the interrupt handler     
    popfl              
    popal                           # restore the regesters
    iret   

/* pit_irq_wrap
 *      DESCIRPTION:    wrapper for pit_interrupt_handler, store all the registers and restore them
 *                      it will be put in the IDT
 *      INPUT:  none
 *      OUTPUT: execute the pit interupt handler
 *      RETURN: none
 */
pit_irq_wrap:
    pushal
    pushfl
    call pit_interrupt_handler
    popfl
    popal
    iret

