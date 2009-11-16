/*
 * This file is part of the Palacios Virtual Machine Monitor developed
 * by the V3VEE Project with funding from the United States National 
 * Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  You can find out more at 
 * http://www.v3vee.org
 *
 * Copyright (c) 2008, Andy Gocke <agocke@gmail.com>
 * Copyright (c) 2008, The V3VEE Project <http://www.v3vee.org> 
 * All rights reserved.
 *
 * Author: Andy Gocke <agocke@gmail.com>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "V3VEE_LICENSE".
 */

#include <palacios/vmm.h>
#include <palacios/vm_guest.h>
#include <palacios/vmm_msr.h>

#define LOW_MSR_START   0x00000000
#define LOW_MSR_END     0x1fff
#define HIGH_MSR_START  0xc0000000
#define HIGH_MSR_END    0xc0001fff

#define LOW_MSR_INDEX   0
#define HIGH_MSR_INDEX  1024

static int get_bitmap_index(uint_t msr)
{
    if( (msr >= LOW_MSR_START) && msr <= LOW_MSR_END) {
        return LOW_MSR_INDEX + msr;
    } else if (( msr >= HIGH_MSR_START ) && (msr <= HIGH_MSR_END)) {
        return HIGH_MSR_INDEX + (msr - HIGH_MSR_START);
    } else {
        PrintError("MSR out of range: 0x%x\n", msr);
        return -1;
    }
}

/* Same as SVM */
static int update_map(struct guest_info * info, uint_t msr, int hook_reads, int hook_writes) {

    int index = get_bitmap_index(msr);
    uint_t major = index / 8;
    uint_t minor = (index % 8);
    uchar_t mask = 0x1;
    uint8_t read_val = (hook_reads) ? 0x1 : 0x0;
    uint8_t write_val = (hook_writes) ? 0x1 : 0x0;
    uint8_t * bitmap = (uint8_t *)(info->msr_map.arch_data);


    *(bitmap + major) &= ~(mask << minor);
    *(bitmap + major) |= (read_val << minor);
    

    *(bitmap + 2048 + major) &= ~(mask << minor);
    *(bitmap + 2048 + major) |= (write_val << minor);
    
    return 0;
}

int v3_init_vmx_msr_map(struct guest_info * info) {
    struct v3_msr_map * msr_map = &(info->msr_map);

    msr_map->update_map = update_map;
    
    msr_map->arch_data = V3_VAddr(V3_AllocPages(1));
    memset(msr_map->arch_data, 0, PAGE_SIZE_4KB);
    
    v3_refresh_msr_map(info);
    
    return 0;
}