

#ifndef _PAGE_H
#define _PAGE_H

#include "x86_desc.h"
#include "lib.h"

#define ENTRY_NUM  1024
#define OFFSET_12 12

#define VIDEO 0xB8000
#define USER_VIDEO_MEM_PDE 33

void SET_PTEMAP_PARAMS(int index, int present);
void SET_PDEMAP_KB_PARAMS(int index, int present);
void SET_PTE_PARAMS(int index, int present);

void SET_USER_VIDEO_MAP(int index, int present);
void SET_PDEMAP_KB_PARAMS(int index, int present);
// ini page: set the tabke value and entry and 
// change the value of reg
void init_page();

#endif /* _PAGE_H */
