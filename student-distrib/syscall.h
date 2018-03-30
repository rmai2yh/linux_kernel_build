#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "lib.h"
#include "paging.h"
#include "tasks.h"
#include "terminal.h"
#include "types.h"
#include "fs.h"
#include "x86_desc.h"
#include "rtc.h"

#define FD_ARRAY_LEN 8                   // file descriptor array length
#define MAX_CMD_SIZE 128                 // max size of a command 

#define _8MEGA	0x800000 		//8mb
#define _8KILO	0x1000			//4kb
#define ALIGN_4MB 22

#define MAX_NUM_PROCESSES 7

/* Terminates a process and returns specific value to parent process. */    
int32_t halt (uint8_t status);
/* Loads and executes a new program. */
int32_t execute (const uint8_t* command);
/* Reads data from keyboard, a file, RTC, or directory. */
int32_t read (int32_t fd, void* buf, int32_t nbytes);
/* Writes data to terinal or a device(RTC). */
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
/* Provides access to a file system. */
int32_t open (const uint8_t* filename);
/* Closes a specified file descriptor and makes it avaliable for return from later calls to open. */
int32_t close (int32_t fd);
/* Reads the program's command line arguments into a user-level buffer. */
int32_t getargs (uint8_t* buf, int32_t nbytes);
/* Maps the text-mode video memory into user space at a pre-set virtual address. */
int32_t vidmap (uint8_t** screen_start);
/* Signal handling. */
int32_t set_handler (int32_t signum, void* handler_address);
/* Signal handling. */
int32_t sigreturn (void);


/* loads 3 shells */
void boot();

typedef int32_t (*open_t)(const uint8_t* filename);
typedef int32_t (*read_t)(int32_t fd, void* buf, int32_t nbytes);
typedef int32_t (*write_t)(int32_t fd, const void* buf, int32_t nbytes);
typedef int32_t (*close_t)(int32_t fd);

/* The structure for the file operations jump table containing open, read, write, close. */
typedef struct fo_jump_table_t {
    open_t open;
    read_t read;
    write_t write;
    close_t close;
} fo_jump_table_t;


/* The structure for an entry in a file descriptor table. */
typedef struct fd_entry_t {
    fo_jump_table_t* fo_jump_table_ptr;         // Pointer to the jump table allocated for this entry
    uint32_t inode_index;                       // Index of the inode, 0 if not a file
    uint32_t file_position;                     // Position in the file that's been read
    union {
        uint32_t flags;                         // bit 0 is whether or not the fd is in use
        struct {
            uint32_t active       :1;           // Is fd active or not
            uint32_t reserved     :31;          // reserved for future use
        } __attribute__ ((packed));
    };
} fd_entry_t;

/* Structure for the PCB. */
typedef struct pcb_t {
	fd_entry_t fd_array[FD_ARRAY_LEN];         //file descriptor array
    uint8_t filenames[8][32];                  //filenames for convenience
    uint8_t process_id;                        //this process's id from 0 to 8
    uint32_t parent_ebp;                       //this process's base pointer
    uint32_t parent_esp;                       //this process's stack pointer
    struct pcb_t * parent_pcb;                 //pointer to process's parent pcb
    struct pcb_t * child_pcb;                  //pointer to process's child pcb
    uint8_t args[MAX_CMD_SIZE];
    uint32_t return_ebp;
    uint32_t return_esp;
    uint32_t entry;
} pcb_t;

/* Obtains the PCB given a specified process ID. */
pcb_t* get_pcb_by_PID(int PID);
/* returns a pointer to the PCB of the current task */
pcb_t* get_current_executing_pcb();
/* returns a pointer to teh PCB of the current displaying task */
pcb_t* get_current_displaying_pcb();

/* array of active process ids; active high */
uint8_t processes[MAX_NUM_PROCESSES];

#endif /* _SYSCALL_H */
