#include "idt.h"
#include "syscall.h"
#include "terminal.h"   //TODO:REMOVE

/* IDT VECTOR CONSTANTS see idt given in lecture */
#define SYSCALL_VECTOR 0x80
#define KEYBOARD_VECTOR 0x21
#define RTC_VECTOR 0x28
#define PIT_VECTOR 0x20
/* 
This function will initialize the IDT. 
The structure for a descriptor entry is given in the file "x86_desc.h". It is as follows-
 typedef union idt_desc_t {
	 uint32_t val[2];
	 struct {
		 uint16_t offset_15_00;
		 uint16_t seg_selector;
		 uint8_t  reserved4;
		 uint32_t reserved3 : 1;
		 uint32_t reserved2 : 1;
		 uint32_t reserved1 : 1;
		 uint32_t size      : 1;
		 uint32_t reserved0 : 1;
		 uint32_t dpl       : 2;
		 uint32_t present   : 1;
		 uint16_t offset_31_16;
	 } __attribute__ ((packed));
 } idt_desc_t;
 
 The total number of entries in the table is 256, as defined by NUM_VEC in the file "x86_desc.h".
 idt[NUM_VEC] is a the table with 256 entries, and we have to initialize the entries based on the struct present above.
 */

/* init_idt 
 * Description: initializes idt table. 
 * Inputs: None
 * Outputs: None
 * Side Effects: Fills idt with exception handlers and device vectors
 */
void init_idt(){
    int i;  //general loop

    /* LOAD IDT POINTER AND SIZE INTO IDTR */
    /* function and pointer provided by x86_desc.h */
 	lidt(idt_desc_ptr);
    
    for (i=0; i<NUM_VEC; i++){
        
        idt[i].seg_selector = KERNEL_CS;    //Not sure, the different values are defined in "x86_desc.h", line 6.
        
        /* see ia-32 pg. 156 and http://wiki.osdev.org/Interrupt_Descriptor_Table */
        idt[i].reserved4 = 0;
        idt[i].reserved3 = 0;
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1;
        idt[i].size = 1;
        idt[i].reserved0 = 0;
        
        if(SYSCALL_VECTOR == i)
            idt[i].dpl = 3;     //Descriptor privelage level, set to 3 only for syscalls
        
        else
            idt[i].dpl = 0;     //Other interrupts all hardware level
        
        idt[i].present = 1;      //The present bit is set to 1 for working interrupts.
    }

        //This sets the runtime parameters for an entry, i.e. offset_15_00 and offset_31_16. See file "x86_desc.h", line 171 for description.
        
        /* FILL IN 0 - 21 HARDWARE INTERRUPTS WITH FUNCTION POINTERS */
    	/* NOTE: SET_IDT_ENTRY() provided by x86_desc.h */
        SET_IDT_ENTRY(idt[0], interrupt_0);
        SET_IDT_ENTRY(idt[1], interrupt_1);
        SET_IDT_ENTRY(idt[2], interrupt_2);
        SET_IDT_ENTRY(idt[3], interrupt_3);
        SET_IDT_ENTRY(idt[4], interrupt_4);
        SET_IDT_ENTRY(idt[5], interrupt_5);
        SET_IDT_ENTRY(idt[6], interrupt_6);
        SET_IDT_ENTRY(idt[7], interrupt_7);
        SET_IDT_ENTRY(idt[8], interrupt_8);
        SET_IDT_ENTRY(idt[9], interrupt_9);
        SET_IDT_ENTRY(idt[10], interrupt_10);
        SET_IDT_ENTRY(idt[11], interrupt_11);
        SET_IDT_ENTRY(idt[12], interrupt_12);
        SET_IDT_ENTRY(idt[13], interrupt_13);
        SET_IDT_ENTRY(idt[14], interrupt_14);
        SET_IDT_ENTRY(idt[15], interrupt_15);
        SET_IDT_ENTRY(idt[16], interrupt_16);
        SET_IDT_ENTRY(idt[17], interrupt_17);
        SET_IDT_ENTRY(idt[18], interrupt_18);
        SET_IDT_ENTRY(idt[19], interrupt_19);

        /* SET SYSCALL ENTRY */
        SET_IDT_ENTRY(idt[SYSCALL_VECTOR], syscall_wrapper);

        /* SET PIC DEVICE ENTRIES */
        SET_IDT_ENTRY(idt[KEYBOARD_VECTOR], keyboard_wrapper);
        SET_IDT_ENTRY(idt[RTC_VECTOR], rtc_wrapper);
		SET_IDT_ENTRY(idt[PIT_VECTOR], pit_wrapper);
}


/* BEGIN HARDWARE INTERRUPT HANDLERS FUNCTIONS */
void interrupt_0() {
    printf("Divide Error Exception. \n");
    while(1)halt(255);		//halt
}

void interrupt_1() {
    printf("Debug Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_2() {
    printf("NMI Interrupt. \n");
    while(1)halt(255);      //halt
}

void interrupt_3() {
    printf("Breakpoint Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_4() {
    printf("Overflow Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_5() {
    printf("BOUND Range Exceeded Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_6() {
    printf("Invalid Opcode Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_7() {
    printf("Device Not Available Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_8() {
    printf("Double Fault Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_9() {
    printf("Coprocessor Segment Overrun. \n");
    while(1)halt(255);      //halt
}

void interrupt_10() {
    printf("Invalid TSS Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_11() {
    printf("Segment Not Present. \n");
    while(1)halt(255);      //halt
}

void interrupt_12() {
    printf("Stack Fault Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_13() {
    printf("General Protection Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_14() {
    printf("Page-Fault Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_15() {
	printf("Test failed. \n");
    while(1)halt(255);      //halt
}

void interrupt_16() {
    printf("x87 FPU Floating-Point Error. \n");
    while(1)halt(255);      //halt
}

void interrupt_17() {
    printf("Alignment Check Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_18() {
    printf("Machine-Check Exception. \n");
    while(1)halt(255);      //halt
}

void interrupt_19() {
    printf("SIMD Floating-Point Exception. \n");
    while(1)halt(255);      //halt
}
/*
void interrupt_20() {
    printf("Virtualization Exception. \n");
    while(1);	//Enter infinite loop
}

void interrupt_21() {
    printf("Control Protection Exception. \n");
    while(1);	//Enter infinite loop
}
*/

/* END HARDWARE INTERRUPT HANDLERS FUNCTIONS */

