/* syscall.c -- system calls and context switching
 * vim:ts=4 noexpandtab
 */

#include "syscall.h"
#include "tests.h"

#define ELF_SIZE 4
#define PROGRAM_LOAD_VIRT_ADDRESS 0x08048000
#define USER_PD_INDEX 32		//128mb/4mb
#define ENTRY_POINT_OFFSET 24   //entry point starts at byte 24

//functions to prevent writing to stdin and reading from stdout
static int32_t read_no_op(int32_t fd, void* buf, int32_t nbytes){return -1;};
static int32_t write_no_op(int32_t fd, const void* buf, int32_t nbytes){return -1;};
static int32_t open_no_op(const uint8_t* filename){return-1;};
static int32_t close_no_op(int32_t fd){return -1;};

/* File operations jump table for a file */
fo_jump_table_t file_fo_jump_table = {
    file_open,
    file_read,
    file_write,
    file_close
};

/* File operations jump table for a directory */
fo_jump_table_t dir_fo_jump_table = {
    file_open,
    dir_read,
    file_write,
    file_close
};

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

/* Jump table for rtc operations  */
fo_jump_table_t rtc_jump_table = {
	rtc_open,
	rtc_read,
	rtc_write,
	rtc_close
};

static uint8_t elf_magic[ELF_SIZE] = {0x7f, 0x45, 0x4c, 0x46}; //array to check for elf in file
//static uint8_t processes[MAX_NUM_PROCESSES];

/*
 * halt
 *   DESCRIPTION:	This system call terminates a proess and then returns the 
 * 					specified value to its parent proess. This is responsible 
 * 					for expanding the 8-bit argument from BL into the 32-bit 
 * 					return value to the parent program's exeute system all. 
 *					If no more processes, then reload/execute shell.
 *   INPUTS: 		Status
 *   OUTPUTS:		none
 *   RETURN VALUE: 	-1 for failure to terminate
 *   SIDE EFFECTS: 	Halts the currently executing process. 
 */
int32_t halt (uint8_t status){
	int i;
	uint32_t _status;

	cli();
	// modify status
	if(status == 255)
		_status = 256;
	else
		_status = status;
	/* Obtain the current pcb. */
	pcb_t* current = get_current_executing_pcb();

	/* If it is the first shell, restart the shell. */
	if(current->parent_pcb == NULL){
		tss.esp0 = (uint32_t)(get_pcb_by_PID(current->process_id - 1) - 4); //-4 because 32 bytes above the next pcb
		tss.ss0 = KERNEL_DS;
		asm volatile ("            \n\
	        cli                    \n\
	        movl %0, %%edx         \n\
	        movw %%dx, %%ds        \n\
	        pushl %0               \n\
	        movl $33, %%edx        \n\
	        shll $22, %%edx        \n\
			subl $4, %%edx         \n\
	        pushl %%edx            \n\
	        pushfl                 \n\
	        popl %%edx             \n\
	        orl $0x200, %%edx      \n\
	        pushl %%edx            \n\
	        pushl %1               \n\
	        pushl %2               \n\
	        iret                   \n\
			"
			:
			: "r" (USER_DS), "r" (USER_CS), "r" (current->entry)
			: "edx" // clobbers %EDX
		);
	}
	
	// clear arg buffer
	(current->args)[0] = '\0';
	/* Close all used files within the fd array in pcb. */
	for(i = 0; i < FD_ARRAY_LEN; i++) {
		if(current->fd_array[i].flags == 1)
			close(i);
		current->fd_array[i].flags = 0;
		current->fd_array[i].fo_jump_table_ptr = NULL; 
	}	

	/* Restore the old page mapping. */
	if (current->parent_pcb != NULL){
		create_user_4mb_page(current->parent_pcb->process_id + 2, USER_PD_INDEX);
		//current = current->parent_pcb;
		reload_cr3();
	}
	
	/* Set esp0 in tss. and ss0 */
	//tss.esp0 = stack pointer of previous pcb
	tss.esp0 = (uint32_t)(get_pcb_by_PID(current->parent_pcb->process_id - 1) - 4); //-4 because 32 bytes above the next pcb
	tss.ss0 = KERNEL_DS;

	//set process to inactive
	processes[current->process_id] = 0;

	//save values for the current PCB's base and stack pointer
	uint32_t current_ESP = current->parent_esp;
	uint32_t current_EBP = current->parent_ebp;
	
	current = current->parent_pcb;
	current->child_pcb = NULL;
	asm volatile("					\n\
				 xorl %%eax, %%eax  \n\
				 movl %2, %%eax		\n\
				 movl %0, %%esp 	\n\
				 movl %1, %%ebp 	\n\
				 leave 				\n\
				 ret 				\n\
				 "
				 :
                 :"r"(current_ESP), "r"(current_EBP), "r"(_status)
                 :"eax"
    );
	return -1; //should never reach this

}

/*
 * execute
 *   DESCRIPTION:	This system call attempts to load and exeute a new program, 
 *					handing off the proessor to the new program until it terminates. 
 * 					The command is a space-separated sequence of words. The first
 * 					is the file name of the program to be exeuted, and the rest of the 
 *					command provides the new program on request via the getargs system all. 
 *   INPUTS: 		command : full command with file and arguments
 *   OUTPUTS:		none
 *   RETURN VALUE: 	-1 if the command cannot be executed, or if the progam does not exist or filename is not
 *					executable. 
 *   SIDE EFFECTS: 	Loads/executes the specified program/filename
 */
int32_t execute (const uint8_t* command){
	// Allocate local variables
	uint8_t cmd[MAX_CMD_SIZE];					 //buffer to hold cmd (first string in command)
	uint8_t args[MAX_CMD_SIZE];	 //array to hold arguments (other strings in command)
	uint8_t buf[ELF_SIZE];					   //copy buffer for the ELF check
	dentry_t dentry;							 //dentry to copy into
	int bytes_to_read;						   //number of bytes to read from file
	int* esp0;								   //address of the kernel stack for current task
	pcb_t* pcb;					  //address of the pcb for the current task
	int i;									   //iterators
	uint32_t entry_point;						//entry point to user leve program
	int new_PID = -1;								//available PID
	pcb_t * prev_pcb;							//the previous pcb

	//return if NULL command
	if(command == NULL)
		return -1;	

	//PARSE COMMAND______________________________________________________________
	parse_command(cmd, args, command);		//call helper function: fills cmd and args

	//test return to parent program if exceptions occur
	if(!strncmp((int8_t*)cmd, (int8_t*)"exception", 9))  //9 because size of string "exception" is 9
		exception_test();

	//CHECK FOR VALID FILE AND EXECUTABLE_________________________________________
	if(read_dentry_by_name(cmd, &dentry) == -1){			//test if file exists
		return -1;
	}
	if(read_data(dentry.inode_index, 0, buf, ELF_SIZE) == -1)
		return -1;
	if(strncmp((int8_t*)buf, (int8_t*)elf_magic, ELF_SIZE) != 0)			//test if elf exists
		return -1;

	//SEARCH FOR AVAILABLE PROCESS ID. return 0 if no processes available
	for(i = 3; i < MAX_NUM_PROCESSES; i++){
		if(!processes[i]){
			new_PID = i;
			processes[new_PID] = 1;
			break;
		}
	}
	if(new_PID == -1){
		printf("Process # limit reached\n");
		return 0;
	}

	//SET UP PAGING______________________________________________________________
	// allocate a page for the new task at 128mb (index 32), pointing in real memory
	// to 8mb (index 2) for the first task and 12mb (index 3) for the second
	// and then flush the TLBs
	if (create_user_4mb_page(new_PID + 2, USER_PD_INDEX) != 0) {
		processes[new_PID] = 0; 
		return -1;	//return -1 if page didn't allocate
	}
	reload_cr3();


	//PROGRAM LOADER: COPY IMAGE INTO VIRTUAL ADDRESS________________________________
	bytes_to_read = inodes[dentry.inode_index].length;
	if(read_data(dentry.inode_index, 0, (uint8_t*)PROGRAM_LOAD_VIRT_ADDRESS, bytes_to_read) != bytes_to_read) {
		// We've hit an error, not everything copied so undo the paging stuff
		processes[new_PID] = 0;
		// won't actually do anything if we try to overwrite kernel page
		create_user_4mb_page(new_PID + 2, USER_PD_INDEX);
		reload_cr3();
		return -1;
	}
	read_data(dentry.inode_index, ENTRY_POINT_OFFSET, (uint8_t*)(&entry_point), 4); // get entry point

	
	//CREATE PCB__________________________________________________________________
	// Find the address of the kernel stack: 0x800000 in blocks of 8kb upwards
	prev_pcb = get_current_executing_pcb();
	esp0 = (int*)(get_pcb_by_PID(new_PID - 1)- 4);  //-4 because 32 bytes above the next pcb
	pcb = get_pcb_by_PID(new_PID);
	// initialize stdin and stdout
	pcb->fd_array[0].fo_jump_table_ptr = &stdin_jump_table;
	pcb->fd_array[1].fo_jump_table_ptr = &stdout_jump_table;
	pcb->fd_array[0].active = 1;
	pcb->fd_array[1].active = 1;
	pcb->entry = entry_point;
	//fill argument string in pcb
	strcpy((int8_t*)pcb->args, (int8_t*)args);
	// Clear the rest of the FD array
	for (i = 2; i < FD_ARRAY_LEN; i++) {
		pcb->fd_array[i].active = 0;
	}
	pcb->process_id = new_PID; //save process ID

	//save parent ebp and esp
	asm volatile("movl %%esp, %0":"=g"(pcb->parent_esp));
	asm volatile("movl %%ebp, %0":"=g"(pcb->parent_ebp));
	
	// Link to parent pcb and set child to NULL
	if(new_PID < 3){
		pcb->parent_pcb = NULL;
	}
	else{
		pcb->parent_pcb = prev_pcb;
		prev_pcb->child_pcb = pcb;
	}
		
	pcb->child_pcb = NULL;


	// CONTEXT SWITCH_______________________________________________________________
	//update tss
	cli();
	tss.esp0 = (uint32_t)esp0;		//kernel stack pointer
	tss.ss0 = KERNEL_DS;			//kernal data segment = kernal stack segment
	// Push IRET context to stack
	/* cs and ds registers are of the form:
	 * 13 bits GDT index, 1 bit table descriptor, 2 bits RPL
	 * ex index 3 in GDT (user cs) with RPL=3 would be:
	 * 0000000000011 0 11 or 0x1b
	 *
	 * After setting the data segment (cs set from iret) and updating the TSS
	 * entry to point to the newly allocated kernel stack, we push the
	 * following to the stack for iret in order:
	 * new stack segment (user data segment)
	 * new stack pointer (bottom of 4MB user program page, 33 << 22)
	 * flags
	 * new code segment
	 * new instruction pointer (bytes 24-27 of loaded executable)
	 *	 - address is 128MB (32 << 22) + either 24 or 27, try both
	 *
	 * We need to prevent interrupts during this, but we can't call
	 * sti from userspace.  So we clear the IF flag in the EFLAGS value
	 * saved on the stack.
	 */
	asm volatile ("            \n\
        cli                    \n\
        movl %0, %%edx         \n\
        movw %%dx, %%ds        \n\
        pushl %0               \n\
        movl $33, %%edx        \n\
        shll $22, %%edx        \n\
		subl $4, %%edx         \n\
        pushl %%edx            \n\
        pushfl                 \n\
        popl %%edx             \n\
        orl $0x200, %%edx      \n\
        pushl %%edx            \n\
        pushl %1               \n\
        pushl %2               \n\
        iret                   \n\
		"
		:
		: "r" (USER_DS), "r" (USER_CS), "r" (entry_point)
		: "edx" // clobbers %EDX
	);

	// Loaded successfully
	return 0;
}

/*
 * read
 *   DESCRIPTION: 	Reads data from keyboard, a file, RTC, or directory.
 *  				Reads from a entry in file descriptor array using a 
 * 					function pointer to a jump table.
 *   INPUTS: 		fd: file descriptor number to read from
 * 					buf: buffer to fill
 * 					nbytes: number of bytes to read
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 for failure
 *   SIDE EFFECTS: 	none
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes){
	// Use file operations jump table to call the corresonding read
	pcb_t* pcb_ptr; // Pointer to this task's pcb
	fd_entry_t* fd_array; // Array of file descriptors in this pcb

	// OOB check for fd and nbytes, NULL check for buf, and check that a user task exists
	if (fd < 0 || fd >= FD_ARRAY_LEN || buf == NULL || nbytes < 0)
		return -1;

	pcb_ptr = get_current_executing_pcb();
	fd_array = pcb_ptr->fd_array;
	// unopened check
	if(!fd_array[fd].active)
		return -1;

	return (*(fd_array[fd].fo_jump_table_ptr->read))(fd, buf, nbytes);
}

/*
 * write
 *   DESCRIPTION: 	Writes data to terinal or a device(RTC), basically
 * 					this function writes to a file.
 *   INPUTS: 		fd: file descriptor number to read from
 * 					buf: buffer to fill
 * 					nbytes: number of bytes to read
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 for failure
 *   SIDE EFFECTS: 	Posibility of setting the RTC
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes){
	// Use file operations jump table to call the corresponding write
	pcb_t* pcb_ptr; // Pointer to this task's pcb
	fd_entry_t* fd_array; // Array of file descriptors in this pcb

	// OOB check for fd and nbytes, NULL check for buf, and check that a user task exists
	if (fd < 0 || fd >= FD_ARRAY_LEN || buf == NULL || nbytes < 0)
		return -1;

	pcb_ptr = get_current_executing_pcb();
	fd_array = pcb_ptr->fd_array;
	// unopened check
	if(!fd_array[fd].active)
		return -1;

	return (*(fd_array[fd].fo_jump_table_ptr->write))(fd, buf, nbytes);
}

/*
 * open
 *   DESCRIPTION: 	Provides access to a file system by opening the given
 * 					file, directory, or device.
 *   INPUTS: 		filename: filename to open
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 for failure
 *   SIDE EFFECTS: 	Sets up specified function pointers for file type. 
 */
int32_t open (const uint8_t* filename){
	// Find the file in the file system and assign an unsed file descriptor
	// File desriptrs must be set up according to file type
	dentry_t dentry; // Dentry for file
	pcb_t* pcb_ptr; // Pointer to this task's pcb
	fd_entry_t* fd_array; // Array of file descriptors in this pcb
	int i; // Iterator

	// Validity checks.  If we're good, read the directory entry and get the file descriptor array
	if (filename == NULL || read_dentry_by_name(filename, &dentry) != 0)
		return -1;
	pcb_ptr = get_current_executing_pcb();
	fd_array = pcb_ptr->fd_array;
	i = 2;
	while (fd_array[i].active) {
		i++;
		if (i >= FD_ARRAY_LEN) // no available fd entries
			return -1;
	}
	// We've found an empty fd entry, populate it
	fd_array[i].file_position = 0;
	strncpy((int8_t*)pcb_ptr->filenames[i], (int8_t*)dentry.file_name, USER_PD_INDEX);
	switch (dentry.file_type) {
		case 0: // RTC
			fd_array[i].fo_jump_table_ptr = &rtc_jump_table;
			fd_array[i].inode_index = 0;
			break;
		case 1: // Directory
			fd_array[i].fo_jump_table_ptr = &dir_fo_jump_table;
			fd_array[i].inode_index = 0;
			break;
		case 2: // File
			fd_array[i].fo_jump_table_ptr = &file_fo_jump_table;
			fd_array[i].inode_index = dentry.inode_index;
			break;
		default: // Error
			return -1;
	}
	// Mark it as active last in case of error
	fd_array[i].active = 1;

	// Call any type-specific open function
	//(*(fd_array[i].fo_jump_table_ptr->open))(filename);

	// Return the index of the opened file descriptor
	return i;
}

/*
 * close
 *   DESCRIPTION: 	Closes a specified file descriptor and makes it avaliable for 
 * 					return from later calls to open.
 *   INPUTS: 		fd: file descriptor index to close
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 for failure
 * 					0 for sucess
 *   SIDE EFFECTS: 	Free the file descriptor for other uses. 
 */
int32_t close (int32_t fd){
	// Close the file descriptor passed in(set it to be avaliable)
	// Check for invalid descriptors
	pcb_t* pcb_ptr; // Pointer to this task's pcb
	fd_entry_t* fd_array; // Array of file descriptors in this pcb

	// Only close valid fd's, excluding stdin and stdout (so fd < 2).  Also fail if active tasks
	if (fd < 2 || fd >= FD_ARRAY_LEN)
		return -1;

	pcb_ptr = get_current_executing_pcb();
	fd_array = pcb_ptr->fd_array;
	if (!fd_array[fd].active)
		return -1;
	else
		fd_array[fd].active = 0;
	return 0;
	//return (*(fd_array[fd].fo_jump_table_ptr->close))(fd);
}

/*
 * getargs
 *   DESCRIPTION: 	copies arguments from command read from terminal into
 *					buf
 *   INPUTS: 		buf: buffer to copy args to, nbytes: nbytes to copy
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 for failure
 * 					0 for sucess
 *   SIDE EFFECTS: none 
 */
int32_t getargs (uint8_t* buf, int32_t nbytes){
	//local variables
	int args_length;  //length of argument	
	int i = 0; //counter
	pcb_t * pcb_ptr = get_current_executing_pcb(); //pointer to current pcb
	args_length = strlen((int8_t*)pcb_ptr->args);
	if(!args_length)
		return -1;

	//error check buf (is in user space, not null)
	if(buf == NULL 
	   || nbytes < args_length
	   || ((uint32_t)buf < (USER_PD_INDEX << ALIGN_4MB))
	   || ((uint32_t)buf >= (((USER_PD_INDEX+1) << ALIGN_4MB) - 4)))
		return -1;

	//copy all arguments
	while(i < args_length && i < nbytes){
		buf[i] = pcb_ptr->args[i];
		i++;
	}
	//null terminate buffer
	buf[i] = '\0';
	return 0;
}

/*
 * vidmap
 *   DESCRIPTION: copies user level virtual address that is mapped to
 *				  video memory in physical space into screen_start
 *   INPUTS: 		screen_start: address to copy new virtual address to
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 for failure
 * 					0 for sucess
 *   SIDE EFFECTS: 	changes paging structure: creates user level page; flushes tlb
 */
int32_t vidmap (uint8_t** screen_start){
	//error check screen_start (is in user space, not null)
	if(screen_start 
	   && ((uint32_t)screen_start >= (USER_PD_INDEX << ALIGN_4MB))
	   && ((uint32_t)screen_start < (((USER_PD_INDEX+1) << ALIGN_4MB) - 4)))
		*screen_start = (uint8_t*)create_vid_4kb_page(); //create page and copy to screen_start
	else
		return -1;
	// printf("VIDMAP CALLED in process: %d\n", get_current_executing_pcb()->process_id);
	reload_cr3();
	return 0;
}

/*
 * set_handler
 *   DESCRIPTION: 	Related to signal handling and changes the default action taken when a
 *					signal is recieved. 
 *   INPUTS: 		int32_t signum : value of signal number
 					void* handler_address : pointer to the handler address
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 our system calls do not have use for this function.
 *   SIDE EFFECTS: 	none
 */
int32_t set_handler (int32_t signum, void* handler_address){
	return -1;
}

/*
 * sigreturn
 *   DESCRIPTION: 	Copy the hardware context that was on the user level stack back onto the
 *					processor.
 *   INPUTS: 		void
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	-1 our system calls do not have use for this function.
 *   SIDE EFFECTS: 	none
 */
int32_t sigreturn (void){
	return -1;
}

/*
 * boot
 *   DESCRIPTION: 	Responsible for initializing and setting up pages for each of the
 *					three terminal windows. First it initializes all the processes that
 *					can be run. Then for each terminal, we set up a user 4mb page and load
 *					in the shell user program. We then update each file descriptor array and
 *					save all necessary pcb identifiers such as entry point, esp, ebp, and 
 * 					process id. At the end of setting up all three terminals we then update the
 *					tss, remap to the first shell(index 2) and context switch.
 *   INPUTS: 		none
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	changes paging structure: creates user level page; flushes tlb
 */
void boot(){
	init_terminal(); //initialize terminals
	// Allocate local variables
	dentry_t dentry;							 //dentry to copy into
	int bytes_to_read;						   	 //number of bytes to read from file
	pcb_t* pcb;								  	 //address of the pcb for the current task
	int i, j;									 //iterators
	uint32_t entry_point;						 //entry point to user leve program

	//initialize process_id array to all inactive
	for(i = 0; i < MAX_NUM_PROCESSES; i++)
		processes[i] = 0;

	//LOAD SHELL DENTRY_________________________________________
	read_dentry_by_name((uint8_t*)"shell", &dentry);		//test if file exists

	//INITIALIZE 3 SHELLS (WHICH IS WHY THERE'S A 3 in the for loop)
	for(i = 0; i < 3; i++){
		//SET UP PAGING______________________________________________________________
		// allocate a page for the new task at 128mb (index 32), pointing in real memory
		// to 8mb (index 2) for the first task and 12mb (index 3) for the second
		// and then flush the TLBs
		create_user_4mb_page(i+2, USER_PD_INDEX);
		reload_cr3();	

		//PROGRAM LOADER: COPY IMAGE INTO VIRTUAL ADDRESS________________________________
		bytes_to_read = inodes[dentry.inode_index].length;
		exec_term_id = i;
		read_data(dentry.inode_index, 0, (uint8_t*)PROGRAM_LOAD_VIRT_ADDRESS, bytes_to_read);
		read_data(dentry.inode_index, ENTRY_POINT_OFFSET, (uint8_t*)(&entry_point), 4); // get entry point

		
		//CREATE PCB__________________________________________________________________
		// Find the address of the kernel stack: 0x800000 in blocks of 8kb upwards
		pcb = get_pcb_by_PID(i);
		// initialize stdin and stdout
		pcb->fd_array[0].fo_jump_table_ptr = &stdin_jump_table;
		pcb->fd_array[1].fo_jump_table_ptr = &stdout_jump_table;
		pcb->fd_array[0].active = 1;
		pcb->fd_array[1].active = 1;
		//fill entry point into program
		pcb->entry = entry_point;
		//fill argument string in pcb
		pcb->args[0] = '\0';
		// Clear the rest of the FD array
		for (j = 2; j < FD_ARRAY_LEN; j++) {
			pcb->fd_array[j].active = 0;
		}
		pcb->process_id = i; // PID

		//save parent ebp and esp
		asm volatile("movl %%esp, %0":"=g"(pcb->parent_esp));
		asm volatile("movl %%ebp, %0":"=g"(pcb->parent_ebp));

		pcb->parent_pcb = NULL;
		pcb->child_pcb = NULL;
	}
	curr_term_id = 0;
	exec_term_id = 0;
	create_user_4mb_page(exec_term_id + 2, USER_PD_INDEX); //remap to first shell (index 2)
	reload_cr3();	
	// CONTEXT SWITCH_______________________________________________________________
	//update tss
	tss.esp0 = (uint32_t)(get_pcb_by_PID(exec_term_id -1) - 4);		//kernel stack pointer
	tss.ss0 = KERNEL_DS;			//kernal data segment = kernal stack segment

	processes[0] = 1;				//set first process to active

	// Push IRET context to stack
	/* cs and ds registers are of the form:
	 * 13 bits GDT index, 1 bit table descriptor, 2 bits RPL
	 * ex index 3 in GDT (user cs) with RPL=3 would be:
	 * 0000000000011 0 11 or 0x1b
	 *
	 * After setting the data segment (cs set from iret) and updating the TSS
	 * entry to point to the newly allocated kernel stack, we push the
	 * following to the stack for iret in order:
	 * new stack segment (user data segment)
	 * new stack pointer (bottom of 4MB user program page, 33 << 22)
	 * flags
	 * new code segment
	 * new instruction pointer (bytes 24-27 of loaded executable)
	 *	 - address is 128MB (32 << 22) + either 24 or 27, try both
	 *
	 * We need to prevent interrupts during this, but we can't call
	 * sti from userspace.  So we clear the IF flag in the EFLAGS value
	 * saved on the stack.
	 */	
	init_pit();
	asm volatile ("            \n\
        cli                    \n\
        movl %0, %%edx         \n\
        movw %%dx, %%ds        \n\
        pushl %0               \n\
        movl $33, %%edx        \n\
        shll $22, %%edx        \n\
		subl $4, %%edx         \n\
        pushl %%edx            \n\
        pushfl                 \n\
        popl %%edx             \n\
        orl $0x200, %%edx      \n\
        pushl %%edx            \n\
        pushl %1               \n\
        pushl %2               \n\
        iret                   \n\
		"
		:
		: "r" (USER_DS), "r" (USER_CS), "r" (entry_point)
		: "edx" // clobbers %EDX
	);
}


/*
 * get_pcb_by_PID
 *   DESCRIPTION: 	Obtains the pcb_t pointer based upon the given PID value. We use
 * 					the equation (8mb - (PID+1)8kb).
 *   INPUTS: 		int PID : PID value we want to find the pcb pointer for
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	pcb_t* : pointer to pcb based on PID passed in
 *   SIDE EFFECTS: 	none
 */
pcb_t* get_pcb_by_PID(int PID){
	return (pcb_t*)(_8MEGA - ((PID + 1) * _8KILO));
}

/*
 * get_current_executing_pcb
 *   DESCRIPTION: 	Obtains the pcb_t pointer based upon the currently executing terminal id.
 * 					Then we walk down the list of child nodes inorder to find the child executing
 *					on the specific executing terminal. Once we have obtained the child node we return
 * 					the pcb pointer to it. 
 *   INPUTS: 		none
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	pcb_t* : pointer to pcb based on currently executing terminal id
 *   SIDE EFFECTS: 	none
 */
pcb_t* get_current_executing_pcb() {
	// Get the current top-level node
	pcb_t* curr_node = get_pcb_by_PID(exec_term_id);
	// Traverse its children (if any) until you find a task
	// with no children
	while (curr_node->child_pcb != NULL)
		curr_node = curr_node->child_pcb;
	// At leaf node
	return curr_node;
}

/*
 * get_current_displaying_pcb
 *   DESCRIPTION: 	Obtains the pcb_t pointer based upon the currently displaying terminal id.
 * 					Then we walk down the list of child nodes inorder to find the child executing
 *					on the specific displaying terminal. Once we have obtained the child node we return
 * 					the pcb pointer to it. 
 *   INPUTS: 		none
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	pcb_t* : pointer to pcb based on currently displaying terminal id
 *   SIDE EFFECTS: 	none
 */
pcb_t* get_current_displaying_pcb() {
	// Get the current top-level node
	pcb_t* curr_node = get_pcb_by_PID(curr_term_id);
	// Traverse its children (if any) until you find a task
	// with no children
	while (curr_node->child_pcb != NULL)
		curr_node = curr_node->child_pcb;
	// At leaf node
	return curr_node;
}




