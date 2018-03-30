/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

#define total_IRQ_ports		8 				// total number of irq_ports on a PIC
#define master_end 			7 				// IRQ ending value for master PIC
#define slave_end	 		15 				// IRQ ending value for slave PIC
#define slave_start 		8 				// IRQ starting value for slave PIC

/*
 * i8259_init
 *   DESCRIPTION:	Initializes both the master and slave PICs using the given 
 *					ICWs in the header file. We first mask out all interrupts 
 *					on the PIC and then we initialize the PIC with the slave 
 *					PIC set to IRQ 2 on the master PIC.
 *   INPUTS: 		none
 *   OUTPUTS:		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Initializes the PIC
 */
void i8259_init(void) {
	master_mask = 0xFF; 						// set mask for i8250 master 0x1111 1111
	slave_mask = 0xFF;							// set mask for i8250 slave  0x1111 1111

	outb(master_mask, MASTER_8259_PORT+1); 		// mask all of i8250 master
	outb(slave_mask, SLAVE_8259_PORT+1); 		// mask all of i8250 slave

	outb(ICW1, MASTER_8259_PORT);				// ICW1: select i8250 master init
	outb(ICW2_MASTER, MASTER_8259_PORT+1); 		// ICW2: i8250 master IR0-7 mapped to 0x20-0x27
	outb(ICW3_MASTER, MASTER_8259_PORT+1); 		// ICW3: i8250 master has slave in IR2
	outb(ICW4, MASTER_8259_PORT+1); 			// ICW4: i8250 auto EOI

	outb(ICW1, SLAVE_8259_PORT);  				// ICW1: select i8259 slave init
	outb(ICW2_SLAVE, SLAVE_8259_PORT+1); 		// ICW2: i8250 slave IR0-7 mapped to 0x28-0x2F
	outb(ICW3_SLAVE, SLAVE_8259_PORT+1); 		// ICW3: i8250 slave is on master's IR2
	outb(ICW4, SLAVE_8259_PORT+1);				// ICW4: i8250 auto EOI

	enable_irq(2);
}

/*
 * enable_irq
 *   DESCRIPTION:	Enables (unmasks the specified IRQ) number on the given PIC by
 *					first finding out which PIC the IRQ number belongs to and then 
 * 					unmasking the IRQ using an active low mask to enable that specific
 *					IRQ number.
 *   INPUTS: 		uint32_t irq_num: specifies which IRQ number we want to unmask
 *   OUTPUTS:		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	enables the specific irq number passed in
 */
void enable_irq(uint32_t irq_num) {

	/* If irq_num lies on master then set IRQ num to 0. */
	if(irq_num >= 0 && irq_num <= master_end) {
		master_mask = master_mask & ~(1 << irq_num);	// Enable specific IRQ num
		outb(master_mask, MASTER_8259_PORT+1);
	}

	/* If irq_num lies on slave then set IRQ num to 0. */
	else if(irq_num >= slave_start && irq_num <= slave_end) {
		irq_num -= total_IRQ_ports;
		slave_mask = slave_mask & ~(1 << irq_num); 		// Enable specific IRQ num
		outb(slave_mask, SLAVE_8259_PORT+1);
	}
}
/*
 * disable_irq
 *   DESCRIPTION:	Disables (masks the specified IRQ) number on the given PIC by
 *					first finding out which PIC the IRQ number belongs to and then 
 * 					masking the IRQ using an active low mask to enable that specific
 *					IRQ number.
 *   INPUTS: 		uint32_t irq_num: specifies which IRQ number we want to mask
 *   OUTPUTS:		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	disables the specific irq number passed in
 */
void disable_irq(uint32_t irq_num) {

	/* If irq_num lies on slave then set IRQ num to 1. */
	if(irq_num >= 0 && irq_num <= master_end) {
		master_mask = master_mask | (1 << irq_num);
		outb(master_mask, MASTER_8259_PORT+1);			// Disable specific IRQ num
	}

	/* If irq_num lies on slave then set IRQ num to 1. */
	else if(irq_num >= slave_start && irq_num <= slave_end) {
		irq_num -= total_IRQ_ports;
		slave_mask = slave_mask | (1 << irq_num);
		outb(slave_mask, SLAVE_8259_PORT+1); 			// Disable specific IRQ num
	}
}

/*
 * send_eoi
 *   DESCRIPTION:	Sends an end-of-interrupt signal for the specified IRQ passed in.
 * 					If on slave we must send EOI to slave port and port 2 on master.
 *   INPUTS: 		uint32_t irq_num: specifies which IRQ number to send eoi
 *   OUTPUTS:		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	sends eoi for specified IRQ
 */
void send_eoi(uint32_t irq_num) {

	/* If irq_num lies on slave. */
	if(irq_num >= slave_start && irq_num <= slave_end) {
		outb(EOI|(irq_num - total_IRQ_ports), SLAVE_8259_PORT);
		outb(EOI|2, MASTER_8259_PORT);  							// Slave is locatd on IRQ 2 on master
	}

	/* irq_num lies on master so OR EOI with interrrupt number. */
	else {
		outb(EOI|irq_num, MASTER_8259_PORT);
	}
}
