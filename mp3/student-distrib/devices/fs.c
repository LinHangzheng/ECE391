/* idt.c - functions used in file system */
/* Created by Chenting on 10/22/2019 */

#include "fs.h"
#include "../sys_call/do_sys_call.h"
 
/* return values -- 0 for success and -1 for fail */
#define FAIL    -1
#define SUCCESS 0

/* given start ptr and global data structrue*/
void*   fs_start_addr;
boot_block_t* boot_block;
inode_t*    inode_start_addr;
data_block_t* data_block_start_addr;


// int32_t fileIsOpen;

// int32_t dirIsOpen;

/*
 *  int32_t fs_init(uint32_t fs_start_ptr)
 *  DESCRIPTION: ready-only file system initialization
 *               
 *  INPUTS: uint32_t fs_start_ptr, 
 *          a ptr to the start fo the given file system
 *  OUTPUTS: set uo the data structrue for the file sys for our later use
 *  RETURN VALUE: should always return 0 for SUCCESS
 */
int32_t fs_init(uint32_t fs_start_ptr){

    /* setup file system start addr */
    fs_start_addr = (void*) fs_start_ptr;

    /* setup file system blocks start addr */
    boot_block = (boot_block_t*) fs_start_ptr;
    inode_start_addr = (inode_t*) (boot_block + 1);
    data_block_start_addr = (data_block_t*) (inode_start_addr + boot_block->num_inodes);

    /* return 0 for successful init */
    return SUCCESS;
}


/*
 *  int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
 *  DESCRIPTION: search dentry by name 
 *               
 *  INPUTS: uint8_t* fname, the file name we want to read
 *          dentry_t* dentry, the ptr we should give the content to
 *  OUTPUTS: copy the dentry structrue to the given ptr
 *  RETURN VALUE: return 0 for SUCCESS and -1 for failure
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry){

    int i;  /* loop index */
    int j;  /* loop index */

    if (strlen((int8_t*)fname) > MAX_FILENAME_LEN){
        return FAIL;
    }

    /* buffer for compare when len is 32 */
    char new_buf1[MAX_FILENAME_LEN+1];
    char new_buf2[MAX_FILENAME_LEN+1];
    /* traverse all dentries */
    for( i = 0; i < boot_block->num_dentries; i++){

        /* if length not equal */
        uint32_t filename_len = strlen((int8_t*) boot_block->dentry_array[i].filename);
        /* if the length is 32, we add a \0 at the 33th place for strcmp works correctly */
        if(filename_len > MAX_FILENAME_LEN){
            for(j = 0; j < MAX_FILENAME_LEN; j++){
                new_buf1[j] = boot_block->dentry_array[i].filename[j];
                new_buf2[j] = fname[j];
            }
            new_buf1[MAX_FILENAME_LEN] = '\0';
            new_buf2[MAX_FILENAME_LEN] = '\0';

            filename_len = MAX_FILENAME_LEN;
            if ( !(strncmp( new_buf1,new_buf2, filename_len))){
                /* once requested dentry was found */
                
                *dentry = boot_block->dentry_array[i];
                return SUCCESS;
            }else{
                continue;
            }
        }

        /* else, just cmp */
        if (strlen((char*)fname) != filename_len ) continue;
        // printf("\n cmpstring: %s", boot_block->dentry_array[i].filename);
        /* compare two file names */
        if ( !(strncmp( boot_block->dentry_array[i].filename,(char*) fname, filename_len) )){
            /* once requested dentry was found */
            
            *dentry = boot_block->dentry_array[i];
            return SUCCESS;
        }
    }

    /* requested dentry not found */
    return FAIL;
}


/*
 *  int32_t read_dentry_by_name(uint32_t index, dentry_t* dentry)
 *  DESCRIPTION: search dentry by index
 *               
 *  INPUTS: uint8_t* index, the inode index we want to find
 *          dentry_t* dentry, the ptr we should give the content to
 *  OUTPUTS: copy the dentry structrue to the given ptr
 *  RETURN VALUE: return 0 for SUCCESS and -1 for failure
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry){

    // int i;  /* loop index */

    /* sanity check */
    if (index >= boot_block->num_dentries){
        return FAIL;
    }

    /* get required dentry and return success */
    *dentry = boot_block->dentry_array[index];
    return SUCCESS;
}


/*
 *  int32_t read_data(uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t length)
 *  DESCRIPTION: read data based on the given inode file and offset for the length num of byte
 *               
 *  INPUTS: uint8_t* index, the inode index we want to find
 *          uint32_t offset, we read the file starting from the offest of the file
 *          dentry_t* buf, the ptr we should give the content to
 *          uint32_t length, we read length num of byte in the file 
 *  OUTPUTS: copy the file content to the given ptr
 *  RETURN VALUE: read_count - number of bytes read in buffer
 *                or 0 for end of file reached 
 *                or -1 for failure
 */
int32_t read_data (uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t length){
    
    int i,j;  /* loop index */
    int read_count = 0;         /* count # of chars been read */


    /* determine the corresponding file by given inode index */
    inode_t* inode = (inode_t*) & inode_start_addr[inode_idx];

    /* sanity check for index */
    if(inode_idx >= boot_block->num_inodes || inode == NULL){
        return FAIL;
    }


    /* check whether end of file reached  */
    if(offset >=  inode->file_size){
        return 0;
    }


    uint32_t end_pos = offset + length; 
    /* sanity check for size */
    if( offset + length > inode->file_size){
        end_pos = inode->file_size ;
    }
    

    /* determine where reading begin and where reading will finish */
    int start_data_block_idx = offset / BLOCK_SIZE_IN_BYTE;
    int end_data_block_idx = (end_pos-1) / BLOCK_SIZE_IN_BYTE;

    /* read in the start data block */
    uint32_t start_data_block_num = inode->data_blocks[start_data_block_idx];
    data_block_t* start_data_block = (data_block_t*) data_block_start_addr + start_data_block_num;
    // print_data_block(*start_data_block);
    for(j = offset % BLOCK_SIZE_IN_BYTE; j < BLOCK_SIZE_IN_BYTE; j++){
        buf[read_count] = start_data_block->datas[j];
        read_count++;   
        /* finish reading once length satified */
        if(read_count == length ||  read_count == (inode->file_size - offset)){
            /* this case we read only one data block */
            /* return number of bytes read in buffer for success */
            return read_count;
        }
    }


    /* read the medium data blocks */
    for(i = start_data_block_idx + 1; i < end_data_block_idx; i++){
        uint32_t curr_data_block_num = inode->data_blocks[i];
        data_block_t* curr_data_block = (data_block_t*) data_block_start_addr + curr_data_block_num;
        for(j = 0; j < BLOCK_SIZE_IN_BYTE; j++){
            buf[read_count] = curr_data_block->datas[j];
            read_count++;
        }
    }

    /* read in the last data block */
    uint32_t end_data_block_num = inode->data_blocks[end_data_block_idx];
    data_block_t* end_data_block = (data_block_t*) data_block_start_addr + end_data_block_num;
    for(j = 0; j < BLOCK_SIZE_IN_BYTE; j++){
        buf[read_count] = end_data_block->datas[j];
        read_count++;
        /* finish reading once length satified */
        if(read_count == length || read_count == (inode->file_size - offset) ) break;
    }

    /* return number of bytes read in buffer for success */
    return read_count;

}


/*
 *  int32_t file_open (const uint8_t* filename)
 *  DESCRIPTION: open the data file in the directory (not implemented yet)
 * 
 *  INPUTS: uint8_t* filename, the file we want to open
 *  OUTPUTS: change the global para and set the cur dentry para
 *  RETURN VALUE: return 0 for SUCCESS and -1 for failure
 */
int32_t file_open (const uint8_t* filename){
    /* current opened file waited to be read */
    dentry_t currFileDentry;
    /* call read_dentry_by_name to open given file */
    if(read_dentry_by_name(filename, &currFileDentry) || currFileDentry.file_type != 2){
        printf("type:%d",  currFileDentry.file_type );
        /* return -1 if read filename fails or not regular file */
        return FAIL;
    }

    /* set flag to 1 for successful open */
    // fileIsOpen = 1;
    return SUCCESS;
}


/*
 *  int32_t file_close (int32_t fd)
 *  DESCRIPTION: close the data file in the directory (not implemented yet)
 * 
 *  INPUTS: int32_t fd, the file we want to close
 *  OUTPUTS: change the global para 
 *  RETURN VALUE: return 0 for SUCCESS
 */
int32_t file_close (int32_t fd){
    /* close file by setting curr dentry to none */
    // fileIsOpen = 0;
    return SUCCESS;
}

/*
 *  int32_t file_read (int32_t fd, void* buf, int32_t nbytes)
 *  DESCRIPTION: call read data
 *  INPUTS: 
 *          int32_t fd, file direc to read
 *          dentry_t* buf, the ptr we should give the content to
 *          int32_t nbytes, we read length num of byte in the file *  OUTPUTS: change the global para 
 *  RETURN VALUE: return number of bytes read in buffer
 *                or 0 for end of file reached 
 *                or -1 for failure
 */
int32_t file_read (int32_t fd, uint32_t offset, void* buf, int32_t nbytes){
    /* check whther file opened */
    // if(!fileIsOpen) {
    //     printf("not open!!!");    
    //     return FAIL;
    // }
    int32_t ret;
    ret = read_data(get_file_array()[fd].inode, offset, buf, nbytes);
    if (ret == FAIL){return FAIL;}
    get_file_array()[fd].file_position += ret;
    return ret;
}

/*
 *  int32_t file_write (int32_t fd, const void* buf, int32_t nbytes)
 *  DESCRIPTION: nothing
 *  INPUTS: ignored
 *  RETURN VALUE: return SUCCESS -1 for fail (read only)
 */
int32_t file_write (int32_t fd, const void* buf, int32_t nbytes){
    // do nothing 
    return FAIL;
}

/*
 *  int32_t dir_open (const uint8_t* filename)
 *  DESCRIPTION: open the data dir in the directory (not implemented yet)
 * 
 *  INPUTS: uint8_t* filename, the file we want to open
 *  OUTPUTS: change the global para and set the cur dentry para
 *  RETURN VALUE: return 0 for SUCCESS and -1 for failure
 */

int32_t dir_open (const uint8_t* filename){
    /* current opened directory waited to be read */
    dentry_t currDirDentry;
    /* call read_dentry_by_name to open given directory */
    if(read_dentry_by_name(filename, &currDirDentry) || currDirDentry.file_type != 1){
        /* return -1 if read filename fails or not directory */
        return FAIL;
    }

    /* set flag to 1 for successful open */
    // dirIsOpen = 1;
    return SUCCESS;

}

/*
 *  int32_t dir_close (int32_t fd)
 *  DESCRIPTION: close the data file in the directory (not implemented yet)
 * 
 *  INPUTS: int32_t fd, the file we want to close
 *  OUTPUTS: change the global para 
 *  RETURN VALUE: return 0 for SUCCESS
 */

int32_t dir_close (int32_t fd){
    /* close file by setting curr dentry to none */
    // dirIsOpen = 0;
    return SUCCESS;
}



/*
 *  int32_t dir_read (int32_t fd, void* buf, int32_t nbytes)
 *  DESCRIPTION: read_dentry_by_index
 *  INPUTS: 
 *          int32_t fd, file direc to read
 *          dentry_t* buf, the ptr we should give the content to
 *          int32_t nbytes, we read length num of byte in the file 
 *  RETURN VALUE: return 0 for SUCCESS -1 for fail
 *  OUTPUT: fill the buffer with content
 */

int32_t dir_read (int32_t fd, uint32_t offset, void* input_buf, int32_t nbytes){

    /* change buf type to char* */
    char buf_33[MAX_FILENAME_LEN+1];
    char* buf = input_buf;
    int i;
    
    dentry_t currDentry;
    /* return -1 id read file fails */
    if(read_dentry_by_index(offset, &currDentry) == FAIL) return 0;
    for (i=0;i<MAX_FILENAME_LEN;i++){
            buf_33[i] = currDentry.filename[i];
        }buf_33[MAX_FILENAME_LEN] = '\0';
    /* copy filename into buffer and then update read count */
    uint32_t filename_len = strlen((int8_t*)(buf_33));
    /* check for boundary */
    if( filename_len  > nbytes){
        return 0;
    }

    /* copy string into buffer */
    strcpy( &buf[0], (int8_t*)(buf_33));

    /* check for long filename */
    if(filename_len > MAX_FILENAME_LEN){
        filename_len = MAX_FILENAME_LEN;
    }

    /* update read count and start new line */
    buf[filename_len] = '\0';
    
    /* update the file_position for success */
    get_file_array()[fd].file_position += 1;

    /* return 0 for success */
    return filename_len;
}

/*
 *  int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes)
 *  DESCRIPTION: nothing
 *  INPUTS: ignored
 *  RETURN VALUE: return SUCCESS -1 for fail (read only)
 */
int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes){
    // do nothing
    return FAIL;
}


/*
 *   print_dentry(dentry_t dentry)
 *   DESCRIPTION: helper in test, just print the content in
 *                  the input dentry
 *   INPUTS: the dentry we want to print
 *   RETURN VALUE: NOTHING
 */
void print_dentry(dentry_t dentry){
    int i;
    printf("=============DENTRY==============\n");
    printf("file_type: %d\n", dentry.file_type);
    printf("filename: ");
    for(i=0; i < MAX_FILENAME_LEN; i++){
        putc(dentry.filename[i]);
    }
    printf("\n");
    printf("inode num: %d\n", dentry.inode_idx);
    printf("================================\n");
}


/*
 *   boot_block_t get_boot_block()
 *   DESCRIPTION: helper in test, return boot_block
 *   INPUTS: none
 *   RETURN VALUE: boot_block
 */
boot_block_t get_boot_block(void){
    return *boot_block;
}

/*
 *   print_data_block(data_block_t data_block)
 *   DESCRIPTION: helper in test, just print the content in
 *                  the input datablock
 *   INPUTS: the datablock we want to print
 *   RETURN VALUE: NOTHING
 */
void print_data_block(data_block_t data_block){
    int i;
    for(i = 0; i < BLOCK_SIZE_IN_BYTE; i++){
        putc(data_block.datas[i]);
    }
}

/* get_file_size
 * 
 *  DESCRIPTION: helper function used to get size of corresponding inode
 *  INPUTS: none
 *  OUTPUT: none
 *  RETURN VALUE: the size of corresponding file
 * 
 */
uint32_t get_file_size(uint32_t inode_num){

    /* get current the inode structure */
    inode_t* curr_inode = inode_start_addr + inode_num;

    /* return file size */
    return curr_inode->file_size;

}




