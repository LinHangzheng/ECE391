#ifndef _RTC_H
#define _RTC_H

#include    "../types.h"
#include    "../lib.h"
#include    "i8259.h"

/* RTC command and data port */
#define     RTC_PORT_CMD    0x70
#define     RTC_PORT_DATA   0x71

/* RTC registers */
#define     RA              0x8A
#define     RB              0x8B
#define     RC              0x8C

#define     RTC_PIC_IDX     8
#define     INI_RATE        15

#define     LOW_FOUR_BIT 0x0F   
#define     BITS_FIVE_2_EIGHT 0xF0
#define     BIT_SEVEN  0x40
#define     RTC_1024HZ      1024
#define     RTC_2HZ         2

/* Initialize the RTC and set the rate to 1024HZ */
uint32_t RTC_init();
/* Change the RTC rate base on the input rate */
int RTC_change_freq();
/* The RTC handler */
void RTC_interrupt();
/* change the rate of the RTC */
void RTC_change_rate(uint32_t rate);
/*  change the frequence to rate */
int32_t freq2rate(int32_t freq);
/* change the RTC frequence to 2HZ */
int32_t RTC_open(const uint8_t* filename);
/* Do nothing, just return 0s */
int32_t RTC_close(int32_t fd);
/* Wait until the RTC interrupt occur */
int32_t RTC_read(int32_t fd,  uint32_t offset, void* buf, int32_t nbytes);
/* change the frequence based on the buf  */
int32_t RTC_write(int32_t fd, const void* buf, int32_t nbytes);
#endif
