/* paging.c - initialization of the paging hardware
 * vim:ts=4 noexpandtab
 */

#include "paging.h"
#include "tasks.h"
#include "lib.h"

#define PAGE_TABLE_SIZE 1024

/* forward declarations of functions private to this file */
static void clear_page_directory_table();
static void kernel_paging_init();
static void vga_paging_init();
static void setup_control_registers();

/* init_paging
 * Initializes the Intel x86 paging hardware
 * Assigns kernel space to its location in static memory (4MB to 8MB)
 * Assigns VGA memory to a 4K page in static memory (0x0B800000)
 * All other pages should be marked not present
 * Inputs: None
 * Returns: None
 */
void init_paging() {
    // clears everything, inits the kernel and VGA mem, then 
    // turns on paging using the control registers
    clear_page_directory_table();
    kernel_paging_init();
    vga_paging_init();
	setup_control_registers();
    active_tasks = 0;
}

/* clear_page_directory_table
 * Marks all entries of the page directory and page table as not present
 * Inputs: None
 * Returns: None
 */
static void clear_page_directory_table() {
    int i;
    for(i = 0; i < PAGE_TABLE_SIZE; i++) {
        // marks every page directory and page table entry as not present
		page_directory[i].present = 0;
        page_table_0[i].present = 0;
    }
}

/* kernel_paging_init
 * Initializes the page directory to have the kernel's virtual address
 * exactly match its physical address in memory as a 4MB page
 * Inputs: None
 * Returns: None
 */
static void kernel_paging_init() {
    /* Since this is a 4MB page, we only need to define
     * an entry in the page directory.  The 10 address bits
     * (top 10 of the 20 bits provided in our union) should be set
     * to 0b0000000001 since this is the second 4MB segment of physical memory */
    page_directory[1].addr = 0x400; // For 4MB, the 9 reserved bits and the PAT will be set to zero
    page_directory[1].avail = 0;
    page_directory[1].global_page = 1; // Kernel is a global page
    page_directory[1].size = 1; // The size bit is 1 for a 4MB page
    page_directory[1].reserved_dirty = 0;
    page_directory[1].accessed = 0;
    page_directory[1].cache_disabled = 0;
    page_directory[1].write_through = 0;
    page_directory[1].privilege_level = 0; // Kernel level priv
    page_directory[1].rw = 1; // Set as read/write
    page_directory[1].present = 1;  // Mark this page as present
}

/* vga_paging_init
 * Initializes the page directory at index 0 to point to a page table that is
 * entirely empty except for a single 4K page that directly corresponds to the
 * physical address of the VGA memory (0x0B800000)
 * Inputs: None
 * Returns: None
 */
static void vga_paging_init() {
    /* video memory is at 0x000B8000
       page dir lookup: 0x0
       page table lookup: 0x0B8 */

    // Fill a page table entry that points to the location of VGA mem in phys mem
    page_table_0[0x0B8].addr = 0x000B8; // point to same location in phys mem
    page_table_0[0x0B8].avail = 0;
    page_table_0[0x0B8].global_page = 0;
    page_table_0[0x0B8].pat_index = 0;
    page_table_0[0x0B8].dirty = 0;
    page_table_0[0x0B8].accessed = 0;
    page_table_0[0x0B8].cache_disabled = 0;
    page_table_0[0x0B8].write_through = 0;
    page_table_0[0x0B8].privilege_level = 0; // User level priv
    page_table_0[0x0B8].rw = 1; // set as read write
    page_table_0[0x0B8].present = 1; // Mark this page as present

	// Fill a page directory entry that points to the above page table
	// take the 20 high bits of the page table entry address
	page_directory[0].addr = (((uint32_t)page_table_0) & 0xFFFFF000) >> 12;
	page_directory[0].avail = 0;
	page_directory[0].global_page = 0;
	page_directory[0].size = 0; // mark as 4kb page size
	page_directory[0].reserved_dirty = 0; // always 0 for 4kb pages
	page_directory[0].accessed = 0;
	page_directory[0].cache_disabled = 0;
	page_directory[0].write_through = 0;
	page_directory[0].privilege_level = 0; // User level priv
	page_directory[0].rw = 1; // set as read write
	page_directory[0].present = 1; // Mark this page as present
}
 
/* create_user_4mb_page
 *
 * Marks the page directory entry at index virt_index as
 * present, sets it up as a user page of 4MB size, and points
 * it to 4MB of real memory at real_index * 4MB
 *
 * Inputs: real_index -- index of the page in real memory, 2-1023
 *         virt_index -- index of the page in virt memory, 2-1023
 * Returns: 0 for success, -1 for error
 * Side effects: Updates page directory
 */
int create_user_4mb_page(int real_index, int virt_index) {
	// OOB checks.  indexes <2 are used for the kernel
	if (real_index < 2 || virt_index < 2 || real_index >= PAGE_TABLE_SIZE || virt_index >= PAGE_TABLE_SIZE)
		return -1;

	// Setup the page directory entry
	page_directory[virt_index].addr = real_index << 10; // index of 4MB page with 10 0's for reserved and PAT
	page_directory[virt_index].avail = 0;
	page_directory[virt_index].global_page = 0;
	page_directory[virt_index].size = 1; // The size bit is 1 for a 4MB page
	page_directory[virt_index].reserved_dirty = 0;
	page_directory[virt_index].accessed = 0;
	page_directory[virt_index].cache_disabled = 0;
	page_directory[virt_index].write_through = 0;
	page_directory[virt_index].privilege_level = 1; // User level priv
	page_directory[virt_index].rw = 1; // Set as read/write
	page_directory[virt_index].present = 1;  // Mark this page as present
	return 0;
}

/* create_vid_4kb_page
 *
 * Marks the page directory entry at index virt_index as
 * present, sets it up as a user page of 4kb size, and points
 * it to  of real memory at real_index * 4MB
 *
 * Inputs: None
 * Returns: virtual address of new page for video
 * Side effects: Updates page directory
 */
uint32_t create_vid_4kb_page() {
    // Fill a page directory entry that points to the above page table
    // take the 20 high bits of the page table entry address
    page_directory[VID_MAP_VIRTUAL_INDEX].addr = (((uint32_t)vid_page_table_0) & 0xFFFFF000) >> 12;
    page_directory[VID_MAP_VIRTUAL_INDEX].avail = 0;
    page_directory[VID_MAP_VIRTUAL_INDEX].global_page = 0;
    page_directory[VID_MAP_VIRTUAL_INDEX].size = 0; // The size bit is 0 for a 4kB page
    page_directory[VID_MAP_VIRTUAL_INDEX].reserved_dirty = 0;
    page_directory[VID_MAP_VIRTUAL_INDEX].accessed = 0;
    page_directory[VID_MAP_VIRTUAL_INDEX].cache_disabled = 0;
    page_directory[VID_MAP_VIRTUAL_INDEX].write_through = 0;
    page_directory[VID_MAP_VIRTUAL_INDEX].privilege_level = 1; // User level priv
    page_directory[VID_MAP_VIRTUAL_INDEX].rw = 1; // Set as read/write
    page_directory[VID_MAP_VIRTUAL_INDEX].present = 1;  // Mark this page as present

    // Fill a page table entry that points to the location of VGA mem in phys mem
    vid_page_table_0[0].addr = 0x000B8; // point to same location in phys mem (0x000B8)
    vid_page_table_0[0].avail = 0;
    vid_page_table_0[0].global_page = 0;
    vid_page_table_0[0].pat_index = 0;
    vid_page_table_0[0].dirty = 0;
    vid_page_table_0[0].accessed = 0;
    vid_page_table_0[0].cache_disabled = 0;
    vid_page_table_0[0].write_through = 0;
    vid_page_table_0[0].privilege_level = 1; // User level priv
    vid_page_table_0[0].rw = 1; // set as read write
    vid_page_table_0[0].present = 1; // Mark this page as present
    return VID_MAP_VIRTUAL_INDEX << 22; //<< 22 to get to 32 bit 4mb alligned address
}

/* remap_vid
 *
 * Description: If our executing terminal is our current terminal then we want to remap our executing terminal
 *              to our physical display. Otherwise we remap our executing terminal into our nondisplay buffer.
 *
 * Inputs: int exec_term_id : the terminal being currently executed.
 * Returns: none
 * Side effects: Remaps executing terminal to physical display or the nondisplay buffer. 
 */
void remap_vid(int exec_term_id) {
    if(exec_term_id == curr_term_id)
        vid_page_table_0[0].addr = 0x000B8; // remap to physical display (xB8000)
    else{
        vid_page_table_0[0].addr = ((uint32_t)(terms[exec_term_id].vid_save) & 0xFFFFF000) >> 12; // remap to nondisplay buffer (shift 12 for 4k aligned)
    }
}

/* setup_contorl_registers
 * Pushes the address of the page directory to cr3,
 * turns on 4M pages, and then turns on paging
 * NOTE: cr3 has low bits of importance, but since the page
 * directory is 4k aligned we will know that the low bits will
 * be 0, which is their intended values
 * Inputs: None
 * Returns: None
 * Side Effects: TLB cleared for all non-global pages
 */
static void setup_control_registers() {
    // 1. set cr3 to page_directory address
    // 2. turn on bit 5 of cr4 (4M pages)
    // 3. turn on bit 31 of cr0 (enables paging)
    asm volatile ("                      \n\
        movl   $page_directory, %%eax    \n\
        movl   %%eax, %%cr3              \n\
        movl   %%cr4, %%eax              \n\
        orl    $0x00000010, %%eax        \n\
        movl   %%eax, %%cr4              \n\
        movl   %%cr0, %%eax              \n\
        orl    $0x80000000, %%eax        \n\
        movl   %%eax, %%cr0              \n\
        "
        :                  
        :                   
        : "eax" // clobbers %EAX
    ); 
}

/* reload_cr3
 * 
 * Reloads cr3 to flush the TLBs
 *
 * Inputs: None
 * Returns: None
 * Side Effects: TLB cleared for all non-global pages
 */
void reload_cr3() {
    asm volatile ("                      \n\
        movl   $page_directory, %%eax    \n\
        movl   %%eax, %%cr3              \n\
		"
		:
		:
		: "eax" // clobbers %EAX
	);
}

