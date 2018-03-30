/*
 * vim:ts=4 noexpandtab
 */

#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"
#include "terminal.h"
#define VID_MAP_VIRTUAL_INDEX 31

/* Defines a 32-bit structure for a page table entry */
typedef struct pte_t {
    union {
        uint32_t val;
        struct {
            uint32_t present            :1; // is this entry active?
            uint32_t rw                 :1; // RW is 0, RO is 1
            uint32_t privilege_level    :1; // 0 for kernel, 1 for user
            uint32_t write_through      :1;
            uint32_t cache_disabled     :1;
            uint32_t accessed           :1;
            uint32_t dirty              :1;
            uint32_t pat_index          :1;
            uint32_t global_page        :1;
            uint32_t avail              :3; // available for our usage
            uint32_t addr               :20; // 20 bits addr to start of page
        } __attribute__ ((packed));
    };
} pte_t;

/* Defines a 32-bit structure for a page directory entry */
typedef struct pde_t {
    union{
        uint32_t val;
        struct {
            uint32_t present           :1; // is this entry active?
            uint32_t rw                :1; // RW is 0, RO is 1
            uint32_t privilege_level   :1; // 0 for kernel, 1 for user
            uint32_t write_through     :1;
            uint32_t cache_disabled    :1;
            uint32_t accessed          :1;
            uint32_t reserved_dirty    :1; // reserved (always 0) for 4k, dirty for 4M
            uint32_t size              :1; // 0 for 4K pages, 1 for 4M pages
            uint32_t global_page       :1;
            uint32_t avail             :3; // available for our usage
            // for 4M: 10 bits addr, 9 bits reserved, 1 bit PAT.
            // for 4K: 20 bits addr to page table
            uint32_t addr              :20; 
        } __attribute__ ((packed));
    };
} pde_t;

/* statically allocate one page directory
 * and one page table at entry 0 in the page directory
 */
pde_t page_directory[1024] __attribute__((aligned (4096)));
pte_t page_table_0[1024] __attribute__((aligned (4096)));
pte_t vid_page_table_0[1024] __attribute__((aligned (4096)));

/* function to initialize the paging hardware in the ISA */
void init_paging();
/* function to create a 4MB page that exists in real memory
 * at real_index * 4MB and make the page in virt memroy at
 * virt_index * 4MB */
int create_user_4mb_page(int real_index, int virt_index);
/* function to create a 4kb page that exists in real memory
 * at pde_index * 4MB + pte_index * 4kb and make the page in virt memory at
 * virt_index */
uint32_t create_vid_4kb_page();
/* function to reload cr3 and clear the TLBs */
void reload_cr3();
void remap_vid(int exec_term_id);

#endif /* _PAGING_H */
