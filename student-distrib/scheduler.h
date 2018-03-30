#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "lib.h"
#include "i8259.h"
#include "types.h"
#include "syscall.h"
#include "terminal.h"

#define		PIT_IRQ 	0
#define 	CHANNEL_0	0x40         //Channel 0 data port (read/write)
#define 	CHANNEL_1	0x41         //Channel 1 data port (read/write)
#define 	CHANNEL_2 	0x42         //Channel 2 data port (read/write)
#define 	CMD_REG		0x43         //Mode/Command register (write only)
#define 	MODE_3 		0x36 		 //		00 			11 			  011 	  0
									 //(channel 0)(lobyte/highbyte)(mode 3)(binary)
#define 	FREQ_10MILI 11932		 //HZ = 1193180/freq
#define 	FREQ_MASK	0xFF 		 //Mask for lower 8 bits
#define 	EIGHT 		8			 //Value of 8

#define 	NUM_TERMINALS 3			 //number of terminals

/* Initializes the PIT. */
void init_pit();
/* Code for pit interruption and handels scheduling. */
void pit_handler();

#endif /* _SCHEDULER_H */

