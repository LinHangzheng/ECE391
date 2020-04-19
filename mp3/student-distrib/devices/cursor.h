

#ifndef _CURSOR_H
#define _CURSOR_H

#include "../types.h"
#include "../lib.h"

#define SCREEN_WIDTH 80
#define OFFSET_8 8

/* Enabling the Cursor */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
/* Disabling the Cursor */
void disable_cursor();

/* Moving the Cursor */
void update_cursor(int x, int y);
/* Get Cursor Position */
uint16_t get_cursor_position(void);


#endif

