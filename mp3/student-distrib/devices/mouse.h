#ifndef MOUSE_H
#define MOUSE_H
#include    "../types.h"
#include    "../lib.h"
#include    "i8259.h"

#define MOSUE_IRQ_NUM 12
#define SCREEN_HEIGHT 25
#define KEYBOARD_PORT 0x60
#define MOUSE_PORT  0x64

#define NEXT_STATE      0x20   //bit 5


typedef union mouse_packet_t {
    uint8_t val;
    struct {
        uint8_t btn_left    : 1;
        uint8_t btn_right   : 1;
        uint8_t btn_middle  : 1;
        uint8_t always1     : 1;
        uint8_t x_sign      : 1;
        uint8_t y_sign      : 1;
        uint8_t x_overflow  : 1;
        uint8_t y_overflow  : 1;
    } __attribute__ ((packed));
} mouse_packet_t;

/*init the keyboard and begin to accept interrupt*/
void mouse_init(void);
/*The interrupt handler for keystroke*/
void mouse_irq_handler(void);
/* judge whether the key is a special key */

uint8_t read_60(void);

void write_60(uint8_t data);

void _output_wait();
void _input_wait(void);

#endif 


