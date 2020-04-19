#include "RTC.h"
#include "../process.h"

/* uint32_t RTC_init()
 * @DESCRIPTION: Initialize the RTC and set the rate to 1024HZ
 * @INPUTS:  none
 * @OUTPUTS: none
 * @RETURN VALUE: 0
 * @SIDE EFFECTS: Allow the RTC interrupt.
 */

#define SUCCESS  0
#define FAIL    -1
uint32_t RTC_init(){
    /*Refrence from: https://wiki.osdev.org/RTC */
    outb(RB,RTC_PORT_CMD);		                // select register B, and disable NMI
    char prev=inb(RTC_PORT_DATA);	            // read the current value of register B
    outb(RB,RTC_PORT_CMD);		                // set the index again (a read will reset the index to register D)
    outb(prev | BIT_SEVEN, RTC_PORT_DATA);	        // write the previous value ORed with 0x40. This turns on bit 6 of register B

    /* Set the initial rate into 6*/
    RTC_change_freq(RTC_1024HZ);

    /* Enable the interupt of the PTC pin (8)*/
    enable_irq(RTC_PIC_IDX);
    return SUCCESS;
}

/* void RTC_change_freq(uint32_t rate)
 * @DESCRIPTION: Change the RTC rate base on the input rate
 * @INPUTS:  uint32_t rate - The RTC rate
 * @OUTPUTS: none
 * @RETURN VALUE: none 
 * @SIDE EFFECTS: The RTC rate changed.
 */
int RTC_change_freq(uint32_t freq){
     /*Refrence from: https://wiki.osdev.org/RTC */
    uint32_t rate = freq2rate(freq);
    
    /* If the freq is not power of 2 return -1*/
    if(rate == FAIL){return FAIL;}

    outb(RA, RTC_PORT_CMD);		                    // set index to register A, disable
    char prev= inb(RTC_PORT_DATA);	                // get initial value of register A
    outb(RA, RTC_PORT_CMD);		                    // reset index to A
    outb((prev & 0xF0) | rate,RTC_PORT_DATA);       //write only our rate to A. Note, rate is the bottom 4 bits.
    return SUCCESS;
}

/* void RTC_interrupt()
 * @DESCRIPTION: The RTC handler
 * @INPUTS:  none
 * @OUTPUTS: none
 * @RETURN VALUE: none 
 * @SIDE EFFECTS: interrupt begins 
 */
void RTC_interrupt(){
    int32_t pid;
    int32_t fd;
    for (pid = 0;pid < MAX_PROCESSES;pid++){
        /* if no more process, break */
        if (process_bitmap[pid] == 0){continue;}
        
        /* if the process didn't open rtc, continue */
        process_t* process_ptr = get_process_ptr(pid);
        if (process_ptr->rtc_ival == 0){continue;}

        /* Find the rtc on fd array and increment the rtc counter*/
        for (fd = 0;fd < FILE_ARRAY_SIZE;fd++){
            if (process_ptr->fd_array[fd].inode == -1){
                process_ptr->fd_array[fd].file_position++;
                break;
            }
        }
    }
    /*Refrence from: https://wiki.osdev.org/RTC */
    outb(RC, RTC_PORT_CMD);	                        // select register C
    inb(RTC_PORT_DATA);		                        // just throw away contents
    send_eoi(RTC_PIC_IDX);
}

/* int32_t RTC_open(const uint8_t* filename)
 * @DESCRIPTION: change the RTC frequence to 2HZ
 * @INPUTS:  const uint8_t* filename
 * @OUTPUTS: none
 * @RETURN VALUE: 0 for success
 * @SIDE EFFECTS: change the RTC frequence to 2HZ
 */
int32_t RTC_open(const uint8_t* filename){
    process_t* calling_process = get_process_ptr(curr_pid);
    /* initialize the RTC frequence to 2HZ */
    calling_process->rtc_ival = RTC_1024HZ/RTC_2HZ;
    return SUCCESS;
}

/* int32_t RTC_close(int32_t fd
 * @DESCRIPTION: Do nothing, just return 0
 * @INPUTS:  int32_t fd
 * @OUTPUTS: none
 * @RETURN VALUE: 0 for success
 * @SIDE EFFECTS: none
 */
int32_t RTC_close(int32_t fd){
    process_t* calling_process = get_process_ptr(curr_pid);
    calling_process->rtc_ival = 0;
    calling_process->fd_array[fd].file_position = 0;
    return SUCCESS;
}

/* int32_t RTC_read(int32_t fd, void* buf, int32_t nbytes)
 * @DESCRIPTION: Wait until the RTC interrupt occur
 * @INPUTS:  int32_t fd
 *           void* buf
 *           int32_t nbytes
 * @OUTPUTS: none
 * @RETURN VALUE: 0 for success
 * @SIDE EFFECTS: none
 */
int32_t RTC_read(int32_t fd,  uint32_t offset, void* buf, int32_t nbytes){
    /*block until the next interrupt occur*/
    process_t* calling_process = get_process_ptr(curr_pid);
    int* file_position = &(calling_process->fd_array[fd].file_position);
    int32_t rtc_ival = calling_process->rtc_ival;

    while((*file_position) < rtc_ival){}
    *file_position = 0;
    return SUCCESS;
}

/* int32_t RTC_write(int32_t fd, const void* buf, int32_t nbytes)
 * @DESCRIPTION: change the frequence based on the buf 
 * @INPUTS:  int32_t fd
 *           void* buf
 *           int32_t nbytes
 * @OUTPUTS: none
 * @RETURN VALUE: 0 for success
 * @SIDE EFFECTS: RTC frequence will be changed
 */
int32_t RTC_write(int32_t fd, const void* buf, int32_t nbytes){
    if (buf == NULL){return FAIL;}
    /* if the freq is invalid return fail */
    int32_t freq = *(int32_t*)buf;
    if (freq2rate(freq) == FAIL){return FAIL;}

    /* set the rtc freq in current process */
    process_t* calling_process = get_process_ptr(curr_pid);
    calling_process->rtc_ival = RTC_1024HZ/freq;
    
    return SUCCESS;
}


/* int32_t freq2rate(int32_t freq)
 * @DESCRIPTION: change the frequence to rate
 * @INPUTS:  int32_t freq - frequence need to change
 * @OUTPUTS: none
 * @RETURN VALUE: success - rate
                  Fail - -1
 * @SIDE EFFECTS: interrupt begins 
 */
int32_t freq2rate(int32_t freq){
    switch (freq){
        case 2:     return 0x0F;
        case 4:     return 0x0E;
        case 8:     return 0x0D;
        case 16:    return 0x0C;
        case 32:    return 0x0B;
        case 64:    return 0x0A;
        case 128:   return 0x09;
        case 256:   return 0x08;
        case 512:   return 0x07;
        case 1024:  return 0x06;
        // case 2048:  return 0x05;
        // case 4096:  return 0x04;
        // case 8192:  return 0x03;
        default:    return FAIL;
    }
}

