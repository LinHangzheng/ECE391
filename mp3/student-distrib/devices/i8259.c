/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "../lib.h"

#define MASK_ALL 0xFF       /*mask all the interrupts*/
#define PIC_COUNT 2         /*The number of pics*/
/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = MASK_ALL; /* IRQs 0-7  */
uint8_t slave_mask = MASK_ALL;  /* IRQs 8-15 */

/* 
 * i8259_init
 *  DESCRIPTION: Mask all interrupts and exectute the initialize sequence of I8259
 *               Enable the interrupt of pin 2
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: initialize the I8259
 */
/* Initialize the 8259 PIC */
void i8259_init(void) {
    /*first mask all interrupts*/
    outb(MASK_ALL, MASTER_8259_IMR);
    outb(MASK_ALL, SLAVE_8259_IMR);

    /*execute the initailization sequence*/
    outb(ICW1, MASTER_8259_CMD);
    outb(ICW1,SLAVE_8259_CMD);                      //Tell the 8259A the initialization begin
    outb(ICW2_MASTER, MASTER_8259_IMR);
    outb(ICW2_SLAVE, SLAVE_8259_IMR);              //The high bits of of the interupt vector numbers are provided in ICW2
    outb(ICW3_MASTER, MASTER_8259_IMR);
    outb(ICW3_SLAVE, SLAVE_8259_IMR);              
    outb(ICW4, MASTER_8259_IMR);
    outb(ICW4, SLAVE_8259_IMR);

    outb(master_mask, MASTER_8259_IMR);             //restore the mask settings
    outb(slave_mask, SLAVE_8259_IMR);

    /*Enable the slave I8259*/
    enable_irq(MASTER_SLAVE_PIN);
}

/* 
 * enable_irq
 *  DESCRIPTION: Tell I8259 to accept the interrup specified by irq_num
 *  INPUTS: irq_num -- range from 0 - 15
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Enable the PIC to acceept interrupt
 */
/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num) {
    //unsigned long flags;
    /*sanity check*/
    if (irq_num >= PIC_COUNT * I8259_PORTS_NUMBER){return;}           //Only accept interrupt range from 0-15
    
    //irq_num range from 8-15 is on slave
    if (irq_num >= I8259_PORTS_NUMBER){
        slave_mask &= ~(1 << (irq_num - I8259_PORTS_NUMBER));       
        outb(slave_mask, SLAVE_8259_IMR);
    }
    //irq_num range from 0-7 is on master
    else{
        master_mask &= ~(1 << irq_num);
        outb(master_mask,MASTER_8259_IMR);
    }
}


/* 
 * disable_irq
 *  DESCRIPTION: Tell I8259 to disable the interrup specified by irq_num
 *  INPUTS: irq_num -- range from 0 - 15
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: disable the PIC to acceept interrupt
 */
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
    //unsigned long flags;
    /*sanity check*/
    if (irq_num >= PIC_COUNT * I8259_PORTS_NUMBER){return;}         //Only accept interrupt range from 0-15
    //irq_num range from 8-15 is on slave
    if (irq_num >= I8259_PORTS_NUMBER){
        slave_mask |= (1 << (irq_num - I8259_PORTS_NUMBER));
        outb(slave_mask, SLAVE_8259_IMR);
    }
    //irq_num range from 0-7 is on master
    else{
        master_mask |= (1 << irq_num);
        outb(master_mask,MASTER_8259_IMR);
    }
    
}

/* 
 * send_eoi
 *  DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 *  INPUTS: irq_num -- range from 0 - 15
 *  OUTPUTS: none
 *  RETURN VALUE: none
 */
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
    //unsigned long flags;
    /*sanity check*/
    if (irq_num >= PIC_COUNT * I8259_PORTS_NUMBER){return;}
    
    if (irq_num >= I8259_PORTS_NUMBER){
        outb(EOI|(irq_num - I8259_PORTS_NUMBER), SLAVE_8259_CMD);                            //send eoi to the slave
        outb(EOI|MASTER_SLAVE_PIN, MASTER_8259_CMD);         /*should also send eoi to the master, IR2 on master is used to link the slave*/
    }
    else
    {
        outb(EOI|irq_num, MASTER_8259_CMD);
    }


}
