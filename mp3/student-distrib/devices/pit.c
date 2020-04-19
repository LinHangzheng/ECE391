#include "../types.h"
#include "../lib.h"
#include "i8259.h"
#include "pit.h"
#include "../process.h"
#include "../terminal.h"

/* void pit_init(void)
 * @DESCRIPTION: Initialize the PIT and set the pit frequency to 100HZ
 * @INPUTS:  none
 * @OUTPUTS: none
 * @RETURN VALUE: 0
 * @SIDE EFFECTS: Allow the pit interrupts with the frequency of 100HZ
 */

void pit_init(void){  
    outb(PIT_MODE,PIT_CMD_PORT);
    /*sent low 8 bits of divisor*/
    outb((uint8_t)PIT_FREQ_DIVISOR && 0xff, PIT_DATA_PORT);
    /*sent high 8 bits of divisor*/
    outb((uint8_t)(PIT_FREQ_DIVISOR >> 8),PIT_DATA_PORT);
    
    enable_irq(PIT_IRQ);
}


/* void pit_interrupt_handler(void)
 * @DESCRIPTION: Handle the interrupt sent by PIT and performs context switch
 *               per 10 ms
 * @INPUTS:  none
 * @OUTPUTS: none
 * @RETURN VALUE: 0
 * @SIDE EFFECTS: Switch to another terminal every 10 ms
 */
void pit_interrupt_handler(void){
    send_eoi(PIT_IRQ);
    video_mem_switch(running_terminal->next_terminal->tid);
    process_switch();
    /*Perform Context switch*/
}


