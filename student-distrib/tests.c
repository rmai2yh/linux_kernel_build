#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "terminal.h"
#include "types.h"
#include "rtc.h"
#include "fs.h"
#include "lib.h"
#include "types.h"
#include "syscall.h"
#include "tasks.h"

#define PASS 1
#define FAIL 0

#define MAX_FILE_SIZE 36164

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

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
static int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}


/* provided_fs_test
 *
 * Checks that pointers to the ROFS have been
 * correctly setup by checking random data structures in
 * the fs against known values from the default filesystem.
 * These known values were found using `xdd filesys_img`
 *
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side effects: None
 * Coverage: filesystem
 */
static int provided_fs_test() {
	TEST_HEADER;

	// checks that there are 17 directory entries
	int result = PASS;
	if (boot_block->num_dentries != 0x11) {
		assertion_failure();
		result = FAIL;
	}
	// checks that there are 64 inodes
	if (boot_block->num_inodes != 0x40) {
		assertion_failure();
		result = FAIL;
	}
	// checks that the first directory entry is .
	if (dentries[0].file_name[0] != '.') {
		assertion_failure();
		result = FAIL;
	}
	// checks that the 18th file name is empty
	if (dentries[17].file_name[0] != '\0') {
		assertion_failure();
		result = FAIL;
	}
	// checks that the 4th inode has a length of 0x1445
	if (inodes[3].length != 0x1445) {
		assertion_failure();
		result = FAIL;
	}
	// checks that the first data block is empty
	if (data_blocks[0].data[0] != 0) {
		assertion_failure();
		result = FAIL;
	}
	// checks that the 3rd data block begins with 0x7f
	if (data_blocks[2].data[0] != 0x7f) {
		assertion_failure();
		result = FAIL;
	}
	// create a directory entry and populate it with the ls executable, then
	// check that its inode index is 5
	dentry_t test_dentry;
	if ((read_dentry_by_name((uint8_t*)("ls"), &test_dentry) != 0) ||
		(test_dentry.inode_index != 5)) {
		assertion_failure();
		result = FAIL;
	}
	// now load the dentry at index 10 and make sure it's frame0.txt
	if ((read_dentry_by_index(10, &test_dentry) != 0) ||
		(strncmp("frame0.txt", test_dentry.file_name, 32) != 0)) {
		assertion_failure();
		result = FAIL;
	}
	// create a buffer to read data into and a fish to compare
	uint8_t buf[2277];
	char fish[187] = "/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\n"
						"         o\n"
						"           o    o\n"
						"       o\n"
						"             o\n"
						"        o     O\n"
						"    _    \\\n"
						" |\\/.\\   | \\/  /  /\n"
						" |=  _>   \\|   \\ /\n"
						" |/\\_/    |/   |/\n"
						"----------M----M--------\n";
	// read frame0.txt and make sure it's the same as the fish
	if ((read_data(test_dentry.inode_index, 0, buf, 187) != 0) ||
		(strncmp((int8_t*) buf, fish, 187) != 0)) {
		assertion_failure();
		result = FAIL;
	}
	// now read the bytes of a very long file on the boundary between two
	// data blocks to check that the read loops
	if ((read_dentry_by_index(11, &test_dentry) != 0) ||
		(read_data(test_dentry.inode_index, 3000, buf, 2277) != 0) ||
		(buf[0] != 'N') || (buf[2000] != '5')) {
		assertion_failure();
		result = FAIL;
	}
	// return the result if we get here (should be a pass)
	return result;
}

/* paging test
 * 
 * Asserts that first 4 entries of array are accessable and accurate
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: paging 
 */
static int paging_test() {
    TEST_HEADER;

    int result = PASS;
    int i;
    uint32_t test_arr[4];
    // uint32_t cr;
    // asm ("movl %%cr0, %0" : "=r" (cr));
    // if ((cr & 0x80000000) == 0) {
    //     assertion_failure();
    //     result = FAIL;
    // }
    // asm ("movl %%cr4, %0" : "=r" (cr));
    // if ((cr & 0x00000008) == 0) {
    //     assertion_failure();
    //     result = FAIL;
    // }
    // just try a ton of r/w instead

    for (i = 0; i < 4; i++) {
        test_arr[i] = 1 << i;
    }
    if ((test_arr[0] != 1) |
        (test_arr[1] != 2) |
        (test_arr[2] != 4) |
        (test_arr[3] != 8)) {
        assertion_failure();
        result = FAIL;
    }
    return result;
}


/* page fault test - tries to dereference NULL
 * Inputs: None
 * Outputs: None
 * Side Effects: will cause page fault exception
 * Coverage: page_fault test 
 */
static void page_fault_test() {
    int * x = NULL;
    int y = * x;
    y++;
};

/* video paging test: should clear screen if 4kb paging is set up properly
 * Inputs: None
 * Outputs: None
 * Side Effects: clears the terminal screen
 * Coverage: paging 
 */
static void video_paging_test() {
    clear();
};

/*
 * rtc_test
 *   DESCRIPTION:   Demonstrates that we can change the rate of the RTC clock 
 *                  using the write function and that the read function retuns 
 *                  after an interrupt has occured. The RTC test function 
 *                  increments a counter and then writes to that specific spot 
 *                  on the screen.
 *   INPUTS:        none
 *   OUTPUTS:       none
 *   RETURN VALUE:  none
 *   SIDE EFFECTS:  Sets RTC interrupts to frequency desired and prints to the
 *                  screen.
 *   COVERAGE:      RTC
 */
static void rtc_test() {

    /* clear the screen. */
    clear();

    /* Set all needed values. */
    const uint8_t * filename = NULL;
    uint32_t ret_value;
    uint32_t fd = NULL;
    int freq = 2;       //default frequency
    int32_t nbytes = 4;
    void* read_buf = NULL;
    int rate;
    int curr;

    /* "Open" the RTC file. */
    rtc_open(filename);

    /* Loop through all frequencies for all given rates. */
    for(rate = 0; rate < 10; rate++) {

        /* Given the frequency, set the rate. */
        ret_value = rtc_write(fd, &freq, nbytes);

        /* Loop through to the given value of the frequency. */
        for(curr = 0; curr < freq; curr++) {
            
            /* Once an interrupt finishes, then print 1. */
            if(rtc_read(fd, read_buf, nbytes) == 0) {
                printf("1");
            }
        }

        /* Increment the frequency to a power of 2. */
        freq = freq*2;
        clear();
    }

    /* Prove that rtc_open works as rtc write, setting default freqnecy. */
    rtc_open(filename);
    for(curr = 0; curr < freq; curr++) {
        if(rtc_read(fd, read_buf, nbytes) == 0) {
            printf("1");
        }
    }
}


// add more tests here

/* Checkpoint 2 tests */
/* terminal_read_test
 *
 * Lets the user type an input, then return what they typed
 * prefixed with a "TERMINAL HAS READ: "
 *
 *   INPUTS:        none
 *   OUTPUTS:       none
 *   RETURN VALUE:  none
 *   SIDE EFFECTS:  Changes the contents of the screen
 *   COVERAGE:      terminal
 */
static void terminal_read_test(){
    char buf[BUFF_SIZE];

    while(1){
        if(terminal_read(0, (void*)buf, 0) > 0){      
            printf("TERMINAL HAS READ: %s \n", buf);
        }
    }
};

/* terminal_write_test
 *
 * Writes "abcd" to the terminal
 *
 *   INPUTS:        none
 *   OUTPUTS:       none
 *   RETURN VALUE:  none
 *   SIDE EFFECTS:  Changes the contents of the screen
 *   COVERAGE:      terminal
 */
static void terminal_write_test(){
    char buf[6] = "abcd\n";
    terminal_write(0, buf, 0);
};

/* fs_test_ls
 *
 * Lists all characters in the file system with their type and size
 *
 *   INPUTS:        none
 *   OUTPUTS:       PASS/FAIL
 *   SIDE EFFECTS:  Changes the contents of the screen
 *   COVERAGE:      filesystem
 */
static int fs_test_ls() {
	int i;
	int result = PASS;
	char file_string[33];
	dentry_t dentry;
	// iterate through every dentry index
	for (i = 0; i < MAX_NUM_DENTRIES; i++) {
		if (read_dentry_by_index(i, &dentry) != 0) {
			assertion_failure();
			result = FAIL;
		}
		// check to see if it exists (has a file name)
		if (dentry.file_name[0] != '\0') {
			// zero terminate file name
			strncpy(file_string, dentry.file_name, 32);
			file_string[32] = '\0';
			// print off the info for this dentry
			printf("file name: %s, file type: %d, file size: %d\n", file_string, dentry.file_type, inodes[dentry.inode_index].length);
		}
	}
	return result;
}

/* fs_print_by_name
 *
 * Reads a file by name and prints it to the screen
 *
 *   INPUTS:        none
 *   OUTPUTS:       PASS/FAIL
 *   SIDE EFFECTS:  Changes the contents of the screen
 *   COVERAGE:      filesystem
 */
static int fs_print_by_name(const uint8_t * fname) {
	// setup the variables, including a buffer large enough to
	// fit the largest file provided by our read-only filesystem
	int result = PASS;
    dentry_t dentry;
    uint32_t index, length;
    uint8_t buf[MAX_FILE_SIZE + 1];

    if (read_dentry_by_name(fname, &dentry) != 0) {
		result = FAIL;
		assertion_failure();
	}
    index = dentry.inode_index;
    length = inodes[index].length;
    buf[length] = '\0';

    if (read_data(index, 0, buf, length) != 0) {
		result = FAIL;
		assertion_failure();
	}
    //terminal_write(0, buf, 0);
    printf("%s", buf);
	return result;
};

/* fs_print_by_index
 *
 * Reads a file by its dentry index and prints it to the screen
 *
 *   INPUTS:        index -- dentry index
 *   OUTPUTS:       PASS/FAIL
 *   SIDE EFFECTS:  Changes the contents of the screen
 *   COVERAGE:      filesystem
 */
static int fs_print_by_index(uint32_t index){
	int result = PASS;
    dentry_t dentry;
    uint32_t inode_idx, length;
    uint8_t buf[MAX_FILE_SIZE + 1];

	// try to read the dentry by the provided index, make sure it works
	if (read_dentry_by_index(index, &dentry) != 0) {
		result = FAIL;
		assertion_failure();
	}
    inode_idx = dentry.inode_index;
    length = inodes[inode_idx].length;
    buf[length] = '\0'; // ensure null terminated buffer

	// Now try to read the data
	if (read_data(inode_idx, 0, buf, length) != 0) {
		result = FAIL;
		assertion_failure();
	}
    terminal_write(0, buf, 0);
    //printf("%s", buf);
	return result;
};
/* Checkpoint 3 tests */

/* syscall_rwoc_test
 *
 * Tests the read/write/open/close system calls
 *
 * INPUTS: None
 * RETURNS: PASS/FAIL
 * SIDE EFFECTS: None
 * COVERAGE: system calls
 */
//functions to prevent writing to stdin and reading from stdout
static int32_t read_no_op(int32_t fd, void* buf, int32_t nbytes){return -1;};
static int32_t write_no_op(int32_t fd, const void* buf, int32_t nbytes){return -1;};
static int32_t open_no_op(const uint8_t* filename){return-1;};
static int32_t close_no_op(int32_t fd){return -1;};
// the actual test
static int syscall_test() {
    int fd, fd2; // file descriptor index
    int rtc_fd;
    int i; // iter
    char buf[128]; // buffer for reads and writes
    pcb_t* pcb; // PCB struct to fill in for tests

    int result = PASS; // PASS/FAIL

    // Check to make sure all functions won't work if no user tasks are running
    if (open((uint8_t*)"frame0.txt") != -1 ||
        read(0, buf, 10) != -1 ||
        write(0, buf, 10) != -1 ||
        close(2) != -1) {
            result = FAIL;
            assertion_failure();
    }

    // Let's fake an execute call.  IE setup all variables
    // in the first pcb that relate to system calls
    // This is part of the execute call in syscall.c:execute
    /* Jump table for stdin operations */
    fo_jump_table_t stdin_jump_table = {
        open_no_op,
        terminal_read,
        write_no_op,
        close_no_op
    };
    /* Jump table for stdout operations */
    fo_jump_table_t stdout_jump_table = {
        open_no_op,
        read_no_op,
        terminal_write,
        close_no_op
    };
    active_tasks++;
	pcb = (pcb_t*)(_8MEGA - (active_tasks * _8KILO));
	// initialize stdin and stdout
	pcb->fd_array[0].fo_jump_table_ptr = &stdin_jump_table;
	pcb->fd_array[1].fo_jump_table_ptr = &stdout_jump_table;
	pcb->fd_array[0].active = 1;
	pcb->fd_array[1].active = 1;
	// Clear the rest of the FD array
	for (i = 2; i < FD_ARRAY_LEN; i++) {
		pcb->fd_array[i].active = 0;
	}
    pcb->parent_pcb = NULL;
	pcb->child_pcb = NULL;

    // Bad file names should return -1
    if (open((uint8_t*)"bad_filename.error") != -1 ||
        open((uint8_t*)"") != -1 ||
        open((uint8_t*)NULL) != -1) {
            result = FAIL;
            assertion_failure();
    }

    // You shouldn't be work on a random fd
    if (read(2, buf, 10) != -1 ||
        write(2, buf, 10) != -1 ||
        close(2) != -1) {
            result = FAIL;
            assertion_failure();
    }

    // Opening a file should have a return val 2-7
    fd = open((uint8_t*)"frame0.txt");
    if (fd < 2 || fd >= FD_ARRAY_LEN) {
		result = FAIL;
		assertion_failure();
    }
    // Populate the rest of the fds and check that you can't open any more
    for (i = 3; i < FD_ARRAY_LEN; i++) {
        fd2 = open((uint8_t*)".");
        if (fd2 < 2 || fd >= FD_ARRAY_LEN) {
            result = FAIL;
            assertion_failure();
        }
    }
    if (open((uint8_t*)".") != -1) {
		result = FAIL;
		assertion_failure();
    }

    // Test reads and writes
    // Test bad values for buf or nbytes
    if (read(fd, NULL, 0) != -1 ||
        write(fd, NULL, 0) != -1) {
            result = FAIL;
            assertion_failure();
    }
    if (read(fd, buf, -1) != -1 ||
        write(fd, buf, -1) != -1) {
            result = FAIL;
            assertion_failure();
    }
    if (read(fd, buf, 0) != 0) {
            result = FAIL;
            assertion_failure();
    }
    // This should work
    if (read(fd, buf, 4) != 4 ||
        buf[0] != '/' || buf[1] != '\\' ||
        buf[2] != '/' || buf[3] != '\\') {
            result = FAIL;
            assertion_failure();
    }
    // Make sure write doesn't do anything in a read-only fs
    if (write(fd, buf, 10) != -1) {
		result = FAIL;
		assertion_failure();
    }

    // Check that close acts in a sane manner
    if (close(-1) != -1 || close(FD_ARRAY_LEN) != -1) {
		result = FAIL;
		assertion_failure();
    }
    // Check that we can't close stdin or stdout
    if (close(0) != -1 || close(1) != -1) {
		result = FAIL;
		assertion_failure();
    }
    // Actually close everything
    for (i = 2; i < FD_ARRAY_LEN; i++) {
        if (close(i) != 0) {
            result = FAIL;
            assertion_failure();
        }
    }

    rtc_fd = open((uint8_t*)"rtc");
    //test bad rtc read write
    if (read(rtc_fd, NULL, 0) != -1 ||
        write(rtc_fd, NULL, 0) != -1) {
            result = FAIL;
            assertion_failure();
    }
    if (read(rtc_fd, buf, -1) != -1 ||
        write(rtc_fd, buf, -1) != -1) {
            result = FAIL;
            assertion_failure();
    }
    if (read(rtc_fd, buf, 0) != 0) {
            result = FAIL;
            assertion_failure();
    }

    //test bad getargs parameters
    //null buff
    if (getargs(NULL, 1) != -1) {
            result = FAIL;
            assertion_failure();
    }
    //bad nbytes (33 << 22 = 128mb (user space))
    if (getargs((void*)(33 << 22), -1) != -1) {
            result = FAIL;
            assertion_failure();
    }
    //buf not in user space
    if (getargs((void*)1, 1) != -1) {
            result = FAIL;
            assertion_failure();
    }

    //test bad vidmap parameter
    //null screen_start
    if(vidmap(NULL) != -1){
        result = FAIL;
        assertion_failure();
    }
    //not user space address
    if(vidmap((void*)1) != -1){
        result = FAIL;
        assertion_failure();
    }

    // Reset active_tasks to 0
    active_tasks = 0;

    return result;
}

void exception_test(){
	//int x = 1/0;
}
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
    TEST_OUTPUT("paging_test", paging_test());
    if(PAGE_FAULT_EXCEPTION_TEST_FLAG)
        page_fault_test();
    if(VIDEO_PAGING_TEST_FLAG)
        video_paging_test();
    if(TERMINAL_READ_TEST_FLAG)
        terminal_read_test();
    if(TERMINAL_WRITE_TEST_FLAG)
        terminal_write_test();
    if(RTC_TEST_FLAG)
        rtc_test();
	if (DEFAULT_FS_TEST_FLAG)
		TEST_OUTPUT("provied_fs_test", provided_fs_test());
	if (FS_LS_TEST_FLAG)
		fs_test_ls();
    if(FS_PRINT_BY_NAME_TEST_FLAG)
        fs_print_by_name((uint8_t*)("verylargetextwithverylongname.txt"));
    if(FS_PRINT_BY_INDEX_TEST_FLAG)
        fs_print_by_index(10);
    if(SYSCALL_TEST_FLAG)
    	TEST_OUTPUT("syscall_test", syscall_test());
}
