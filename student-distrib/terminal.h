#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"
#include "lib.h"
#include "keyboard.h"
#include "syscall.h"

/* terminal struct values */
#define KEY_BUFF_SIZE 128
#define NUM_TERMINALS 3
#define NUM_COLS 80
#define NUM_ROWS 25
#define VID_SIZE NUM_ROWS * NUM_COLS * 2
/* update term cases */
#define IS_CLEAR 0
#define IS_ENTER 1
#define IS_BACKSPACE 2
#define IS_CHAR 3

/* struct for terminals: contains all data needed for switch */
typedef struct term_t{
	int x_save;
	int y_save;
	uint8_t buff_save[KEY_BUFF_SIZE];
	uint8_t vid_save[VID_SIZE]__attribute__((aligned (4096)));
	int enters_save;
	int buff_index_save;
}term_t;

//array of terminals
term_t terms[NUM_TERMINALS];
// pointer to current operating terminal
volatile int curr_term_id;
volatile int exec_term_id;

//initializes terminal
extern void init_terminal();
//switch operating terminal
extern void switch_displaying_term(int term_id);
//switch terminal executing code
extern void next_executing_term();


//writes to terminal
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes);
//reads from keyboard
int32_t terminal_read (int32_t fd, void* buf, int32_t nbytes);
//opens terminal
int32_t terminal_open (const uint8_t * filename);
//closes terminal
int32_t terminal_close (int32_t fd);

/* struct for each instance of terminal */




#endif /* TERMINAL_H*/
