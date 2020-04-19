#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "devices/RTC.h"
#include "devices/keyboard.h"
#include "terminal.h"
#include "devices/fs.h"
#include "process.h"
#include "devices/pit.h"

#define PASS 1
#define FAIL 0
#define MAX_FILENAME_LEN 32
#define MAX_FILE_SIZE_INTEST 4096
#define MAX_TOTAL_FILE_NAME 300
#define BUF_SIZE 128

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test 
 * 
 * Asserts that first 20 IDT entries (exception handlers) hold parameters in correct way
 * and also twe divices interrupt handlers.
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition and Initialization
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;
	int i;
	int result = PASS;

	/* Assert the first 20 IDT entries (exception handlers) */
	for (i = 0; i < 20; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
		if (idt[i].present == 0){
			assertion_failure();
			result = FAIL;
		}
	}

	/* Assert the two divices interrupt handlers (keyboard and RTC) */
	if (((idt[0x21].offset_15_00 == NULL) && (idt[0x21].offset_31_16 == NULL))
		|| idt[0x21].present == 0){
		assertion_failure();
		result = FAIL;
	}
	if (((idt[0x28].offset_15_00 == NULL) && (idt[0x28].offset_31_16 == NULL))
		|| idt[0x28].present == 0){
		assertion_failure();
		result = FAIL;
	}

	return result;
}

/* Page Test
 * 
 * Check the entries of the table
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load cr3, Page Initialization
 * Files: page.h/c, x86_desc.h/S
 */
int page_test(){
	TEST_HEADER;
	int i;
	int result = PASS;

	/* test margin of page -- should pass without page fault */
	int val;
	int* ptr = (int*)0x400000;	/* the margin of PD */
	ptr[0] = 1;
	val = ptr[0];

	/* check the value of the entry of the tables */
	// first, check whether the first two exist 
	if (page_directory[0].KB.present == 0 || page_directory[1].MB.present == 0 ){
		printf ("PDE 1 OR 0 NOT PRESENT\n");
		result = FAIL;
	}

	// then, check whether the other not present and present at the video memory
	for(i = 0; i < 1024; i++){				// 1024 is the entry number
		if(page_directory[i].MB.present == 1 && i > 1){
			printf ("PDE wrongly PRESENT\n");
			result = FAIL;
		}
		if(page_table[i].present == 1 && i != 0xB8000 >> 12 ){		//  0xB8000 >> 12 is ithe index where the page table should present
			printf ("PTE wrongly PRESENT\n");
			result = FAIL;
		}
		if (page_table[i].present == 0 && i == 0xB8000 >> 12){ 		// 0xB8000 >> 12 is ithe index where the page table should present
			printf ("PTE no PRESENT\n");
			result = FAIL;
		} 
	}

	return result;
}


/* test_divide_error
 * 
 * test exception of divide error.  
 * Inputs: None
 * Outputs: This test should give a blue screen of "divide error"
 * Side Effects: None
 * Coverage: exception handler of divide error
 * Files: idt.h/c
 */
void test_divide_error(){
	/* values used to test */
	int test_zero = 0;
	int test_num = 42;	
	int test_ret;
	test_ret = test_num/test_zero;	// this should give divide error
}

/* test_page_fault
 * 
 * test exception of page fault.  
 * Inputs: None
 * Outputs: This test should give a blue screen of "page fault"
 * Side Effects: None
 * Coverage: exception handler of  page fault
 * Files: idt.h/c
 */
void test_page_fault(){
	int* ptr = (int*) 0x00;
	int val;
	val = ptr[0];	// this should give page fault
}

/* test_excp_reserved
 * 
 * test reserved exception.  
 * Inputs: None
 * Outputs: This test should give a blue screen of reserved exception
 * Side Effects: None
 * Coverage: reserved exception handler
 * Files: idt.h/c
 */
void test_excp_reserved(){
	asm("int $0x01");	// this should give a blue screen 
}

/* test_one_excp
 * 
 * test any exception wanted  
 * Inputs: None
 * Outputs: This test should give a blue screen of corresponding exception
 * Side Effects: None
 * Coverage: any exception handler
 * Files: idt.h/c
 */
void test_one_excp(){
	/* change the following vector between 0-19(0x00-0x13) */ 
	/* to test each corresponding exception handlers */
	asm("int $0x01");
}

/* test_i8259_disable_irq_garbage
 * Inputs: None
 * Outputs: This test should don't mask any interrupts when a garbage input is given to disable_irq 
 * Side Effects: None
 * Coverage: garbage input to disable_irq
 * Files: i8259.c
 */
void test_i8259_disable_irq_garbage(){
	/*send a invalid irq_num to disable_irq, nothing should happen*/
	/*The interrupt from keyboard and rtc should still be aceepted*/
	disable_irq(16);
	disable_irq(1000);
}


/* test_i8259_disable_irq
 * Inputs: None
 * Outputs: This test should mask interrupts from keyboard and rtc
 * Side Effects: None
 * Coverage: disable_irq
 * Files: i8259.c
 */
void test_i8259_disable_irq(){
	/*send irq_num of keyboard and rtc and check whether it is masked*/
	disable_irq(KEYBOARD_IRQ_NUM);
	disable_irq(RTC_PIC_IDX);
}

/* test_i8259_enable_irq_garbage
 * Inputs: None
 * Outputs: This test should don't enable any interrupts when a garbage input is given to enable_irq 
 * Side Effects: None
 * Coverage: garbage input to enable_irq
 * Files: i8259.c
 */
void test_i8259_enable_irq_garbage(){
	/*send garbage input to enable_irq nothing should happend*/
	enable_irq(16);
	enable_irq(1000);
}


/* test_i8259_enable_irq
 * Inputs: None
 * Outputs: This test should enable interrupts from keyboard and rtc
 * Side Effects: None
 * Coverage: enable_irq
 * Files: i8259.c
 */
void test_i8259_enable_irq(){
	/*it should enable the interrupt from the keyboard and rtc after disable_irq is called*/
	enable_irq(KEYBOARD_IRQ_NUM);
	enable_irq(RTC_PIC_IDX);
}



/* Checkpoint 2 tests */

/* test_begin
 * Inputs: None
 * Outputs: helper called by lauch_test, type user and ece391 to begin
 * Side Effects: None
 * Coverage: noun
 * Files: noue
 */
void test_begin(){
	char buf[BUF_SIZE];
	int32_t fd = NULL;

	/*just for fun, write user and ece391 to begin test*/
	terminal_open(0);
	while (1)
	{
		printf("localhost login: ");
		terminal_read(fd, 0, buf,BUF_SIZE);
		if (!strncmp("user",buf,BUF_SIZE)){
			break;
		}
	}

	while (1)
	{
		printf("password: ");
		terminal_read(fd, 0, buf,BUF_SIZE);
		if (!strncmp("ece391",buf,BUF_SIZE)){
			break;
		}
	}
	terminal_close(0);
}

/* test_end
 * Inputs: None
 * Outputs: helper called by every test, put exit message on screen 
 * 			to get a good style
 * Side Effects: None
 * Coverage: noun
 * Files: noue
 */
void test_end(){
	char buf[BUF_SIZE];
	terminal_open(0);
	while(1){
		printf("\nPress 'q' to exist: ");
		terminal_read(0, 0, buf,BUF_SIZE);
		if (!strncmp(buf,"q",BUF_SIZE)){
			clear();
			break;
		}
	}
}

/* test_RTC_open_close
 * Inputs: None
 * Outputs: Test for RTC open and close, rtc open will set the freq to 2 and close will simply return 0
 * Side Effects: None
 * Coverage: RTC_open and close
 * Files: RTC.c
 */
void test_RTC_open_close(){
	int check_close = 1;
	int count = 0;
	uint32_t rtc_buf;
	TEST_HEADER;
	RTC_open(0);
	/*Print ten '1' to the screen*/
	while (count < 10){
		RTC_read(0, 0, &rtc_buf,4);
		putc('1');
		count++;
	}
	/*check whether the RTC_close will return 0*/
	check_close = RTC_close(0);
	if (!check_close){
		printf("\nRTC_close returns 0 successfully");
	}
	test_end();
}


/* test_RTC_read_write
 * Inputs: None
 * Outputs: Test for RTC read and write and see how frequency will be changed
 * Side Effects: None
 * Coverage: RTC_read and write
 * Files: RTC.c
 */
void test_RTC_read_write(){
	uint32_t rtc_buf;
	int digits = 0;
	int count = 0;
	char cmd_buf[BUF_SIZE];
	int check;

	/*Test for RTC_write and read, change frequency*/
	TEST_HEADER;
	terminal_open(0);
	while (1)
	{
		printf("Set RTC frequency(Press 'q' to exit): ");
		/*Read in the user input, user types in a frequency*/
		digits = terminal_read(0, 0, cmd_buf,10);
		/*press q to exist*/
		if (cmd_buf[0] == 'q'){break;}
		/*Transfer the type in ascill code of number to the actual number*/
		switch (digits)
		{
		case 4:
			rtc_buf = (uint32_t)(cmd_buf[0]-0x30)*1000 + (uint32_t)(cmd_buf[1]-0x30)*100 + (uint32_t)(cmd_buf[2]-0x30)*10 + (uint32_t)(cmd_buf[3]-0x30);
			break;
		case 3:
			rtc_buf =  (uint32_t)(cmd_buf[0]-0x30)*100 + (uint32_t)(cmd_buf[1]-0x30)*10 + (uint32_t)(cmd_buf[2]-0x30);
			break;
		case 2:
			rtc_buf = (uint32_t)(cmd_buf[0]-0x30)*10 + (uint32_t)(cmd_buf[1]-0x30);
			break;
		case 1:
			rtc_buf = (uint32_t)(cmd_buf[0]-0x30);
			break;
		default:
			break;
		}
		RTC_open(0);
		check = RTC_write(0,&rtc_buf,4);
		/*If user gives a invalid input,the RTC_write will return FAIL, and let user type again*/
		if (check == -1){
			printf("This frequency is not allowed\n");
			RTC_close(0);
			continue;
		}
		/*Print 250 '1' in different frequency*/
		count = 0;
		while (count < 250)
		{
			RTC_read(0, 0, &rtc_buf,4);
			putc('1');
			count++;
		}
		RTC_close(0);
		clear();
	}
	terminal_close(0);
	clear();
}


/* test_terminal_read_write
 * Inputs: None
 * Outputs: Read in the keyboard inputs to a buffer and write out the buffer to the screen
 * Side Effects: None
 * Coverage: terminal_read and write, handle buffer overflow
 * Files: terminal.c, keyboard.c
 */
void test_terminal_read_write(){
	char buf[BUF_SIZE];
	int read_num = 0;
	int write_num = 0;
	int32_t fd = NULL;
	
	/*Test for terminal read and write*/
	TEST_HEADER;
	terminal_open(0);
	while (1)
	{
		printf("What's your name?(press 'q' to exist) ");
		/*read in the user input to a buffer*/
		read_num = terminal_read(fd,0,  buf, BUF_SIZE);		
		/*Press q to quit*/		
		if (!strncmp("q",buf,BUF_SIZE)){
			break;
		}
		printf("Hello,");
		/*write out the buffer value to the screen*/
		write_num = terminal_write(0,buf,BUF_SIZE);
		/*compare the read in bytes and write out bytes*/
		printf("\nread in bytes: %d\nwrite out bytes: %d\n",read_num, write_num);
	}
	terminal_close(0);
	clear();
}


/* test_keyboard_handler
 * Inputs: None
 * Outputs: Test for unknown scancode, ctrl + l and scrolling
 * Side Effects: None
 * Coverage: unknown scancode, crtl+l, scrolling
 * Files: terminal.c, keyboard.c
 */
void test_keyboard_handler(){
	int i;
	char buf[BUF_SIZE];
	TEST_HEADER;
	printf("Try for unknown scancodes and ctrl + l, press q to exist\n");
	/*wait until user press q*/
	terminal_open(0);
	while(1){
		terminal_read(0, 0, buf,BUF_SIZE);
		if (!strncmp(buf,"q",BUF_SIZE)){
			clear();
			break;
		}
	}
	terminal_close(0);
	printf("Test for scrolling,press q to exist\n");
	/*fill the screen with 1*/
	for (i = 0;i < 1800;i++){
		putc('1');
	}

	/*wait until user press q*/
	while(1){
		terminal_read(0, 0, buf,BUF_SIZE);
		if (!strncmp(buf,"q",BUF_SIZE)){
			clear();
			break;
		}
	}
}

/* test_terminal
 * Inputs: None
 * Outputs: Test for terminal driver, it will call other terminal test function
 * Side Effects: None
 * Coverage: Anything about terminal driver
 * Files: terminal.c, keyboard.c
 */
void test_terminal(){
	test_keyboard_handler();
	test_terminal_read_write();
}



/* test_RTC
 * Inputs: None
 * Outputs: Test for rtc driver, it will call other rtc test function
 * Side Effects: None
 * Coverage: Anything about rtc driver
 * Files: rtc.c
 */
void test_RTC(){
	test_RTC_open_close();
	test_RTC_read_write();
}


/* print the list of all file name */

/* test_file_name
 * Inputs: None
 * Outputs: This is a test that print all the file names
 * Side Effects: None
 * Coverage: terminal read and write
 * Files: terminal.c/h
 */
void test_file_name(){

	clear();
	TEST_HEADER;
	int result;
	int i; 		// counter
	result = dir_open((uint8_t*)".");
	// printf("%d", result);
	char buf_test_fs[MAX_TOTAL_FILE_NAME];
	char cur_file_name[MAX_FILENAME_LEN+1]; 
	dir_read(0, 0, buf_test_fs, MAX_TOTAL_FILE_NAME);
	dir_close(0);

	/* we get all the name into a single buffer in the loop */
	int cur_read_offset = 0;
	int last_file_offset = 0;
	dentry_t cur_dentry;
	char cur_buf[MAX_FILE_SIZE_INTEST];
	/* for every file we ge the name with dentry_read for '.' */
	while(buf_test_fs[cur_read_offset] != '\0'){
		cur_file_name[cur_read_offset - last_file_offset] = buf_test_fs[cur_read_offset];
		
		if(buf_test_fs[cur_read_offset] == '\n'){
			cur_file_name[cur_read_offset - last_file_offset] = '\0';
			last_file_offset = cur_read_offset + 1;
			/*now we have a file name in cur_file_name*/
			read_dentry_by_name((uint8_t*)cur_file_name, &cur_dentry);
			cur_buf[MAX_FILE_SIZE_INTEST-1] = '\0';
			RTC_open((uint8_t*)cur_file_name);
			RTC_read(0, 0, NULL,MAX_FILE_SIZE_INTEST);
			RTC_close(0);
			// clear();
			printf("file_type: %d       ", cur_dentry.file_type);
			printf("filename: ");
			for(i=0; i < MAX_FILENAME_LEN; i++){
				putc(cur_dentry.filename[i]);
			}
			printf("  ");
			printf("inode num: %d\n", cur_dentry.inode_idx);
		}
		cur_read_offset++;	
	}

	cur_file_name[cur_read_offset - last_file_offset] = '\0';
	last_file_offset = cur_read_offset + 1;
	/*now we have a file name in cur_file_name*/
	read_dentry_by_name((uint8_t*)cur_file_name, &cur_dentry);
	cur_buf[MAX_FILE_SIZE_INTEST-1] = '\0';
	RTC_open((uint8_t*)cur_file_name);
	RTC_read(0, 0, NULL, MAX_FILE_SIZE_INTEST);
	RTC_close(0);
	// clear();
	printf("file_type: %d       ", cur_dentry.file_type);
	printf("filename: ");
	for(i=0; i < MAX_FILENAME_LEN; i++){
		putc(cur_dentry.filename[i]);
	}
	printf("  ");
	printf("inode num: %d\n", cur_dentry.inode_idx);
	test_end();
}


/* print_types
 * Inputs: None
 * Outputs: test helper called by other test. Print out the file of a given dentry 
 * Side Effects: None
 * Coverage: helper
 */

void print_types(const dentry_t cur_dentry){
	
	// temp var to store info
	int32_t read_length;
	char cur_buf[MAX_FILE_SIZE_INTEST*3];
	int i; //counter
	// boot_block_t boot_block;
	printf("==============CONTENT=============\n");
	// open read close print according to type 
	cur_buf[0] = '\0';
	switch (cur_dentry.file_type){
	case 0:
		// open read close print for rtc 
		RTC_open((uint8_t*)(cur_dentry.filename));
		if(RTC_read(0, 0, cur_buf,MAX_FILE_SIZE_INTEST)) printf("fail!!, %s",cur_dentry.filename);
		printf("%s\n", cur_buf);
		RTC_close(0);
		break;
	case 1: 
		// open read close print for dir 
		dir_open((uint8_t*)(cur_dentry.filename));
		if(dir_read(0, 0, cur_buf,MAX_FILE_SIZE_INTEST)) printf("fail!!, %s",cur_dentry.filename);
		printf("%s\n", cur_buf);
		dir_close(0);
		break;
	case 2:
		// open read close print for file 
		file_open((uint8_t*)(cur_dentry.filename));
		if(-1 == (read_length = file_read(0, 0, cur_buf,MAX_FILE_SIZE_INTEST*3))) printf("fail!!, %s",cur_dentry.filename);
		for(i = 0; i < read_length; i++){
			putc(cur_buf[i]);
		}
		file_close(0);
		break;
	default:
		printf("unknown file type!!");
		break;
	}

	printf("\n================================\n");
	
}

/* test_file_content
 * Inputs: None
 * Outputs: print all the file's content on the terminal
 * Side Effects: noun
 * Coverage: file open,read,write,close
 */

void test_file_content(){
	TEST_HEADER;
	int result;
	result = dir_open((uint8_t*)".");
	// printf("%d", result);
	char buf_test_fs[MAX_TOTAL_FILE_NAME];
	dir_read(0, 0, buf_test_fs, MAX_TOTAL_FILE_NAME);
	dir_close(0);


	dentry_t cur_dentry;

	char buf[BUF_SIZE];

	/*just for fun, write user and ece391 to begin test*/
	terminal_open(0);
	while(1)
	{
		printf("\nYour file(Press 'q' to exit): ");
		terminal_read(0, 0, buf,BUF_SIZE);
		/* Exit the loop*/
		if (!strncmp(buf,"q",BUF_SIZE)){
			clear();
			break;
		}
		if (read_dentry_by_name((uint8_t*)buf,&cur_dentry)){
			printf("file doesn't exit.\n");
		}else{	
			/* Print the current file*/
			clear();
			print_types(cur_dentry);
			print_dentry(cur_dentry);
		}	
	}
	
}


/* test_file_content_index
 * Inputs: None
 * Outputs: print all the file's content on the terminal
 * 			according to the index in the boot block
 * Side Effects: noun
 * Coverage: file open,read,write,close
 */

void test_file_content_index(){
	dentry_t cur_dentry;
	boot_block_t boot_block = get_boot_block();
	int i;
	printf("total index: %d \n", boot_block.num_dentries);
	for (i = 0; i < boot_block.num_dentries; i++){
		/* for evey ind, we first wait for a little while */
		RTC_open(NULL);
		RTC_read(0,NULL,0, MAX_FILE_SIZE_INTEST);
		RTC_close(0);
		clear();
		/* then clear and print */
		read_dentry_by_index(i,&cur_dentry);
		print_types(cur_dentry);
		print_dentry(cur_dentry);
		printf("current index: %d\n", i);

	}
	test_end();
}

/* test_long_filename
 * Inputs: None
 * Outputs: print the very long file content
 * Side Effects: noun
 * Coverage: file open,read,write,close
 */

void test_long_filename(){
	dentry_t cur_dentry;
	read_dentry_by_name((uint8_t*)"verylargetextwithverylongname.tx", &cur_dentry);
	print_types(cur_dentry);
	print_dentry(cur_dentry);
	test_end();
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// launch your tests here
	// clear();
	// printf("---------------------------TEST_START-----------------------------\n");
	// TEST_OUTPUT("idt_test", idt_test());
	// TEST_OUTPUT("page_test", page_test());	
	/*For RTC test, just need to uncomment "test_interrupts()" in RTC_interrupt() in file RTC.c*/
	// test_divide_error();		/* test for divide_zero exception */
	// test_page_fault();		/* test for page_fault exception*/
	// test_excp_reserved();	/* test for reserved exception used in future debug */
	// test_one_excp();			/* used to test each exception handler */

	/*Test for i8259*/
	// test_i8259_disable_irq_garbage();	/*used to test whether garbage input will mask the interrupts*/
	// test_i8259_disable_irq();			/*used to test whether disable_irq can mask the interrupts from keyboard and rtc*/
	// test_i8259_enable_irq_garbage();		/*used to test whether garbage input will unmask the interrupts*/
	// test_i8259_enable_irq();				/*used to test whether enable_irq can unmask the interrupts from keyboard and rtc*/
	// printf("--------------------------TEST_SUCCESS----------------------------\n");

	// printf("-----------------------PRESS_KEYBOARD_TO_TEST-------------------------\n");

// 	clear();
// 	test_begin();
// 	/*Test the RTC driver functionality*/
// 	test_RTC();
// 	/*Test the terminal driver functionality*/
// 	test_terminal();
	
	// /*Test the file system driver functionality*/
	// test_file_name();
	// test_file_content_index();
	// test_file_content();
	// // test_long_filename();
	// printf("========================================================================\n");
	// printf("|                           TEST FINISHED                               |\n"); 
	// printf("========================================================================\n");

	// printf("========================================================================\n");
	// printf("|                           EXECUTE SHELL                              |\n"); 
	// printf("========================================================================\n");
	// execute((uint8_t*)"shell");
	while(1);

}


