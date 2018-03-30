#include "terminal.h"

#define VIDEO       0xB8000
#define ATTRIB      0x7
// stores all terminals

/* init_terminal 
 * Description: Initializes all terminals
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 */
void init_terminal() {
	int i, j;
	//initialize terminals to 0
	for(i = 0; i < NUM_TERMINALS; i++){
		terms[i].x_save = 0;
		terms[i].y_save = 0;
		terms[i].buff_save[0] = '\0';
		terms[i].enters_save = 0;
		terms[i].buff_index_save = 0;
		// initialize nondisplay buffers to blank
		for (j = 0; j < NUM_ROWS * NUM_COLS; j++) {
	        *(uint8_t *)(terms[i].vid_save + (j << 1)) = ' ';
	        *(uint8_t *)(terms[i].vid_save + (j << 1) + 1) = ATTRIB;
    	}
	}
	curr_term_id = 0;
	exec_term_id = 0;
}

/* switch_displaying_term
 *
 * Description: switches the terminal currently being shown on the screen
 * Inputs: term_id : terminal to switch to
 * Outputs: None
 * Side Effects: None
 */
void switch_displaying_term(int term_id) {
	pcb_t * pcb;
	//printf("REACHED\n");
	// save curr terminal data: key_buff, video memory, coordinates, num_enters
	memcpy(terms[curr_term_id].buff_save, (uint8_t*)key_buff, (uint32_t)KEY_BUFF_SIZE);
	memcpy(terms[curr_term_id].vid_save, (uint8_t*)VIDEO, (uint32_t)VID_SIZE);
	terms[curr_term_id].x_save = get_x();
	terms[curr_term_id].y_save = get_y();
	terms[curr_term_id].enters_save = num_enters;
	terms[curr_term_id].buff_index_save = buff_index;

	// load new terminal data: key_buff, video memory, coordinates
	memcpy((uint8_t*)key_buff, terms[term_id].buff_save, (uint32_t)KEY_BUFF_SIZE);
	memcpy((uint8_t*)VIDEO, terms[term_id].vid_save, (uint32_t)VID_SIZE);
	set_x(terms[term_id].x_save);
	set_y(terms[term_id].y_save);
	num_enters = terms[term_id].enters_save;
	buff_index = terms[term_id].buff_index_save;

	curr_term_id = term_id;
	send_eoi(KEYBOARD_IRQ);

	//start shell execution if not already initialized
	if(!processes[curr_term_id]){
		//intialize shell id and remap
		processes[curr_term_id] = 1;
		create_user_4mb_page(term_id + 2, 32);
		reload_cr3();	

		pcb = get_pcb_by_PID(term_id);
		tss.esp0 = (uint32_t)(get_pcb_by_PID(term_id - 1) - 4);		//kernel stack pointer
		tss.ss0 = KERNEL_DS;			//kernal data segment = kernal stack segment	
		exec_term_id = term_id;
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
			: "r" (USER_DS), "r" (USER_CS), "r" (pcb->entry)
			: "edx" // clobbers %EDX
		);
	}
}

/* next_executing_term
 *
 * Description: moves to the next terminal for the scheduler
 * Inputs: None
 * Ouputs: None
 * Side Effects: Changes the currently executing terminal
 */
void next_executing_term() {
	while(1){
		exec_term_id++;
		if(exec_term_id > 2)
			exec_term_id = 0;
		if(processes[exec_term_id])
			return;
	}
}

/* terminal_write
 * Description: writes to current operating terminal
 * Inputs: fd: none
 		   buf: buffer to write to screen
 * Outputs: number of bytes read from buf
 * Side Effects: prints to screen
 */
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes) {
    int bytes_read;

	cli();
	if(buf != NULL){
        bytes_read = 0;
        //while ((((char*)buf)[bytes_read] != '\0') && bytes_read < nbytes) {
        while (bytes_read < nbytes) {
        	if(curr_term_id == exec_term_id)
            	putc(((char*)buf)[bytes_read]);								//print to display
            else
            	non_display_putc(((char*)buf)[bytes_read], exec_term_id);	//print to nondisplay buffer
            bytes_read++;
        }
		sti();
		return bytes_read;
	}
	sti();
	return 0;
}

/* terminal_read
 * Description: waits for enter. if enter has been pressed, pops first newline 
 *              terminated string from keyboard buffer and copies to buf.
 * Inputs: fd: none; buf: buffer to copy data to; nbytes: none
 * Outputs: Number of bytes copied to buf
 * Side Effects: None
 */
int32_t terminal_read (int32_t fd, void* buf, int32_t nbytes) {
	int i = 0;
	char temp[BUFF_SIZE];
	//while(exec_term_id != curr_term_id); blocks read until at current terminal
	while(1){
		cli();
		if(exec_term_id == curr_term_id && num_enters > 0)
			break;
		sti();
	}
	num_enters--;
	while((key_buff[i] != '\n') && (i < nbytes)){
		temp[i] = key_buff[i];
		i++;
	}
	shift_buffer(i, (uint8_t*)key_buff); //dequeue (discard read string)
	memcpy(buf, (void*)temp, i);
	return i;
}

/* terminal_open
 * Description: opens terminal
 * Inputs: filename
 * Outputs: 0 success 
 * Side Effects: None
 */
int32_t terminal_open (const uint8_t * filename){
	return 0;
}

/* terminal_close
 * Description: closes terminal
 * Inputs: fd - file descriptor
 * Outputs: 0 success
 * Side Effects: None
 */
int32_t terminal_close (int32_t fd) {
	return 0;
}


