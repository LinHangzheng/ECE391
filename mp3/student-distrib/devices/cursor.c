/* cursor.c - operational functions of cursor(device) control */
/* Reference on https://wiki.osdev.org/Text_Mode_Cursor */


#include "cursor.h"

/* Reference on https://wiki.osdev.org/Text_Mode_Cursor */
/** enable_cursor
 * @DESCRIPTION:
 * 	Enabling the cursor also allows you to set the start and end scanlines, 
 * 	the rows where the cursor starts and ends. The highest scanline is 0 and 
 * 	the lowest scanline is the maximum scanline (usually 15). 
 * @INPUT:	uint8_t	cursor_start -- the highest scanline
 * 			uint8_t cursor_end	 -- the lowest scanline
 * @RETURN: None
 * @OUTPUT:	cursor enabled with given scanline info
 * 
 */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end){
	outb(0x0A, 0x3D4);
	outb((inb(0x3D5) & 0xC0) | cursor_start, 0x3D5);
 
	outb(0x0B, 0x3D4);
	outb((inb(0x3D5) & 0xE0) | cursor_end, 0x3D5);
}

/* Reference on https://wiki.osdev.org/Text_Mode_Cursor */
/** disable_cursor
 * @DESCRIPTION:
 * 	Function used to diable cursor.
 * @INPUT:	None
 * @RETURN: None
 * @OUTPUT:	cursor disabled 
 * 
 */
void disable_cursor(){
    outb(0x0A, 0x3D4);
	outb(0x20, 0x3D5);
}

/* Reference on https://wiki.osdev.org/Text_Mode_Cursor */
/** update_cursor
 * @DESCRIPTION:
 * 	Keep in mind that you don't need to update the cursor's 
 *  location every time a new character is displayed. 
 *  It would be faster to instead only update it after 
 *  printing an entire string. 
 * @INPUT:	int	x -- x coordinate of cursor 
 * 			int y -- y coordinate of cursor 
 * @RETURN: None
 * @OUTPUT:	cursor updated to expected position
 * 
 */
void update_cursor(int x, int y){

    uint16_t pos = y * SCREEN_WIDTH + x;
 
	outb(0x0F, 0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> OFFSET_8) & 0xFF), 0x3D5);
}


/* Reference on https://wiki.osdev.org/Text_Mode_Cursor */
/** get_cursor_position
 * @DESCRIPTION:
 * 	With this code, you get: pos = y * VGA_WIDTH + x. 
 *  To obtain the coordinates, just calculate: y = pos / VGA_WIDTH; 
 *  x = pos % VGA_WIDTH;. 
 * @INPUT:	None
 * @RETURN: uint16_t pos -- current linear position of cursor, y * VGA_WIDTH + x
 * @OUTPUT:	None
 * 
 */
uint16_t get_cursor_position(void){
    uint16_t pos = 0;
    outb(0x0F, 0x3D4);
    pos |= inb(0x3D5);
    outb(0x0E, 0x3D4);
    pos |= ((uint16_t)inb(0x3D5)) << OFFSET_8;
    return pos;
}
