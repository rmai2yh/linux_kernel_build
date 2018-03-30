#include "scheduler.h"
#define USER_PD_INDEX 32

/*
 * init_pit
 *   DESCRIPTION: 	Initializes the PIT (programable interrupt handler). We first set to PIT
 * 					mode 3 by sending it to the command register. We then send the low 8 bits of
 *					the 10 milisecond frequency and then the high 8 bits of the 10 milisecond
 *					frequency to channel 0. After all that we then enable the irq line of 0 for the
 * 					PIT.
 *   INPUTS: 		none
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Initializes the PIT
 */
void init_pit() {

	/* Disable interrupts. */
	cli();

	/* Set to mode 3 square wave generator. */
	outb(MODE_3, CMD_REG);

	/* Send and set lower 8 bits of freqency to 100HZ = 10 miliseconds. */
	outb(FREQ_10MILI&FREQ_MASK, CHANNEL_0);

	/* Send upper 8 bits of frequency. */
	outb(FREQ_10MILI >> EIGHT, CHANNEL_0);

	/* Enable IRQ line 0. */
	enable_irq(PIT_IRQ);

	/* Enable interrupts. */
	sti();
}

/*
 * pit_handler
 *   DESCRIPTION: 	Obtains the next executing user program on each next terminal window,
 *					creates a user 4mb page and then remaps the executing terminal. After
 * 					that we update the tss, ebp, and esp in order to take advantage of the
 *					context switch.
 *   INPUTS: 		none
 *   OUTPUTS: 		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Effectivly sets up a "round robin" to execute programs on all three
 *					terminals every 10 miliseconds.
 */
void pit_handler() {
	send_eoi(PIT_IRQ);

	//get curr_pcb
	pcb_t* curr_pcb = get_current_executing_pcb();
	//remap video to nondisplay
	// remap_vid(exec_term_id);
	// reload_cr3();

	//save ebp and esp
	asm volatile("			\n\
		movl %%ebp, %0 		\n\
		movl %%esp, %1 		\n\
		"
		: "=g" (curr_pcb->return_ebp), "=g" (curr_pcb->return_esp)
	);

	//get new exec_term_id and pcb
	next_executing_term();
	curr_pcb = get_current_executing_pcb();
	//remap user program page
	create_user_4mb_page(curr_pcb->process_id + 2, USER_PD_INDEX);
	reload_cr3();
	//remap video to nondisplay
	remap_vid(exec_term_id);
	reload_cr3();

	//save esp0 and kernel stack segment
	tss.esp0 = (uint32_t)(get_pcb_by_PID(curr_pcb->process_id - 1)) - 4;
	tss.ss0 = KERNEL_DS;

	//return to next function
	asm volatile("			\n\
		movl %0, %%ebp 		\n\
		movl %1, %%esp 		\n\
		leave				\n\
		ret 				\n\
		"
		:
		: "r" (curr_pcb->return_ebp), "r" (curr_pcb->return_esp)
	);
	
	return;	
}


