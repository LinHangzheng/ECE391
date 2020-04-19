#ifndef _PIT_H
#define _PIT_H

/*I/O ports for the PIT*/
#define PIT_CMD_PORT    0x43
#define PIT_DATA_PORT   0x40

#define PIT_IRQ         0x0

/*Constants to set the PIT frequency*/
#define PIT_MODE        0x37            /*Select Channel 0, Mode 3*/
#define PIT_FREQ_DIVISOR     11932      /*Set the PIT frequency to 100hz*/

void pit_init(void);
void pit_interrupt_handler(void);
#endif

