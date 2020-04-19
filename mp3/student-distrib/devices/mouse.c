#include "mouse.h"
#include "keyboard.h"
#include "../terminal.h"
#define SUCCESS  0
#define FAIL    -1
int32_t mouse_x = 0;
int32_t mouse_y = 0;
uint8_t mouse_left = 0;
uint8_t mouse_middle = 0;
uint8_t mouse_right = 0;




/* 
 * mouse_init(void)
 *  DESCRIPTION: To initializ the mouse interrupt
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: The mouse has been initialized 
 */
void mouse_init(void){
    /*
     * Send the Enable Auxiliary Device command (0xA8) to port MOUSE_PORT. 
     * This will generate an ACK response from the keyboard, which you must wait to receive. 
     */
    _output_wait();outb(0xa8,MOUSE_PORT);

    /*0x20:Get Compaq Status Byte, Set Compaq Status/Enable IRQ12*/
    _output_wait();outb(0x20,MOUSE_PORT);
    

    /*The very next byte returned should be the Status byte*/
    _input_wait();
    uint8_t status = inb(KEYBOARD_PORT);
    status |= 2;
    status &= 0xdf;

    _output_wait();outb(KEYBOARD_PORT,MOUSE_PORT);
    _output_wait();outb(status,KEYBOARD_PORT);

    /*F6:Disables streaming, sets the packet rate to 100 per second, and resolution to 4 pixels per mm.*/
    write_60(0xf6);
    read_60();
    /*F4: The mouse starts sending automatic packets when the mouse moves or is clicked.*/
    write_60(0xf4);
    read_60();
    /*Requires an additional data byte: automatic packets per second*/
    write_60(0xf3);
    read_60();
    _output_wait();outb(200,KEYBOARD_PORT);

    enable_irq(MOSUE_IRQ_NUM);
    
}

/* 
 * mouse_irq_handler(void)
 *  DESCRIPTION: mouse interrupt handler, it will change the position of the 
 *               mouse and make the interraction with the mouse click
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: change the mouse state and handle click assignments
 */
void mouse_irq_handler(void){
    send_eoi(MOSUE_IRQ_NUM);
    mouse_packet_t packet;
    packet.val = read_60();
    /*Check whether we should receive the packet*/
    if (packet.x_overflow || packet.y_overflow || !packet.always1)return;
    /*Get the delta x and y, used to update the position of the mouse*/
    int32_t x_move = read_60();
    int32_t y_move = read_60();

    /*Sign check, if it is negative, change its value to approprate amount*/
    if (packet.x_sign){x_move = 0xFFFFFF00 | x_move;}
    if (packet.y_sign){y_move = 0xFFFFFF00 | y_move;}

    int32_t new_x;
    int32_t new_y;
    /*Change the fluency of the mouse*/
    x_move /= 3;
    y_move /= 3;

    /*Calculate the new position of the mouse*/
    new_x = mouse_x + x_move;
    new_y = mouse_y - y_move;

    /*Boundary check*/
    if(new_x < 0)new_x = 0;
    if(new_y < 0)new_y = 0;
    if(new_x > SCREEN_WIDTH-1)new_x = SCREEN_WIDTH-1;
    if(new_y > SCREEN_HEIGHT-1 )new_y = SCREEN_HEIGHT-1;

    /*If the position fo the mouse is changed, clear old highlight*/
    if (new_x != mouse_x || new_y != mouse_y){
        change_original_color(mouse_x,mouse_y);
    }
    /*update the new mouse position*/
    mouse_x = new_x;
    mouse_y = new_y;
    change_background_color(mouse_x,mouse_y);
    
    /*mouse left click to change the terminal*/
    if (packet.btn_left){
        if (mouse_y >=1 && mouse_y <=3){
            if(mouse_x >= 19 && mouse_x <=23) {change_original_color(mouse_x,mouse_y);terminal_switch(0);}
            if(mouse_x >= 25 && mouse_x <=29) {change_original_color(mouse_x,mouse_y);terminal_switch(1);}
            if(mouse_x >= 31 && mouse_x <=35) {change_original_color(mouse_x,mouse_y);terminal_switch(2);}
        }   
    }
}


/* 
 *  write_60(uint8_t data)
 *  DESCRIPTION: Sending a command or data byte to the mouse (to port KEYBOARD_PORT)
 *               must be preceded by sending a 0xD4 byte to port MOUSE_PORT (with
 *               appropriate waits on port MOUSE_PORT, bit 1, before sending each
 *               output byte).
 *  INPUTS: uint8_t data
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: 
 */
void write_60(uint8_t data){
    _output_wait();outb(0xd4,MOUSE_PORT);
    _output_wait();outb(data,KEYBOARD_PORT);
}


/* 
 *  read_60(void)
 *  DESCRIPTION: read the data from the port KEYBOARD_PORT
 *  INPUTS: uint8_t data
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: 
 */
uint8_t read_60(void){
    _input_wait();
    return inb(KEYBOARD_PORT);
}

/* 
 * _output_wait(void)
 *  DESCRIPTION: All output to port KEYBOARD_PORT or MOUSE_PORT must be preceded by waiting
 *               for bit 1 (value=2) of port MOUSE_PORT to become clear. 
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: 
 */
void _output_wait(void){
    int i = 100000;
    while(i-- &&inb(MOUSE_PORT)&2);
}

/* 
 * void _input_wait(void)
 *  DESCRIPTION: Similarly, bytes cannot be read from port 
 *               KEYBOARD_PORT until bit 0 (value=1) of port MOUSE_PORT is set.
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: 
 */
void _input_wait(void){
    int i = 100000;
    while(i-- && inb(MOUSE_PORT)&1);
}
