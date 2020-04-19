/* fop.c - functions for file operations table management */
/* Created by Chenting on 10/28/2019 */


#include "fop.h"
#include "terminal.h"
#include "devices/RTC.h"
#include "devices/fs.h"

/* define constants for success and fail */
#define SUCCESS 0
#define FAIL    -1

/* get_terminal_fop
 *  
 *  DESCRIPTION: the function builds an operation table for terminal
 *               based on the functions defined imn terminal.h          
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: (fop_t) ret -- the operation table for ternimal operations
*/
fop_t get_terminal_fop(void){
    fop_t ret;
    ret.open = terminal_open;
    ret.close = terminal_close;
    ret.read = terminal_read;
    ret.write = terminal_write;
    return ret;
}


/* get_RTC_fop
 *  
 *  DESCRIPTION: the function builds an operation table for RTC
 *               based on the functions defined imn RTC.h          
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: (fop_t) ret -- the operation table for RTC operations
*/
fop_t get_RTC_fop(void){
    fop_t ret;
    ret.open = RTC_open;
    ret.close = RTC_close;
    ret.read = RTC_read;
    ret.write = RTC_write;
    return ret;
}

/* get_file_fop
 *  
 *  DESCRIPTION: the function builds an operation table for file
 *               based on the functions defined imn file.h          
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: (fop_t) ret -- the operation table for file operations
*/
fop_t get_file_fop(void){
    fop_t ret;
    ret.open = file_open;
    ret.close = file_close;
    ret.read = file_read;
    ret.write = file_write;
    return ret;
}

/* get_dir_fop
 *  
 *  DESCRIPTION: the function builds an operation table for dir
 *               based on the functions defined imn dir.h          
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: (fop_t) ret -- the operation table for dir operations
*/
fop_t get_dir_fop(void){
    fop_t ret;
    ret.open = dir_open;
    ret.close = dir_close;
    ret.read = dir_read;
    ret.write = dir_write;
    return ret;
}



/* init_fop_table
 *  
 *  DESCRIPTION: the function builds an operation table for dir
 *               based on the functions defined imn dir.h          
 *  INPUTS: none
 *  OUTPUTS: fop_table -- initialize the whole file operation tables array 
 *  RETURN VALUE: 0 for success
 */
int32_t init_fop_table(void){

    /* assign corresponding driver operations table in array */
    fop_table[TEMINAL_IDX] = get_terminal_fop();
    fop_table[RTC_IDX] = get_RTC_fop();
    fop_table[FILE_IDX] = get_file_fop();
    fop_table[DIR_IDX] = get_dir_fop();

    /* return 0 for success */
    return SUCCESS;
    
}

