/* idt.c - C functions used in IDT management */
/* Created by Chenting on 10/19/2019 */

#include "idt.h"
#include "devices/keyboard.h"
#include "Interrupt_wrapper.h"

/* deifine the number of exceptions */
#define EXCEPTION_NUM 20
#define IDT_SIZE 256
#define EXCEPTION_ERROR 0xF

/* define the interrupt vector for devices */
#define KEYBOARD_INT_VEC    0x21
#define MOUSE_INT_VEC       0x2c
#define RTC_INT_VEC         0x28
#define PIT_INT_VEC         0x20


/* define the interrupt vector for system call */
#define SYS_CALL_VEC        0x80

/* according to IA-32 Architecture Software Deceloper's Manual */
char* excp_name_table[EXCEPTION_NUM] = {
    "0: Divide Error"          ,
    "1: RESERVED"              ,
    "2: Nonmaskable external interrupt"    ,
    "3: Breakpoint"            ,
    "4: Overflow"              ,
    "5: BOUND Range Error"     ,
    "6: Invalid Opcode"        ,
    "7: Devide Not Available"  ,
    "8: Double Fault"          ,
    "9: Coprocessor Segment"   ,
    "10: Invaild TSS"           ,
    "11: Segment Not Present"   ,
    "12: Stack-Segment Fault"   ,
    "13: General Protection"    ,
    "14: Page Fault"            ,
    "15: NOT USED"              ,
    "16: x87 FPU Floating-Point Error"  ,
    "17: Alignment Check"       ,
    "18: Machine Check"         ,
    "19: SIMD Float-Point Exception"    
};



/* 
 * void excp_hder(int excp_vec)
 *  DESCRIPTION: Exceptions handler, that print all necessary info
 *               on screen incluing exception vectors and exception name
 *  INPUTS: excp_vec 8-bit interrupt vector
 *  OUTPUTS: print the corresponding info on screen 
 *  RETURN VALUE: none
 */
void excp_hder(int excp_vec){
    printf("===========================ERROR_DETECTED===========================\n");
    printf("===---\\\\\\        [Exception Number]: %d                    ///---===\n", excp_vec);
    printf("===---\\\\\\        [Error Descirption]: %s       ///---===\n", excp_name_table[excp_vec]);
    printf("====================================================================\n");
    halt(EXCEPTION_ERROR);   /* 0xF for EXCEPTION status */
}


/* functions used to handler each corresponding exception */
#define EXCP_CONSTRUCT(excp_hder_name, excp_vec)  \
void excp_hder_name(void){                        \
    excp_hder(excp_vec);                          \
}                                  


/* construct all exception handlers */
/* 0-19 corresponding to each exception vector */
EXCP_CONSTRUCT  (excp_divide_error, 0);
EXCP_CONSTRUCT  (excp_reserved, 1);
EXCP_CONSTRUCT  (excp_nonmaskable_interrupt, 2);
EXCP_CONSTRUCT  (excp_breakpoint, 3);
EXCP_CONSTRUCT  (excp_overflowr, 4);
EXCP_CONSTRUCT  (excp_bound_range_error, 5);
EXCP_CONSTRUCT  (excp_invalid_opcode, 6);
EXCP_CONSTRUCT  (excp_devide_not_available, 7);
EXCP_CONSTRUCT  (excp_double_fault, 8);
EXCP_CONSTRUCT  (excp_coprocessor_segment, 9);
EXCP_CONSTRUCT  (excp_invaild_tSS, 10);
EXCP_CONSTRUCT  (excp_segment_not_present, 11);
EXCP_CONSTRUCT  (excp_stack_segment_fault, 12);
EXCP_CONSTRUCT  (excp_general_protection, 13);
EXCP_CONSTRUCT  (excp_page_fault, 14);
EXCP_CONSTRUCT  (excp_not_used, 15);
EXCP_CONSTRUCT  (excp_x87_fpu_floating_point_error, 16);
EXCP_CONSTRUCT  (excp_alignment_check, 17);
EXCP_CONSTRUCT  (excp_machine_check, 18);
EXCP_CONSTRUCT  (excp_dimd_float_point, 19);


/*
 * void idt_initialize()
 *  DESCRIPTION: Initialize IDT with interrupt vectors corresponding
 *               to exceptions, interrupts and system calls.
 *  INPUTS: none
 *  OUTPUTS: IDT arrays initialized properly
 *  RETURN VALUE: none
 */
void idt_initialize(){

    /* clear and init all IDT entries */
    int i;  /* loop index */
    for(i = 0; i < IDT_SIZE; i++){
        idt[i].seg_selector = KERNEL_CS;    /* handlers in kernel code segment */
        idt[i].reserved4 = 0;        /* have to be 0 */

        /* Accroding to "wiki.osved.org/Interrupt_Decriptor_Table" */
            /* 0b1110 => 32-bit interrupt gate */
            /* 0b1111 => 32-bit trap gate */

        idt[i].reserved3 = 1;   /* Determine Possible IDT gate types -- 
                                    would be updated to 0 when interrupt handlers */

        idt[i].reserved2 = 1;   /* Determine Possible IDT gate types */
        idt[i].reserved1 = 1;   /* Determine Possible IDT gate types */
        idt[i].size = 1;        /* size=1 => 32-bit space for each handler */

        /* set 0 for interrupt and trap gate */
        idt[i].reserved0 = 0;   /* Determine Possible IDT gate types */

        idt[i].dpl = 0;         /* DPL would be updated when system call */
        idt[i].present = 0;     /* would be updated in SET_IDT_ENTRY */
    }  

    /* setup exception handlers */
    /* 0-19 corresponding to each exception vector */
    SET_IDT_ENTRY(idt[0], excp_divide_error);
    SET_IDT_ENTRY(idt[1], excp_reserved);
    SET_IDT_ENTRY(idt[2], excp_nonmaskable_interrupt);
    SET_IDT_ENTRY(idt[3], excp_breakpoint);
    SET_IDT_ENTRY(idt[4], excp_overflowr);
    SET_IDT_ENTRY(idt[5], excp_bound_range_error);
    SET_IDT_ENTRY(idt[6], excp_invalid_opcode);
    SET_IDT_ENTRY(idt[7], excp_devide_not_available);
    SET_IDT_ENTRY(idt[8], excp_double_fault);
    SET_IDT_ENTRY(idt[9], excp_coprocessor_segment);
    SET_IDT_ENTRY(idt[10], excp_invaild_tSS);
    SET_IDT_ENTRY(idt[11], excp_segment_not_present);
    SET_IDT_ENTRY(idt[12], excp_stack_segment_fault);
    SET_IDT_ENTRY(idt[13], excp_general_protection);
    SET_IDT_ENTRY(idt[14], excp_page_fault);
    SET_IDT_ENTRY(idt[15], excp_not_used);
    SET_IDT_ENTRY(idt[16], excp_x87_fpu_floating_point_error);
    SET_IDT_ENTRY(idt[17], excp_alignment_check);
    SET_IDT_ENTRY(idt[18], excp_machine_check);
    SET_IDT_ENTRY(idt[19], excp_dimd_float_point);

    /* setup interrupt handlers */

    /* setup the PIT handler */
    SET_IDT_ENTRY(idt[PIT_INT_VEC],pit_irq_wrap);
    /* change the gate type into interrupt gate */
    idt[PIT_INT_VEC].reserved3 = 0;

    /* setup the keyboard handler */
    SET_IDT_ENTRY(idt[KEYBOARD_INT_VEC], keyboard_irq_wrap);
    /* change the gate type into interrupt gate */
    idt[KEYBOARD_INT_VEC].reserved3 = 0;

    /* setup the mouse handler */
    SET_IDT_ENTRY(idt[MOUSE_INT_VEC], mouse_irq_wrap);
    /* change the gate type into interrupt gate */
    idt[MOUSE_INT_VEC].reserved3 = 0;

    /* setup the RTC handler */
    SET_IDT_ENTRY(idt[RTC_INT_VEC], rtc_irq_wrap);
    /* change the gate type into interrupt gate */
    idt[RTC_INT_VEC].reserved3 = 0;

    /* setup system-calls */
    SET_IDT_ENTRY(idt[SYS_CALL_VEC], sys_calls);
    /* change the DPL to 3 */
    idt[SYS_CALL_VEC].dpl = 3;
    
}







