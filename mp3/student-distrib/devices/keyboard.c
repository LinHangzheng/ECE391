/* keyboard.c - Functions to interact with the keyboard
 */

#include "keyboard.h"
#include "../lib.h"
#include "../scancode_dict.h" 
#include "../process.h"
#include "../terminal.h"

/* keyboard buffer info */
uint8_t buf_pos = 0;
char* keyboard_buf = NULL;

/* used in synchronization with terminal read */
volatile uint8_t stdin_enable = 0;

/* variables to record status of special keys */
uint8_t CtrlPressed = 0;
uint8_t ShiftPressed = 0;
uint8_t CapsLockActive = 0;
uint8_t CapsLockPressed = 0;
uint8_t BackspacePressed = 0;
uint8_t AltPressed = 0;

/* 
 * keyboard_init
 *  DESCRIPTION: Tell the PIC begin to accept keyboard interrupt
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Begin to acceept keyboard interrupt
 */
void keyboard_init(void){
    /* enable keyboard interrupt on PIC */
    enable_irq(KEYBOARD_IRQ_NUM);
}

/* 
 * keyboard_irq_handler
 *  DESCRIPTION: Handle the keyboard Interrupt, read in the scancode and print
 *               the corresponding character to the screen. Only can print lowercase
 *               character by now               
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: print a character on the screen
 */
void keyboard_irq_handler(void){

    /* send EOI before exception handled */
    send_eoi(KEYBOARD_IRQ_NUM);

    /* read scancode from keyboard */
    uint8_t scancode = inb(KEYBOARD_PORT_DATA) & LOW_8_BITS;  

    /* check whether input scancode is special */
    if(handle_special_key(scancode)) return;

    /* check whther input scancode is avaiable in reserved dict */
    if (scancode >= DICT_SIZE) return;              

    /* turn scandoce into corresponding ASCII */
    uint8_t keystrock = scancode_dict[scancode][0];

    /* handle for "Ctrl+l" -- clear up the screen */
    if(CtrlPressed){
        if(keystrock == 'l'){
            clear();
            return;
        }
    }

    /* handle for normal keys */
    if (keystrock >= 'a' && keystrock <= 'z'){
        /* handle for letters */
        if (CapsLockActive != ShiftPressed){
            keystrock = scancode_dict[scancode][1];
        }
    }else{
        /* handle others */
        if (ShiftPressed){
            keystrock = scancode_dict[scancode][1];
        }
    }

    /* return if keyborad buffer full */
    if ((buf_pos >= MAX_BUF_SIZE - 1) && keystrock != '\n') return;

    /* put the key onto current screen*/
    printkey_on_curr_terminal(keystrock);
    keyboard_buf[buf_pos] = keystrock;
    buf_pos ++;

    /* handle case of '\n' */
    if (keystrock == '\n'){
        /* enable the terminal read */
        get_cur_terminal()->stdin_enable= 1;
        /* clean up the buffer */
        buf_pos = 0;
    }

    return;

}

/* 
 * keyboard_read (int32_t fd, void* buf, int32_t nbytes)
 *  DESCRIPTION: read the value stored in the key board buf
 *                 
 *  INPUTS: int32_t fd, not used yet
 *          void* buf place to read into, 
 *          int32_t nbytes, size to read
 *  OUTPUTS: fill the input buf
 *  RETURN VALUE: # of byte copied
 */
int32_t keyboard_read (int32_t fd,  uint32_t offset, void* buf, int32_t nbytes){
    /*sanity check*/
    if (buf == NULL){return -1;}
    
    int32_t char_count;
    int32_t copy_count = 0;
    
    running_terminal->stdin_enable = 0;
    /* wait until user press enter */
    while (!(running_terminal->stdin_enable)){}

    for (char_count = 0; char_count < nbytes; char_count++){
        /* Handle buffer overflow */
        if (char_count >= MAX_BUF_SIZE){break;}
        /* Read until new line character */
        if (keyboard_buf[char_count] != '\n'){
            ((char*)buf)[char_count] = keyboard_buf[char_count];
            copy_count++;
        }
        else
        {
            /* copy the new line character */
            ((char*)buf)[char_count] = keyboard_buf[char_count];
            char_count++;
            copy_count++;
            /* fill the user buffer with 0 */
            for(;char_count<nbytes;char_count++){
                ((char*)buf)[char_count] = 0;
            }  
            return copy_count;
        }
    }

    return copy_count;
}




/* 
 * handle_special_key(uint8_t scancode)
 *  DESCRIPTION: judge whether the key is a special key
 *                 
 *  INPUTS: uint8_t scancode, the input key
 *  OUTPUTS: none
 *  RETURN VALUE: 1 for special key detected and handled
 *                0 otherwise
 *  SIDE EFFECTS: none
*/
int32_t handle_special_key(uint8_t scancode){
    switch(scancode){
        
        /* detect Shift pressed */
        case LEFT_SHIFT_PRESS:
        case RIGHT_SHIFT_PRESS: 
            ShiftPressed = 1;
            return 1;
        
        /* detect Shift released */
        case LEFT_SHIFT_RELEASE:
        case RIGHT_SHIFT_RELEASE:
            ShiftPressed = 0;
            return 1;

        /* detect Ctrl pressed */
        case LEFT_CTRL_PRESS:
            CtrlPressed = 1;
            return 1;
        
        /* detect Ctrl released */
        case LEFT_CTRL_RELEASE:
            CtrlPressed = 0;
            return 1;

        /* detect Capslock pressed */
        case CAPSLOCK_PRESS:
            /* update status once per press */
            if (!CapsLockPressed){
                /* switch statu for CapsLock if necessary */
                CapsLockActive = 1 - CapsLockActive;
                CapsLockPressed = 1;
            }
            return 1;

        /* detect Capslock released */
        case CAPSLOCK_RELEASE:
            /* record CapsLock be released */
            CapsLockPressed = 0;
            return 1;

        /* detect Backspace pressed */
        case BACKSPACE:
            /* delete char on current screen once  */
            if (buf_pos!=0){
                printkey_on_curr_terminal(ASCILL_BACKSPACE);
                buf_pos--;
            }
            return 1;
        
        /* detect Alt pressed */
        case ALT_PRESS:
            AltPressed = 1;
            return 1;

        /* detect Alt released */
        case ALT_RELEASE:
            AltPressed = 0;
            return 1;

        /* handle for "Alt + Fn" -- switch terminal */
        case F1:
            if (AltPressed){
                /* switch to terminal #1 */
                terminal_switch(0);
            }
            return 1;
        case F2:
            if (AltPressed){
                /* switch to terminal #2 */
                terminal_switch(1);
            }
            return 1;
        case F3:
            if (AltPressed){
                /* switch to terminal #2 */
                terminal_switch(2);
            }
            return 1;

         /* detect ESC pressed */
        case ESC:
            return 1;
        
        /* handle for "Tab" -- auto complete excuted */
        case TAB_PRESS:
            keyboard_auto_complete();
            return 1;

        /* input key is not special */
        default:
            return 0;
    }
}

/* 
 * keyboard_clean()
 *  DESCRIPTION: clean the keyboard buf
 *                 
 *  INPUTS: none
 *  OUTPUTS: keyboard buf clear to 0
 *  RETURN VALUE: none
 */
void keyboard_clean(void){
    /* clear the keyboard buf */
    running_terminal->buf_pos = 0;
    memset(running_terminal->keyboard_buf,0,MAX_BUF_SIZE);
}


/* 
 * keyboard_auto_complete()
 *  DESCRIPTION: auto complete when "tab" typed
 *                 
 *  INPUTS: none
 *  OUTPUTS: auto complete the current command/argument
 *  RETURN VALUE: none
*/
void keyboard_auto_complete(void){

    if (buf_pos >= MAX_FILENAME_LEN){return;}
    /* record the current typed command */
    char CURR_TYPED_CMD[MAX_FILENAME_LEN+1];

    /* parameters used in detecting command */
    int32_t read_count = 0;
    int32_t buf_read_pos = 0;

    /* get the current typed command */
    while(buf_read_pos != buf_pos){
        /* step empty-space and redetect command */
        if(keyboard_buf[buf_read_pos] == ' '){
            buf_read_pos++;
            read_count = 0;
            continue;
        }
        /* copy into buffer */
        CURR_TYPED_CMD[read_count] = keyboard_buf[buf_read_pos];
        buf_read_pos++;
        read_count++;
    }

    /* do nothing if typed put of range */
    if(read_count > MAX_FILENAME_LEN){
        return;
    }

    /* set '\0' to the end of buffer */
    CURR_TYPED_CMD[read_count] = '\0';

    /* buffer for the current filename stored in fs */
    char CMD_SEARCH_BUF[MAX_FILENAME_LEN + 1];

    /* buffer for the current possible filename found */
    char CMD_TARGET_BUF[MAX_FILENAME_LEN + 1];
    int32_t target_buf_been_occupied = 0;
    int32_t target_cmd_len = 0;

    /* read one file name in dentry per time */
    int32_t file_read_count = 0;
    int32_t curr_filename_len = 0;
    while(0 != (curr_filename_len = dir_read(0, file_read_count, CMD_SEARCH_BUF, MAX_FILENAME_LEN + 1))){
        file_read_count++;
        if(0 == strncmp(CMD_SEARCH_BUF, CURR_TYPED_CMD, read_count)){
            /* return if there are more than 1 possible command */
            if(target_buf_been_occupied == 1) return;
            strncpy(CMD_TARGET_BUF, CMD_SEARCH_BUF, MAX_FILENAME_LEN + 1);
            target_buf_been_occupied = 1;
            target_cmd_len = curr_filename_len;
        }
    }

    /* first find the position to start auto-complete */
    int32_t cmp_pos = 0;
    while(CMD_TARGET_BUF[cmp_pos] == CURR_TYPED_CMD[cmp_pos]){
        cmp_pos++;
    }

    /* do auto-complete */
    int32_t curr_pos = cmp_pos;
    while(curr_pos < target_cmd_len){
        printkey_on_curr_terminal(CMD_TARGET_BUF[curr_pos]);
        keyboard_buf[buf_pos++] = CMD_TARGET_BUF[curr_pos];
        curr_pos++;
        /* return when buffer is full */
        if(buf_pos == (MAX_BUF_SIZE - 1)){
            return;
        }
    }

}

/* 
 * printkey_on_curr_termianl()
 *  DESCRIPTION: put one key on the current terminal we are looking at
 *  INPUTS: none
 *  OUTPUTS: put the cur_terminal
 *  RETURN VALUE: none
*/
void printkey_on_curr_terminal(uint8_t keystroke){
    video_mem_switch(get_cur_terminal()->tid);
    putc(keystroke);
    update_cursor(*screen_x, *screen_y);
    video_mem_switch(running_terminal->tid);
}


/* 
 * printf_on_curr_terminal()
 *  DESCRIPTION: put a string on the current terminal we are looking at
 *                 
 *  INPUTS: none
 *  OUTPUTS: put a string on the current terminal we are looking at   
 *  RETURN VALUE: none
*/
void printf_on_curr_terminal(int8_t* string){
    video_mem_switch(get_cur_terminal()->tid);
    printf(string);
    update_cursor(*screen_x,*screen_y);
    video_mem_switch(running_terminal->tid);
}







