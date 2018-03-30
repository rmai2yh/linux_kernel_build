#ifndef _IDT_H
#define _IDT_H

#include "x86_desc.h"
#include "lib.h"  /* NOTE: lib.h contains useful functions like printf(); look here before including unnecessary libraries */
#include "interrupt_wrapper.h"
/* IDT INITIALIZER */
void init_idt();

/* HARDWARE INTERRUPT EXCEPTION HANDLER */
extern void interrupt_0();
extern void interrupt_1();
extern void interrupt_2();
extern void interrupt_3();
extern void interrupt_4();
extern void interrupt_5();
extern void interrupt_6();
extern void interrupt_7();
extern void interrupt_8();
extern void interrupt_9();
extern void interrupt_10();
extern void interrupt_11();
extern void interrupt_12();
extern void interrupt_13();
extern void interrupt_14();
extern void interrupt_15();
extern void interrupt_16();
extern void interrupt_17();
extern void interrupt_18();
extern void interrupt_19();
/*x86 manual does not include these for protected-mode
  exceptions and interrupts. source: ia-32 manual pg. 154
extern void interrupt_20();
extern void interrupt_21();
*/
#endif /* _IDT_H */
