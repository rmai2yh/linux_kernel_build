#include "keyboard.h"

/* MAP: SCANCODE TO ASCII when no modifier is pressed. */
static const char no_modifier[60] =
    {'\0', '\0', '1', '2', '3', '4', '5', 
    '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
    'p', '[', ']', '\0', '\0', 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l' , ';', 
    '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v', 
    'b', 'n', 'm',',', '.', '/', '\0', '*', '\0', ' ', '\0'};

/* MAP: SCANCODE TO ASCII when shift key is pressed. */
static const char shift_pressed[60] =
{'\0', '\0', '!', '@', '#', '$', '%', '^',
 '&', '*', '(', ')', '_', '+', 0x08, '\0',
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\0', '\0', 'A', 'S',
   'D', 'F', 'G', 'H', 'J', 'K', 'L' , ':',
    '"', '~', '\0', '|', 'Z', 'X', 'C', 'V', 
   'B', 'N', 'M', '<', '>', '?', '\0', '*',
    '\0', ' ', '\0'};

/* MAP: SCANCODE TO ASCII when caps key is pressed. */
static const char caps_pressed[60] = 
{'\0', '\0', '1', '2', '3', '4', '5', '6', '7',
 '8', '9', '0', '-', '=', '\0', '\0',
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O',
   'P', '[', ']', '\0', '\0', 'A', 'S',
  'D', 'F', 'G', 'H', 'J', 'K', 'L' , ';', '\'',
   '`', '\0', '\\', 'Z', 'X', 'C', 'V', 
  'B', 'N', 'M', ',', '.', '/', '\0', '*', '\0', ' ', '\0'};

/* MAP: SCANCODE TO ASCII when caps and shift are pressed. */
static const char caps_shift_pressed[60] =
{'\0', '\0', '!', '@', '#', '$', '%', '^', '&',
 '*', '(', ')', '_', '+', '\0', '\0',
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
    'p', '{', '}', '\0', '\0', 'a', 's',
   'd', 'f', 'g', 'h', 'j', 'k', 'l' , ':', '"',
    '~', '\0', '\\', 'z', 'x', 'c', 'v', 
   'b', 'n', 'm', '<', '>', '?', '\0', '*', '\0',
    ' ', '\0'};
/* Flag to indicate if Shift left key is pressed. */
volatile unsigned long SHIFT_LEFT_FLAG = 0;
/* Flag to indicate if Shift right key is pressed. */
volatile unsigned long SHIFT_RIGHT_FLAG = 0;
/* Flag to indicate if Caps lock key is pressed. */
volatile unsigned long CAPS_LOCK_FLAG = 0;
/* Flag to indicate if Control key is pressed. */
volatile unsigned long CTRL_FLAG = 0;
/*Flag to indicate if Alt key is pressed. */
volatile uint8_t alt_flag = 0;

volatile int prev_buff;

#define LEFT_SHIFT_ON    0x2A
#define LEFT_SHIFT_OFF   0xAA
#define RIGHT_SHIFT_ON   0x36
#define RIGHT_SHIFT_OFF  0xB6
#define CAPS_ON          0x3A
#define CAPS_OFF         0xBA  
#define ENTER            0x1C
#define BACKSPACE        0x0E
#define CONTROL_LEFT_ON
#define CONTROL_LEFT_OFF 
#define ALT_L            0x38
#define ALT_R            0xE0
#define ESC              0x01
#define TAB              0x0F
#define P_SCREEN0        0x2A
#define P_SCREEN1        0x37
#define F1               0x3B
#define F2               0x3C
#define F3               0x3D
#define ALT_RELEASE      0xB8


#define l_on 			   0x26			
#define CTRL_ON 	 	 0x1D
#define CTRL_OFF 		 0x9D

#define KNOWN_CODES 0x3B //known scan codes 
/* init_keyboard 
 * Description: Enables interrupt requests at KEYBOARD_IRQ
 *              on pic
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 */
void init_keyboard() {
  int i;
  buff_index = 0;
  num_enters = 0;
	enable_irq(KEYBOARD_IRQ);
  for(i = 0; i < BUFF_SIZE; i++)
    key_buff[i] = '\0';
}

/* keyboard_handler 
 * Description: Called by keyboard wrapper
 *              Handles keypress (prints key if lowercase letter or number)
 * Inputs: None
 * Outputs: None
 * Side Effects: reads from keyboard_port, sends eoi
 */
void keyboard_handler() {
    uint8_t key = 0;
    key = inb(KEYBOARD_PORT);
    switch(key) {
    	case LEFT_SHIFT_ON:
        	SHIFT_LEFT_FLAG = 1;
        	break;
      	case RIGHT_SHIFT_ON:
        	SHIFT_RIGHT_FLAG = 1;
        	break;
     	case LEFT_SHIFT_OFF:
      		SHIFT_LEFT_FLAG = 0;
      	 	break;
      	case RIGHT_SHIFT_OFF:
        	SHIFT_RIGHT_FLAG = 0;
        	break;
      	case ENTER:
          if(buff_index < BUFF_SIZE) {
            key_buff[buff_index] = '\n';
            buff_index++;
            printf("%c", '\n');  
            num_enters++;   
          }
          break;
      	case CTRL_ON:
      		CTRL_FLAG = 1;
      		break;
      	case CTRL_OFF:
      		CTRL_FLAG = 0;
      		break;
      	case BACKSPACE:
          //shift index left and delete previously pressed key
      		buff_index--;
          if(buff_index < 0)
            buff_index = 0;          
          if(key_buff[buff_index] != '\0')
            print_backspace();
          key_buff[buff_index] = '\0';
          break;
     	  case CAPS_ON:
        	CAPS_LOCK_FLAG ^= 1;
        	break;
        case ALT_L:
          alt_flag = 1;
          break;
        case ALT_R:
          alt_flag = 1;
          break;
        case ALT_RELEASE:
          alt_flag = 0;
          break;
      	default:
      		set_buffer(key);
      		break;
    }
    send_eoi(KEYBOARD_IRQ);
}

/* set_buffer 
 * Description: Called by keyboard_handler
 *              sets buffer according to key pressed
 * Inputs: scancode of key
 * Outputs: None
 * Side Effects: changes keyboard buffer
 */
void set_buffer(uint8_t key) {
	if(CTRL_FLAG && key == l_on) {
    	clear();
      return;
  }
  if((key >= F1) && (key <= F3)){
    if(alt_flag){
      cli();
      switch_displaying_term(key - F1);
      sti();
    }
    return;
  }
  //make sure not unkown scancode
  if((key == TAB) || (key == ESC) || (key == P_SCREEN0) || (key == P_SCREEN1))
    return;
    if(buff_index < BUFF_SIZE - 1) {
	    /* If caps lock and shift are pressed then print correct mapping. */
	    if(CAPS_LOCK_FLAG && (SHIFT_RIGHT_FLAG || SHIFT_LEFT_FLAG) && (key < KNOWN_CODES)) {
	    	key_buff[buff_index] = caps_shift_pressed[key];
	    	printf("%c", key_buff[buff_index]);
	    	buff_index++;
	    }
	    /* If shift is pressed then print correct mapping. */
	    else if((SHIFT_LEFT_FLAG || SHIFT_RIGHT_FLAG) && (key < KNOWN_CODES)) {
	    	key_buff[buff_index] = shift_pressed[key];
	    	printf("%c", key_buff[buff_index]);
	    	buff_index++;
	    }
	    /* If capslock is on then print correct mapping. */
	    else if(CAPS_LOCK_FLAG && key < KNOWN_CODES) {
	    	key_buff[buff_index] = caps_pressed[key];
	    	printf("%c", key_buff[buff_index]);
	    	buff_index++;
	    }
	    /* If no modifiers are on then just print regular mapping. */
	    else if(key < KNOWN_CODES) {
	    	key_buff[buff_index] = no_modifier[key];
			  printf("%c", key_buff[buff_index]);
	    	buff_index++;
	   	}
	}
}

/* shift_buffer
 * Description: Called by terminal_read
 *              Clears buffer up to the new line character. Then discards
 *              read text by shifting left.
 * Inputs: index (clears up to this index)
 * Outputs: None
 * Side Effects: changes buffer
 */
void shift_buffer(int index, uint8_t * buf){
  int i;
  // clear buffer up to index
  for(i = 0; i <= index; i++)
    buf[i] = '\0';
  //shift left
  memcpy((void*)buf, (void*)&buf[i+1], BUFF_SIZE-index-1);
  //update index
  if(exec_term_id == curr_term_id)
    buff_index -= (index+1);
  else
    terms[exec_term_id].buff_index_save -= (index+1);
}
