/*
 * This file is part of the Palacios Virtual Machine Monitor developed
 * by the V3VEE Project with funding from the United States National 
 * Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  You can find out more at 
 * http://www.v3vee.org
 *
 * Copyright (c) 2008, Jack Lange <jarusl@cs.northwestern.edu> 
 * Copyright (c) 2008, The V3VEE Project <http://www.v3vee.org> 
 * All rights reserved.
 *
 * Author: Jack Lange <jarusl@cs.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "V3VEE_LICENSE".
 */


#ifndef __VMM_PAGING_H__
#define __VMM_PAGING_H__


#ifdef __V3VEE__

#include <palacios/vmm_types.h>
#include <palacios/vmm_util.h>


/*

In the following, when we say "page table", we mean the whole 2 or 4 layer
page table (PDEs, PTEs), etc.


guest-visible paging state
 This is the state that the guest thinks the machine is using
 It consists of
   - guest physical memory
       The physical memory addresses the guest is allowed to use
       (see shadow page maps, below)
   - guest page tables 
       (we care about when the current one changes)
   - guest paging registers (these are never written to hardware)
        CR0
        CR3


shadow paging state
 This the state that the machine will actually use when the guest
 is running.  It consists of:
   - current shadow page table
        This is the page table actually useed when the guest is running.
        It is changed/regenerated when the guest page table changes
        It mostly reflects the guest page table, except that it restricts 
        physical addresses to those the VMM allocates to the guest.
   - shadow page maps
        This is a mapping from guest physical memory addresses to
        the current location of the guest physical memory content.   
        It maps from regions of physical memory addresses to regions 
        located in physical memory or elsewhere.  
        (8192,16384) -> MEM(8912,...)
        (0,8191) -> DISK(65536,..) 
   - guest paging registers (these are written to guest state)
        CR0
        CR3

host paging state
  This is the state we expect to be operative when the VMM is running.
  Typically, this is set up by the host os into which we have embedded
  the VMM, but we include the description here for clarity.
    - current page table
        This is the page table we use when we are executing in 
        the VMM (or the host os)
    - paging regisers
        CR0
        CR3


The reason why the shadow paging state and the host paging state are
distinct is to permit the guest to use any virtual address it wants,
irrespective of the addresses the VMM or the host os use.  These guest
virtual addresses are reflected in the shadow paging state.  When we
exit from the guest, we switch to the host paging state so that any
virtual addresses that overlap between the guest and VMM/host now map
to the physical addresses epxected by the VMM/host.  On AMD SVM, this
switch is done by the hardware.  On Intel VT, the switch is done
by the hardware as well, but we are responsible for manually updating
the host state in the vmcs before entering the guest.
*/




#define MAX_PTE32_ENTRIES          1024
#define MAX_PDE32_ENTRIES          1024

#define MAX_PTE32PAE_ENTRIES       512
#define MAX_PDE32PAE_ENTRIES       512
#define MAX_PDPE32PAE_ENTRIES      4

#define MAX_PTE64_ENTRIES          512
#define MAX_PDE64_ENTRIES          512
#define MAX_PDPE64_ENTRIES         512
#define MAX_PML4E64_ENTRIES        512


/* Converts an address into a page table index */
#define PDE32_INDEX(x)  ((((uint_t)x) >> 22) & 0x3ff)
#define PTE32_INDEX(x)  ((((uint_t)x) >> 12) & 0x3ff)


#define PDPE32PAE_INDEX(x) ((((uint_t)x) >> 30) & 0x3)
#define PDE32PAE_INDEX(x)  ((((uint_t)x) >> 21) & 0x1ff)
#define PTE32PAE_INDEX(x)  ((((uint_t)x) >> 12) & 0x1ff)

#define PML4E64_INDEX(x) ((((ullong_t)x) >> 39) & 0x1ff)
#define PDPE64_INDEX(x) ((((ullong_t)x) >> 30) & 0x1ff)
#define PDE64_INDEX(x) ((((ullong_t)x) >> 21) & 0x1ff)
#define PTE64_INDEX(x) ((((ullong_t)x) >> 12) & 0x1ff)


/* Gets the base address needed for a Page Table entry */
/* Deprecate these :*/
/*
  #define PD32_BASE_ADDR(x) (((uint_t)x) >> 12)
  #define PT32_BASE_ADDR(x) (((uint_t)x) >> 12)
  #define PD32_4MB_BASE_ADDR(x) (((uint_t)x) >> 22)
  
  #define PML4E64_BASE_ADDR(x) (((ullong_t)x) >> 12)
  #define PDPE64_BASE_ADDR(x) (((ullong_t)x) >> 12)
  #define PDE64_BASE_ADDR(x) (((ullong_t)x) >> 12)
  #define PTE64_BASE_ADDR(x) (((ullong_t)x) >> 12)
  
  // Accessor functions for the page table structures 
  #define PDE32_T_ADDR(x) (((x).pt_base_addr) << 12)
  #define PTE32_T_ADDR(x) (((x).page_base_addr) << 12)
  #define PDE32_4MB_T_ADDR(x) (((x).page_base_addr) << 22)
*/
/* Replace The above with these... */
#define PAGE_BASE_ADDR(x) ((x) >> 12)
#define PAGE_BASE_ADDR_2MB(x) ((x) >> 21)
#define PAGE_BASE_ADDR_4MB(x) ((x) >> 22)

#define BASE_TO_PAGE_ADDR(x) (((addr_t)x) << 12)
#define BASE_TO_PAGE_ADDR_2MB(x) (((addr_t)x) << 21)
#define BASE_TO_PAGE_ADDR_4MB(x) (((addr_t)x) << 22)
/* *** */

/* Deprecated */
/*
  #define PT32_PAGE_OFFSET(x) (((uint_t)x) & 0xfff)
  #define PD32_4MB_PAGE_OFFSET(x) (((uint_t)x) & 0x003fffff)
  
  #define PT32_PAGE_ADDR(x)   (((uint_t)x) & 0xfffff000)
  #define PD32_4MB_PAGE_ADDR(x) (((uint_t)x) & 0xffc00000)
  
  #define PT32_PAGE_POWER 12
  #define PAGE_ALIGNED_ADDR(x)   (((uint_t) (x)) >> 12)
  //#define PAGE_ADDR(x)   (PAGE_ALIGNED_ADDR(x) << 12)
  #define PAGE_POWER 12
  #define PAGE_SIZE 4096
*/
/* use these instead */
#define PAGE_OFFSET(x) ((x) & 0xfff)
#define PAGE_OFFSET_2MB(x) ((x) & 0x1fffff)
#define PAGE_OFFSET_4MB(x) ((x) & 0x3fffff)

#define PAGE_POWER 12
#define PAGE_POWER_2MB 22
#define PAGE_POWER_4MB 21

// We shift instead of mask because we don't know the address size
#define PAGE_ADDR(x) (((x) >> PAGE_POWER) << PAGE_POWER)
#define PAGE_ADDR_2MB(x) (((x) >> PAGE_POWER_2MB) << PAGE_POWER_2MB)
#define PAGE_ADDR_4MB(x) (((x) >> PAGE_POWER_4MB) << PAGE_POWER_4MB)

#define PAGE_SIZE 4096
#define PAGE_SIZE_2MB (4096 * 512)
#define PAGE_SIZE_4MB (4096 * 1024)


/* *** */





#define CR3_TO_PDE32_PA(cr3) ((addr_t)(((uint_t)cr3) & 0xfffff000))
#define CR3_TO_PDPE32PAE_PA(cr3) ((addr_t)(((uint_t)cr3) & 0xffffffe0))
#define CR3_TO_PML4E64_PA(cr3)  ((addr_t)(((ullong_t)cr3) & 0x000ffffffffff000LL))

#define CR3_TO_PDE32_VA(cr3) ((pde32_t *)V3_VAddr((void *)(addr_t)(((uint_t)cr3) & 0xfffff000)))
#define CR3_TO_PDPE32PAE_VA(cr3) ((pdpe32pae_t *)V3_VAddr((void *)(addr_t)(((uint_t)cr3) & 0xffffffe0)))
#define CR3_TO_PML4E64_VA(cr3)  ((pml4e64_t *)V3_VAddr((void *)(addr_t)(((ullong_t)cr3) & 0x000ffffffffff000LL)))






/* Page Table Flag Values */
#define PT32_HOOK 0x1
#define PT32_GUEST_PT 0x2



/* We'll use the general form for now.... 
   typedef enum {PDE32_ENTRY_NOT_PRESENT, PDE32_ENTRY_PTE32, PDE32_ENTRY_LARGE_PAGE} pde32_entry_type_t;
   typedef enum {PTE32_ENTRY_NOT_PRESENT, PTE32_ENTRY_PAGE} pte32_entry_type_t;
   
   typedef enum {PDPE32PAE_ENTRY_NOT_PRESENT, PDPE32PAE_ENTRY_PAGE} pdpe32pae_entry_type_t;
   typedef enum {PDE32PAE_ENTRY_NOT_PRESENT, PDE32PAE_ENTRY_PTE32, PDE32PAE_ENTRY_LARGE_PAGE} pde32pae_entry_type_t;
   typedef enum {PTE32PAE_ENTRY_NOT_PRESENT, PTE32PAE_ENTRY_PAGE} pte32pae_entry_type_t;
   
   typedef enum {PML4E64_ENTRY_NOT_PRESENT, PML4E64_ENTRY_PAGE} pml4e64_entry_type_t;
   typedef enum {PDPE64_ENTRY_NOT_PRESENT, PDPE64_ENTRY_PTE32, PDPE64_ENTRY_LARGE_PAGE} pdpe64_entry_type_t;
   typedef enum {PDE64_ENTRY_NOT_PRESENT, PDE64_ENTRY_PTE32, PDE64_ENTRY_LARGE_PAGE} pde64_entry_type_t;
   typedef enum {PTE64_ENTRY_NOT_PRESENT, PTE64_ENTRY_PAGE} pte64_entry_type_t;
*/


typedef enum {PT_ENTRY_NOT_PRESENT, PT_ENTRY_LARGE_PAGE, PT_ENTRY_PAGE} pt_entry_type_t;
typedef enum {PT_ACCESS_OK, PT_ACCESS_NOT_PRESENT, PT_ACCESS_WRITE_ERROR, PT_ACCESS_USER_ERROR} pt_access_status_t;

typedef struct pde32 {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t reserved        : 1;
  uint_t large_page     : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  uint_t pt_base_addr    : 20;
} __attribute__((packed))  pde32_t;

typedef struct pde32_4MB {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t dirty           : 1;
  uint_t large_page      : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  uint_t pat             : 1;
  uint_t rsvd            : 9;
  uint_t page_base_addr  : 10;

} __attribute__((packed))  pde32_4MB_t;

typedef struct pte32 {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t dirty           : 1;
  uint_t pte_attr        : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  uint_t page_base_addr  : 20;
}  __attribute__((packed)) pte32_t;
/* ***** */

/* 32 bit PAE PAGE STRUCTURES */
typedef struct pdpe32pae {
  uint_t present       : 1;
  uint_t rsvd          : 2; // MBZ
  uint_t write_through : 1;
  uint_t cache_disable : 1;
  uint_t accessed      : 1; 
  uint_t avail         : 1;
  uint_t rsvd2         : 2;  // MBZ
  uint_t vmm_info      : 3;
  uint_t pd_base_addr  : 24;
  uint_t rsvd3         : 28; // MBZ
} __attribute__((packed)) pdpe32pae_t;



typedef struct pde32pae {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t avail           : 1;
  uint_t large_page      : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  uint_t pt_base_addr    : 24;
  uint_t rsvd            : 28;
} __attribute__((packed)) pde32pae_t;

typedef struct pde32pae_2MB {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t dirty           : 1;
  uint_t one             : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  uint_t pat             : 1;
  uint_t rsvd            : 8;
  uint_t page_base_addr  : 15;
  uint_t rsvd2           : 28;

} __attribute__((packed)) pde32pae_2MB_t;

typedef struct pte32pae {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t dirty           : 1;
  uint_t pte_attr        : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  uint_t page_base_addr  : 24;
  uint_t rsvd            : 28;
} __attribute__((packed)) pte32pae_t;





/* ********** */


/* LONG MODE 64 bit PAGE STRUCTURES */
typedef struct pml4e64 {
  uint_t present        : 1;
  uint_t writable       : 1;
  uint_t user_page           : 1;
  uint_t write_through  : 1;
  uint_t cache_disable  : 1;
  uint_t accessed       : 1;
  uint_t reserved       : 1;
  uint_t zero           : 2;
  uint_t vmm_info       : 3;
  ullong_t pdp_base_addr : 40;
  uint_t available      : 11;
  uint_t no_execute     : 1;
} __attribute__((packed)) pml4e64_t;


typedef struct pdpe64 {
  uint_t present        : 1;
  uint_t writable       : 1;
  uint_t user_page      : 1;
  uint_t write_through  : 1;
  uint_t cache_disable  : 1;
  uint_t accessed       : 1;
  uint_t avail          : 1;
  uint_t large_page     : 1;
  uint_t zero           : 1;
  uint_t vmm_info       : 3;
  ullong_t pd_base_addr : 40;
  uint_t available      : 11;
  uint_t no_execute     : 1;
} __attribute__((packed)) pdpe64_t;


// We Don't support this
typedef struct pdpe64_1GB {
  uint_t present        : 1;
  uint_t writable       : 1;
  uint_t user_page      : 1;
  uint_t write_through  : 1;
  uint_t cache_disable  : 1;
  uint_t accessed       : 1;
  uint_t dirty          : 1;
  uint_t large_page     : 1;
  uint_t global_page    : 1;
  uint_t vmm_info       : 3;
  uint_t pat            : 1;
  uint_t rsvd           : 17;
  ullong_t page_base_addr : 22;
  uint_t available      : 11;
  uint_t no_execute     : 1;
} __attribute__((packed)) pdpe64_1GB_t;



typedef struct pde64 {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t avail           : 1;
  uint_t large_page      : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  ullong_t pt_base_addr  : 40;
  uint_t available       : 11;
  uint_t no_execute      : 1;
} __attribute__((packed)) pde64_t;

typedef struct pde64_2MB {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t dirty           : 1;
  uint_t large_page      : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  uint_t pat             : 1;
  uint_t rsvd            : 8;
  ullong_t page_base_addr  : 31;
  uint_t available       : 11;
  uint_t no_execute      : 1;
} __attribute__((packed)) pde64_2MB_t;


typedef struct pte64 {
  uint_t present         : 1;
  uint_t writable        : 1;
  uint_t user_page       : 1;
  uint_t write_through   : 1;
  uint_t cache_disable   : 1;
  uint_t accessed        : 1;
  uint_t dirty           : 1;
  uint_t pte_attr        : 1;
  uint_t global_page     : 1;
  uint_t vmm_info        : 3;
  ullong_t page_base_addr : 40;
  uint_t available       : 11;
  uint_t no_execute      : 1;
} __attribute__((packed)) pte64_t;

/* *************** */

typedef struct pf_error_code {
  uint_t present           : 1; // if 0, fault due to page not present
  uint_t write             : 1; // if 1, faulting access was a write
  uint_t user              : 1; // if 1, faulting access was in user mode
  uint_t rsvd_access       : 1; // if 1, fault from reading a 1 from a reserved field (?)
  uint_t ifetch            : 1; // if 1, faulting access was an instr fetch (only with NX)
  uint_t rsvd              : 27;
} __attribute__((packed)) pf_error_t;




void delete_page_tables_32(pde32_t * pde);
void delete_page_tables_32PAE(pdpe32pae_t * pdpe);
void delete_page_tables_64(pml4e64_t *  pml4);


int v3_translate_guest_pt_32(struct guest_info * info, addr_t guest_cr3, addr_t vaddr, addr_t * paddr);
int v3_translate_guest_pt_32pae(struct guest_info * info, addr_t guest_cr3, addr_t vaddr, addr_t * paddr);
int v3_translate_guest_pt_64(struct guest_info * info, addr_t guest_cr3, addr_t vaddr, addr_t * paddr);

int v3_translate_host_pt_32(addr_t host_cr3, addr_t vaddr, addr_t * paddr);
int v3_translate_host_pt_32pae(addr_t host_cr3, addr_t vaddr, addr_t * paddr);
int v3_translate_host_pt_64(addr_t host_cr3, addr_t vaddr, addr_t * paddr);


/* Should these be static?
pt_entry_type_t pde32_lookup(pde32_t * pd, addr_t addr, addr_t * entry);
pt_entry_type_t pte32_lookup(pte32_t * pt, addr_t addr, addr_t * entry);

pt_entry_type_t pdpe32pae_lookup(pdpe32pae_t * pdp, addr_t addr, addr_t * entry);
pt_entry_type_t pde32pae_lookup(pde32pae_t * pd, addr_t addr, addr_t * entry);
pt_entry_type_t pte32pae_lookup(pte32pae_t * pt, addr_t addr, addr_t * entry);

pt_entry_type_t pml4e64_lookup(pml4e64_t * pml, addr_t addr, addr_t * entry);
pt_entry_type_t pdpe64_lookup(pdpe64_t * pdp, addr_t addr, addr_t * entry);
pt_entry_type_t pde64_lookup(pde64_t * pd, addr_t addr, addr_t * entry);
pt_entry_type_t pte64_lookup(pte64_t * pt, addr_t addr, addr_t * entry);






pt_access_status_t can_access_pde32(pde32_t * pde, addr_t addr, pf_error_t access_type);
pt_access_status_t can_access_pte32(pte32_t * pte, addr_t addr, pf_error_t access_type);





struct guest_info;

pde32_t * create_passthrough_pts_32(struct guest_info * guest_info);
pdpe32pae_t * create_passthrough_pts_32PAE(struct guest_info * guest_info);
pml4e64_t * create_passthrough_pts_64(struct guest_info * info);




//#include <palacios/vm_guest.h>

void PrintDebugPageTables(pde32_t * pde);


void PrintPageTree(v3_vm_cpu_mode_t cpu_mode, addr_t virtual_addr, addr_t cr3);
void PrintPageTree_64(addr_t virtual_addr, pml4e64_t * pml);


void PrintPT32(addr_t starting_address, pte32_t * pte);
void PrintPD32(pde32_t * pde);
void PrintPTE32(addr_t virtual_address, pte32_t * pte);
void PrintPDE32(addr_t virtual_address, pde32_t * pde);
  
void PrintDebugPageTables32PAE(pdpe32pae_t * pde);
void PrintPTE32PAE(addr_t virtual_address, pte32pae_t * pte);
void PrintPDE32PAE(addr_t virtual_address, pde32pae_t * pde);
void PrintPTE64(addr_t virtual_address, pte64_t * pte);

#endif // !__V3VEE__



#endif
