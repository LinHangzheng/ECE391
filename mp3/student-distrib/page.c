
#include "page.h"

//set one PDE with given value
void SET_PTE_PARAMS(int index, int present);
//set one PDE with given value in MB form
void SET_PDE_MB_PARAMS(int index, int present);
//et one PDE with given value in KB form(point to a page table)
void SET_PDE_KB_PARAMS(int index, int present);
// change registers to turn on the page
void page_on();


/* init_page
 *      DESCIRPTION:    Initialize both PT and PD used in start of booting
 *      INPUT:  none
 *      OUTPUT: PT and PE filled with required entry
 *      RETURN: none
 */
void init_page(void){
    int i;
    //ini PDE for
    for(i = 0; i < ENTRY_NUM; i++){
       // get PDE
        if (i == 0){
            //ini the 0-4 MB
            SET_PDE_KB_PARAMS(i, 1);
        }else if(i ==USER_VIDEO_MEM_PDE){
            SET_PDEMAP_KB_PARAMS(i,1);
        }
        else if (i == 1){
            //ini for 4-8 MB
            SET_PDE_MB_PARAMS(i, 1);

        }else{
            //useless init
            SET_PDE_MB_PARAMS(i, 0);

        }
        //PD ini done
    }

    // init PTE                                 //bug log: "}" hcanges the meaning
    for(i = 0; i < ENTRY_NUM; i++){
        SET_USER_VIDEO_MAP(i,0);
        if((i << OFFSET_12) != VIDEO){
            SET_PTE_PARAMS(i,0);
        }else{
            SET_PTE_PARAMS(i,1);
        }
    }

    page_on();
    // while(1);
}


 /* SET_PDE_PARAMS
 *      DESCIRPTION: set one PDE with given value in KB form(point to a page table)
 *      INPUT:  index: index in the PDE, 
 *              present: whther this entry is present in pythical memory.  
 *      OUTPUT: PTE set at PD[index] 
 *      RETURN: none
 */
void SET_PDE_KB_PARAMS(int index, int present){
        // change the value accordingly
        page_directory[index].KB.present = present;
        page_directory[index].KB.read_write = 1;
        page_directory[index].KB.user_supervisor = 0;
        page_directory[index].KB.write_through = 0;
        page_directory[index].KB.cache_disabled = 0;
        page_directory[index].KB.accessed = 0;
        page_directory[index].KB.reserved = 0;
        page_directory[index].KB.page_size = 0;
        page_directory[index].KB.global_page = 0;
        page_directory[index].KB.availablr_for_user = 0;
        page_directory[index].KB.PB_addr = (int) page_table >> OFFSET_12;        //pointer to the PB   
}

 /* SET_PDE_PARAMS
 *      DESCIRPTION: set one PDE with given value in KB form(point to a page table)
 *      INPUT:  index: index in the PDE, 
 *              present: whther this entry is present in pythical memory.  
 *      OUTPUT: PTE set at PD[index] 
 *      RETURN: none
 */
void SET_PDEMAP_KB_PARAMS(int index, int present){
        // change the value accordingly
        page_directory[index].KB.present = present;
        page_directory[index].KB.read_write = 1;
        page_directory[index].KB.user_supervisor = 1;
        page_directory[index].KB.write_through = 0;
        page_directory[index].KB.cache_disabled = 0;
        page_directory[index].KB.accessed = 0;
        page_directory[index].KB.reserved = 0;
        page_directory[index].KB.page_size = 0;
        page_directory[index].KB.global_page = 0;
        page_directory[index].KB.availablr_for_user = 0;
        page_directory[index].KB.PB_addr = (int) page_table_usermap >> OFFSET_12;        //pointer to the PB   
}


 /* SET_PDE_PARAMS
 *      DESCIRPTION: set one PDE with given value in MB form
 *      INPUT:  index: index in the PDE, 
 *              present: whther this entry is present in pythical memory.  
 *      OUTPUT: PTE set at PD[index] 
 *      RETURN: none
 */

void SET_PDE_MB_PARAMS(int index, int present){
        //change the value accordingly
        page_directory[index].MB.present = present;
        page_directory[index].MB.read_write = present;
        page_directory[index].MB.user_supervisor = 0;
        page_directory[index].MB.write_through = 0;
        page_directory[index].MB.cache_disabled = 0;
        page_directory[index].MB.accessed = 0;
        page_directory[index].MB.dirty = 0;
        page_directory[index].MB.page_size = 1;
        page_directory[index].MB.global_page = 1;
        page_directory[index].MB.availablr_for_user = 0;
        page_directory[index].MB.page_table_aindex = 0;
        page_directory[index].MB.reserved = 0;
        page_directory[index].MB.PB_addr = 1;
}




/* SET_PTE_PARAMS
 *      DESCIRPTION:   set one PTE with given value
 *      INPUT:  index: index in the PDE, 
 *              present: whther this entry is present in pythical memory.  
 *      OUTPUT: PTE set at PT[index] 
 *      RETURN: none
 */
void SET_PTE_PARAMS(int index, int present){
    //change the value accordingly
    page_table[index].present = present;
    page_table[index].read_write = 1;
    page_table[index].user_supervisor = 0;
    page_table[index].write_through = 0;
    page_table[index].cache_disabled = 0;
    page_table[index].accessed = 0;
    page_table[index].dirty = 0;
    page_table[index].page_table_aindex = 0;
    page_table[index].global_page = present;
    page_table[index].availablr_for_user = 0;
    page_table[index].PB_addr = index;        // bug log do not need to <<12
}

/* SET_USER_VIDEO_MAP
 *      DESCIRPTION:   set one PTE with given value
 *      INPUT:  index: index in the PDE, 
 *              present: whther this entry is present in pythical memory.  
 *      OUTPUT: PTE set at PT[index] 
 *      RETURN: none
 */
void SET_USER_VIDEO_MAP(int index, int present){
    //change the value accordingly
    page_table_usermap[VIDEO>>OFFSET_12].present = present;
    page_table_usermap[VIDEO>>OFFSET_12].read_write = 1;
    page_table_usermap[VIDEO>>OFFSET_12].user_supervisor = 1;
    page_table_usermap[VIDEO>>OFFSET_12].write_through = 0;
    page_table_usermap[VIDEO>>OFFSET_12].cache_disabled = 0;
    page_table_usermap[VIDEO>>OFFSET_12].accessed = 0;
    page_table_usermap[VIDEO>>OFFSET_12].dirty = 0;
    page_table_usermap[VIDEO>>OFFSET_12].page_table_aindex = 0;
    page_table_usermap[VIDEO>>OFFSET_12].global_page = present;
    page_table_usermap[VIDEO>>OFFSET_12].availablr_for_user = 0;
    page_table_usermap[VIDEO>>OFFSET_12].PB_addr = page_table[index].PB_addr;      
}





/*  page_on
 *  dispcription: set cr3 to the address of the table, set the cr0 and 2 to enable paging
 *  INPUT: noun
 *  OUTPUT: the cr0 and cr4 changed to get page on and cr3 contain the ptr
 *  SIDEEFFECT: EAX get changed 
 *  RETURN: noun
*/
void page_on(){
    asm volatile(
	"movl %0, %%eax             ;"
	"movl %%eax, %%cr3          ;"

	"movl %%cr4, %%eax          ;"
	"orl $0x00000010, %%eax     ;"
	"movl %%eax, %%cr4          ;"

	"movl %%cr0, %%eax          ;"
	"orl $0x80000000, %%eax     ;"
	"movl %%eax, %%cr0          ;"

	:  : "r"(page_directory): "eax" );
}

