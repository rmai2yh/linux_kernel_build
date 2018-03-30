#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "lib.h"
#include "i8259.h"
#include "tests.h"
#include "terminal.h"
	
#define KEYBOARD_PORT 	0x60	//keyboard data port

#define KEYBOARD_IRQ 	1 		//irq num on pic
#define SINGLE_KEYS 	0x3A	//end of single character scancodes
#define LAST_PRESSED	0x58 	//last scancode of key pressed
#define BUFF_SIZE 128

/* Key buffer to hold all keys inputed. */
volatile char key_buff[BUFF_SIZE];
/* flag to keep track of number of enters */
volatile int num_enters;
/* initializes keyboard. */
extern void init_keyboard();
/* handle keyboard interrupt */
extern void keyboard_handler();
/* sets the buffer for keyboard inputs. */
void set_buffer(uint8_t key);
/* discard read string and shift */
void shift_buffer(int index, uint8_t * buf);

/* Key buffer index. */
volatile int buff_index;

#endif /* _KEYBOARD_H */
