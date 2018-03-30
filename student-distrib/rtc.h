#ifndef _RTC_H
#define _RTC_H

#include "lib.h"
#include "terminal.h"
#include "syscall.h"
#include "i8259.h"
#include "tests.h"

#define DEFAULT_FREQ 	2 			// Default frequency is 2 hertz
#define NUM_BYTES		4			// Number of bytes we always want for RTC
#define RTC_IRQ			8			// IRQ number
#define INDEX_PORT 		0x70 		// used to specify and index or register number
#define RW_PORT 		0x71 		// used to read or write from/to byte of CMOS
#define RTC_REG_A 		0x8A 		// RTC Status Register A in CMOS RAM
#define RTC_REG_B 		0x8B 		// RTC Status Register B in CMOS RAM
#define RTC_REG_C 		0x8C		// RTC Status Register C in CMOS RAM
#define RTC_REG_D		0x8D		// RTC Status Register D in CMOS RAM
#define NMI_INDEX 		0x80		// NMI(non-maskable interrupr)
#define freq_1			32768 		// freqency value to set rate to none
#define freq_2			16384 		// freqency value to set rate to none
#define freq_3			8192		// freqency value to set rate to 3
#define freq_4			4096		// freqency value to set rate to 4
#define freq_5			2048		// freqency value to set rate to 5
#define freq_6			1024		// freqency value to set rate to 6
#define freq_7			512			// freqency value to set rate to 7
#define freq_8			256			// freqency value to set rate to 8
#define freq_9			128			// freqency value to set rate to 9
#define freq_10			64			// freqency value to set rate to 10
#define freq_11			32			// freqency value to set rate to 11
#define freq_12			16			// freqency value to set rate to 12
#define freq_13			8			// freqency value to set rate to 13
#define freq_14			4			// freqency value to set rate to 14
#define freq_15			2			// freqency value to set rate to 15
#define rate_3 			0x3 		// rate = 3
#define rate_4 			0x4 		// rate = 4
#define rate_5 			0x5 		// rate = 5
#define rate_6 			0x6 		// rate = 6
#define rate_7 			0x7 		// rate = 7
#define rate_8 			0x8 		// rate = 8
#define rate_9 			0x9 		// rate = 9
#define rate_10 		0xA 		// rate = 10
#define rate_11 		0xB 		// rate = 11
#define rate_12 		0xC	 		// rate = 12
#define rate_13 		0xD 		// rate = 13
#define rate_14 		0xE 		// rate = 14
#define rate_15 		0xF 		// rate = 15

/* kernel.c calls this to initialize the rtc. */
extern void init_rtc();
/* called by wrapper when rtc interrupt has occured. */
extern void rtc_handler();
/* sets the rate given a specified frquency. */
void set_frequency(int32_t frequency);
/* Opens the RTC. */
int32_t rtc_open(const uint8_t * filename);
/* Reads the RTC. */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
/* Writes the RTC. */   
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
/* Closes the RTC. */    
int32_t rtc_close(int32_t fd);

#endif /* _RTC_H */
