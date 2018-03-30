#include "rtc.h"

/* Flag to tell us whether an interrupt is occuring. */
volatile int flags[NUM_TERMINALS] = {0, 0, 0};

/*
 * init_rtc
 *   DESCRIPTION:	Initializes the real time clock(RTC) to be 
 *   				able to handle RTC interrupts. This chip keeps 
 *					the computer's clock up to date. RTC interrupts 
 *					are disabled by default and we need to turn on 
 *					the RTC interrupts so that the RTC will periodically 
 *					generate IRQ 8.
 *   INPUTS: 		none
 *   OUTPUTS:		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Initializes the RTC
 */
void init_rtc() {
	/* Disable interrupts. */
	cli();

	/* Select Status Register A and disable NMI by setting 0x80 bit. */
	outb(RTC_REG_A, INDEX_PORT);

	outb(0x20, RW_PORT);

	/* Select register B and disable NMI. */
	outb(RTC_REG_B, INDEX_PORT); 	// select register B and disable NMI
	
	/* Read the current value of register B. */
	uint8_t prev = inb(RW_PORT); 			
	
	/* Set the index again (a read will reset the index to register B). */
	outb(RTC_REG_B, INDEX_PORT); 

	/* Write previous value | 0x40 to turn on bit 6 of register B. */
	outb((prev|0x40), RW_PORT);	

	/* Set base frequency to 2. */
	set_frequency(DEFAULT_FREQ);

	/* Enable IRQ line 8. */
	enable_irq(RTC_IRQ);

	/* Enable Interrupts. */
	sti();
}

/*
 * set_frequency
 *   DESCRIPTION:	Given a frequency, this function calculates the
 * 					interrupt rate without interfering with the RTC's
 *					ability to keep proper time. The rate is calculated
 * 					using the equation frequency = 32768 >> (rate - 1).
 *   INPUTS: 		int frequency: the given frequency to set the rate
 * 								   that the RTC generates interrupts
 *   OUTPUTS:		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Sets the interrupt rate of the RTC given a frequency
 */
void set_frequency(int32_t frequency) {
	uint8_t rate;

	/* Do not select frequency values that give us a rate of 1,2,3,4,5. */
	if(frequency == freq_1 || frequency == freq_2 || frequency == freq_3 || 
	   frequency == freq_4 || frequency == freq_5) return;

	else if(frequency == freq_6) 	rate = rate_6;
	else if(frequency == freq_7) 	rate = rate_7;
	else if(frequency == freq_8) 	rate = rate_8;
	else if(frequency == freq_9) 	rate = rate_9;
	else if(frequency == freq_10)	rate = rate_10;
	else if(frequency == freq_11) 	rate = rate_11;
	else if(frequency == freq_12)	rate = rate_12;
	else if(frequency == freq_13) 	rate = rate_13;
	else if(frequency == freq_14) 	rate = rate_14;
	else if(frequency == freq_15) 	rate = rate_15; 

	/* Rate must be above 2 and not over 15.*/
	rate &= 0x0F;

	/* Disable interrupts. */ 								
	cli();

	/* Set index to register A and disable NMI. */
	outb(RTC_REG_A, INDEX_PORT);

	/* Obtain initial value of register A. */
	uint8_t prev = inb(RW_PORT);

	/* Reset index to A. */
	outb(RTC_REG_A, INDEX_PORT);

	/* Write our rate at bottom 4 bits to A. */
	outb((prev & 0xF0) | rate, RW_PORT);

	/* Enable Interrupts. */
	sti();
}

/*
 * rtc_handler
 *   DESCRIPTION:	Called by the rtc_handler wrapper whenever a RTC
 *					interrupt occurs upon IRQ8. If register C is not
 *					read after an IRQ 8 then the interrupt will not
 *					happen again. Status register C will contain a bitmask
 *					telling which interrupt happened.
 *   INPUTS: 		none
 *   OUTPUTS:		none
 *   RETURN VALUE: 	none
 *   SIDE EFFECTS: 	Reads the status of register C to make sure any RTC
 *					interrupts that were pending before/while the RTC was
 *					initialized are acknowledged.
 */
void rtc_handler() {
	/* Disable interrupts. */
	cli();

	/* Select register C. */
	outb(RTC_REG_C, INDEX_PORT);

	/* Throw away contents. */
	inb(RW_PORT);

	send_eoi(RTC_IRQ);

	/* Set interrupt flag to 1. */
	flags[0] = 1;
	flags[1] = 1;
	flags[2] = 1;

	/* Enable Interrupts. */
	sti();
}

/*
 * rtc_open
 *   DESCRIPTION:	Open call to open access to file. In this case we just
 *					set the default frequency to 2 and always return 0.
 *   INPUTS: 		const uint8_t filename : contains the filename but is 
 *											 unused for this function (not used)
 *   OUTPUTS:		none
 *   RETURN VALUE: 	int32_t : 0 indicates sucess
 *   SIDE EFFECTS: 	Sets RTC interrupts to base frequency of 2
 */
int32_t rtc_open(const uint8_t * filename) {
    
    /* When opened set frequency to default. */
    set_frequency(DEFAULT_FREQ);

    /* Prove that rtc_open has been called. */
    printf("rtc_open called. \n");

    /* Always return 0. */
    return 0;
}

/*
 * rtc_read
 *   DESCRIPTION:	Read call that always returns 0 only after an interrupt has
 *					occured which is why we set a flag and waits until the interrupt
 *					handler clears it and then returns 0.
 *   INPUTS: 		const uint32_t fd : not used
 * 					void* buf : not used
 *					int32_t nbytes : not used
 *   OUTPUTS:		none
 *   RETURN VALUE: 	int32_t : 0 indicates sucess
 *   SIDE EFFECTS: 	Resets the RTC interrupt flag to 0
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    if(buf == NULL || nbytes<0)
    	return -1;
    /* Wait for interrupt to occur.  */
    while(flags[exec_term_id] == 0){
        //do nothing
    }
    /* Set interrupt flag to 0 to indicate interrupt is done. */
    flags[exec_term_id] = 0;

    /* Return success.  */
    return 0; 
}


/*
 * rtc_write
 *   DESCRIPTION:	This call can only accept a 4 byte integer specifying the
 *					interrupt rate in Hz and then sets the rate of periodic
 *					interrupts accordingly. Only allow interrupts at rate 1024
 * 					or lower.
 *   INPUTS: 		const uint32_t fd : not used
 *					const void* buf : pointer to frequency
 *					int32_t nbytes : number of bytes in integer
 *   OUTPUTS:		none
 *   RETURN VALUE: 	int32_t : 0 indicates sucess
 * 							 -1 indicates failure
 *   SIDE EFFECTS: 	Sets RTC interrupt frequency to desired frequency
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    
    /* Check if nbytes is a 4 byte integer and if buff is NULL. */
    if (nbytes != NUM_BYTES || buf == NULL) {
        return -1;
    }

    /*Set RTC rate to given frequency. */
    set_frequency(*((int32_t*)buf));

    /* Return the number of bytes written. */
    return nbytes;
}

/*
 * rtc_close
 *   DESCRIPTION:	RTC interupts simply remains on at all times
 *   INPUTS: 		const uint32_t fd : not used
 *   OUTPUTS:		none
 *   RETURN VALUE: 	int32_t : 0 indicates sucess
 *   SIDE EFFECTS: 	Set RTC interrupt rate to default frequency
 */
int32_t rtc_close(int32_t fd) {

	/* Set RTC to default rate of 2 interrupts per second. */
	set_frequency(DEFAULT_FREQ);

	/* Always return sucess. */
    return 0;
}



