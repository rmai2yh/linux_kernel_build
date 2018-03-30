#ifndef _INTERRUPT_WRAPPER_H
#define _INTERRUPT_WRAPPER_H

#include "keyboard.h"
#include "rtc.h"
#include "syscall.h"
#include "scheduler.h"

//calls keyboard_handler with iret
void keyboard_wrapper();
//calls rtc_handler with iret
void rtc_wrapper();
//calls syscall_handler with iret
void syscall_wrapper();
//calls pit_handler with iret
void pit_wrapper();

#endif /* _INTERRUPT_WRAPPER_H */
