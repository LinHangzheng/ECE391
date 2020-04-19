/* idt.h - Defines for IDT functions*/
/* Created by Chenting on 10/19/2019 */

#ifndef _IDT_H
#define _IDt_H

#include "x86_desc.h"
#include "sys_call/sys_call.h"
#include "lib.h"

/* Function used to initialize IDT */
void idt_initialize();
#endif

