/* 
 * V3 Control header file 
 * (c) Jack lange, 2010
 */

#ifndef _v3_ctrl_h
#define _v3_ctrl_h

#include <stdio.h>
#include <stdlib.h>
#include "ezxml.h"

/* Global Control IOCTLs */
#define V3_CREATE_GUEST 12
#define V3_FREE_GUEST 13

#define V3_ADD_MEMORY 50
#define V3_RESET_MEMORY 51
#define V3_ADD_PCI_HW_DEV 55
#define V3_ADD_PCI_USER_DEV 56

/* VM Specific IOCTLs */

/* VM Specific ioctls */
#define V3_VM_CONSOLE_CONNECT 20
#define V3_VM_SERIAL_CONNECT 21
#define V3_VM_PAUSE 23
#define V3_VM_CONTINUE 24

#define V3_VM_LAUNCH 25
#define V3_VM_STOP 26
#define V3_VM_LOAD 27
#define V3_VM_SAVE 28
#define V3_VM_SIMULATE 29
#define V3_VM_INSPECT 30
#define V3_VM_DEBUG 31


#define V3_VM_MOVE_CORE 33

#define V3_VM_SEND    34
#define V3_VM_RECEIVE 35

#define V3_VM_FB_INPUT 257
#define V3_VM_FB_QUERY 258

#define V3_VM_HOST_DEV_CONNECT 10245
#define V3_VM_KSTREAM_USER_CONNECT 11245


static const char * v3_dev = "/dev/v3vee";

struct v3_guest_img {
    unsigned long long size;
    void * guest_data;
    char name[128];
} __attribute__((packed));


struct v3_mem_region {
    unsigned long long base_addr;
    unsigned long long num_pages;
} __attribute__((packed));


struct v3_core_move_cmd{
    unsigned short vcore_id;
    unsigned short pcore_id;
} __attribute__((packed));


struct v3_debug_cmd {
    unsigned int core; 
    unsigned int cmd;
} __attribute__((packed));

struct v3_chkpt_info {
    char store[128];
    char url[256]; /* This might need to be bigger... */
} __attribute__((packed));



struct v3_hw_pci_dev {
    char url[128];
    unsigned int bus;
    unsigned int dev;
    unsigned int func;
} __attribute__((packed));

#define V3VEE_STR "\n\n"                         \
                  "The V3Vee Project (c) 2012\n" \
                  "\thttp://v3vee.org\n"         \
                  "\n\n"
                   
#define v3_usage(fmt, args...)                                \
{                                                             \
    printf(("\nUsage: %s " fmt V3VEE_STR), argv[0], ##args);  \
    exit(0);                                                  \
}


int v3_dev_ioctl (int req, void * arg);
int v3_vm_ioctl  (const char * filename,
                  int req,
                  void * arg);
void * v3_mmap_file (const char * filename, int prot, int flags);
int v3_read_file (int fd, int size, unsigned char * buf);

int launch_vm (const char * filename);
int stop_vm   (const char * filename);


/* XML-related structs */
struct cfg_value {
    char * tag;
    char * value;
};

struct xml_option {
    char * tag;
    ezxml_t location;
    struct xml_option * next;
};


struct file_info {
    int size;
    char filename[2048];
    char id[256];
};

#endif
