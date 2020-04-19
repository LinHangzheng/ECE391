/*wrapper for interrupt handler, need to store all the regestors before calling and restore
 *them afterwards
 */

#ifndef _INTERRUPT_WRAPPER_H_
#define _INTERRUPT_WRAPPER_H_


#ifndef ASM
    /*wrapper for keyboard_irq_handler*/
    extern void keyboard_irq_wrap();
    /*wrapper for mouse_irq_handler*/
    extern void mouse_irq_wrap();
    /*wrapper for RTC_interrupt*/
    extern void rtc_irq_wrap();
    /*wrapper for PIT_interupt*/
    extern void pit_irq_wrap();
#endif

#endif
