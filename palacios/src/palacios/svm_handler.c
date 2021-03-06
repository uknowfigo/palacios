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


#include <palacios/svm_handler.h>
#include <palacios/vmm.h>
#include <palacios/vm_guest_mem.h>
#include <palacios/vmm_decoder.h>
#include <palacios/vmm_ctrl_regs.h>
#include <palacios/svm_io.h>
#include <palacios/svm_halt.h>
#include <palacios/svm_pause.h>
#include <palacios/svm_wbinvd.h>
#include <palacios/vmm_intr.h>
#include <palacios/vmm_emulator.h>




static const uchar_t * vmexit_code_to_str(uint_t exit_code);


int v3_handle_svm_exit(struct guest_info * info) {
  vmcb_ctrl_t * guest_ctrl = 0;
  vmcb_saved_state_t * guest_state = 0;
  ulong_t exit_code = 0;
  
  guest_ctrl = GET_VMCB_CTRL_AREA((vmcb_t*)(info->vmm_data));
  guest_state = GET_VMCB_SAVE_STATE_AREA((vmcb_t*)(info->vmm_data));
  

  // Update the high level state 
  info->rip = guest_state->rip;
  info->vm_regs.rsp = guest_state->rsp;
  info->vm_regs.rax = guest_state->rax;

  info->cpl = guest_state->cpl;


  info->ctrl_regs.cr0 = guest_state->cr0;
  info->ctrl_regs.cr2 = guest_state->cr2;
  info->ctrl_regs.cr3 = guest_state->cr3;
  info->ctrl_regs.cr4 = guest_state->cr4;
  info->dbg_regs.dr6 = guest_state->dr6;
  info->dbg_regs.dr7 = guest_state->dr7;
  info->ctrl_regs.cr8 = guest_ctrl->guest_ctrl.V_TPR;
  info->ctrl_regs.rflags = guest_state->rflags;
  info->ctrl_regs.efer = guest_state->efer;

  get_vmcb_segments((vmcb_t*)(info->vmm_data), &(info->segments));
  info->cpu_mode = v3_get_cpu_mode(info);
  info->mem_mode = v3_get_mem_mode(info);


  exit_code = guest_ctrl->exit_code;
 

  // Disable printing io exits due to bochs debug messages
  //if (!((exit_code == VMEXIT_IOIO) && ((ushort_t)(guest_ctrl->exit_info1 >> 16) == 0x402))) {


  //  PrintDebug("SVM Returned: Exit Code: 0x%x \t\t(tsc=%ul)\n",exit_code, (uint_t)info->time_state.guest_tsc); 
  
  if ((0) && (exit_code < 0x4f)) {
    uchar_t instr[32];
    int ret;
    // Dump out the instr stream

    //PrintDebug("RIP: %x\n", guest_state->rip);
    PrintDebug("RIP Linear: %p\n", (void *)get_addr_linear(info, info->rip, &(info->segments.cs)));

    // OK, now we will read the instruction
    // The only difference between PROTECTED and PROTECTED_PG is whether we read
    // from guest_pa or guest_va
    if (info->mem_mode == PHYSICAL_MEM) { 
      // The real rip address is actually a combination of the rip + CS base 
      ret = read_guest_pa_memory(info, get_addr_linear(info, info->rip, &(info->segments.cs)), 32, instr);
    } else { 
      ret = read_guest_va_memory(info, get_addr_linear(info, info->rip, &(info->segments.cs)), 32, instr);
    }
    
    if (ret != 32) {
      // I think we should inject a GPF into the guest
      PrintDebug("Could not read instruction (ret=%d)\n", ret);
    } else {

      PrintDebug("Instr Stream:\n");
      PrintTraceMemDump(instr, 32);
    }
  }


    //  }
  // PrintDebugVMCB((vmcb_t*)(info->vmm_data));


  // PrintDebug("SVM Returned:(VMCB=%x)\n", info->vmm_data); 
  //PrintDebug("RIP: %x\n", guest_state->rip);

  
  //PrintDebug("SVM Returned: Exit Code: %x\n",exit_code); 

  switch (exit_code) {

  case VMEXIT_IOIO: {
    struct svm_io_info * io_info = (struct svm_io_info *)&(guest_ctrl->exit_info1);
    
    if (io_info->type == 0) {
      if (io_info->str) {
	if (v3_handle_svm_io_outs(info) == -1 ) {
	  return -1;
	}
      } else {
	if (v3_handle_svm_io_out(info) == -1) {
	  return -1;
	}
      }
    } else {
      if (io_info->str) {
	if (v3_handle_svm_io_ins(info) == -1) {
	  return -1;
	}
      } else {
	if (v3_handle_svm_io_in(info) == -1) {
	  return -1;
	}
      }
    }
  }
    break;


  case  VMEXIT_CR0_WRITE: {
#ifdef DEBUG_CTRL_REGS
    PrintDebug("CR0 Write\n");
#endif
    if (v3_handle_cr0_write(info) == -1) {
      return -1;
    }
  } 
    break;

  case VMEXIT_CR0_READ: {
#ifdef DEBUG_CTRL_REGS
    PrintDebug("CR0 Read\n");
#endif
    if (v3_handle_cr0_read(info) == -1) {
      return -1;
    }
  } 
    break;

  case VMEXIT_CR3_WRITE: {
#ifdef DEBUG_CTRL_REGS
    PrintDebug("CR3 Write\n");
#endif
    if (v3_handle_cr3_write(info) == -1) {
      return -1;
    }    
  } 
    break;

  case  VMEXIT_CR3_READ: {
#ifdef DEBUG_CTRL_REGS
    PrintDebug("CR3 Read\n");
#endif
    if (v3_handle_cr3_read(info) == -1) {
      return -1;
    }
  }
    break;

  case VMEXIT_EXCP14: {
    addr_t fault_addr = guest_ctrl->exit_info2;
    pf_error_t * error_code = (pf_error_t *)&(guest_ctrl->exit_info1);
#ifdef DEBUG_SHADOW_PAGING
    PrintDebug("PageFault at %p (error=%d)\n", 
	       (void *)fault_addr, *(uint_t *)error_code);
#endif
    if (info->shdw_pg_mode == SHADOW_PAGING) {
      if (v3_handle_shadow_pagefault(info, fault_addr, *error_code) == -1) {
	return -1;
      }
    } else {
      PrintError("Page fault in un implemented paging mode\n");
      return -1;
    }
  } 
    break;

  case VMEXIT_NPF: {
    PrintError("Currently unhandled Nested Page Fault\n");
    return -1;
    
  } 
    break;

  case VMEXIT_INVLPG: {
    if (info->shdw_pg_mode == SHADOW_PAGING) {
#ifdef DEBUG_SHADOW_PAGING
      PrintDebug("Invlpg\n");
#endif
      if (v3_handle_shadow_invlpg(info) == -1) {
	return -1;
      }
    }
   
    /*
      (exit_code == VMEXIT_INVLPGA)   || 
    */
    
  } 
    break;

  case VMEXIT_INTR: { 
    
    // handled by interrupt dispatch earlier

  } 
    break;
    
  case VMEXIT_SMI: {
    
    //   handle_svm_smi(info); // ignored for now

  } 
    break;

  case VMEXIT_HLT: {
#ifdef DEBUG_HALT
    PrintDebug("Guest halted\n");
#endif
    if (v3_handle_svm_halt(info) == -1) {
      return -1;
    }
  } 
    break;

  case VMEXIT_PAUSE: {
    //PrintDebug("Guest paused\n");
    if (v3_handle_svm_pause(info) == -1) { 
      return -1;
    }
  } 
    break;

  case VMEXIT_EXCP1: 
    {
#ifdef DEBUG_EMULATOR
      PrintDebug("DEBUG EXCEPTION\n");
#endif
      if (info->run_state == VM_EMULATING) {
	if (v3_emulation_exit_handler(info) == -1) {
	  return -1;
	}
      } else {
	PrintError("VMMCALL with not emulator...\n");
	return -1;
      }
      break;
    } 


  case VMEXIT_VMMCALL: 
    {
#ifdef DEBUG_EMULATOR
      PrintDebug("VMMCALL\n");
#endif
      if (info->run_state == VM_EMULATING) {
	if (v3_emulation_exit_handler(info) == -1) {
	  return -1;
	}
      } else {
	/*
	ulong_t tsc_spread = 0;
	ullong_t exit_tsc = 0;

	ulong_t rax = (ulong_t)info->vm_regs.rbx;
	ulong_t rdx = (ulong_t)info->vm_regs.rcx;

	*(ulong_t *)(&exit_tsc) = rax;
	*(((ulong_t *)(&exit_tsc)) + 1) = rdx; 

	tsc_spread = info->exit_tsc - exit_tsc;

	PrintError("VMMCALL tsc diff = %lu\n",tsc_spread); 
	info->rip += 3;
	*/
	PrintError("VMMCALL with not emulator...\n");
	return -1;
      }
      break;
    } 
    


  case VMEXIT_WBINVD: 
    {
#ifdef DEBUG_EMULATOR
      PrintDebug("WBINVD\n");
#endif
      if (!v3_handle_svm_wbinvd(info)) { 
	return -1;
      }
      break;
    }




    /* Exits Following this line are NOT HANDLED */
    /*=======================================================================*/

  default: {

    addr_t rip_addr;
    uchar_t buf[15];
    addr_t host_addr;

    PrintDebug("Unhandled SVM Exit: %s\n", vmexit_code_to_str(exit_code));

    rip_addr = get_addr_linear(info, guest_state->rip, &(info->segments.cs));


    PrintError("SVM Returned:(VMCB=%p)\n", (void *)(info->vmm_data)); 
    PrintError("RIP: %p\n", (void *)(addr_t)(guest_state->rip));
    PrintError("RIP Linear: %p\n", (void *)(addr_t)(rip_addr));
    
    PrintError("SVM Returned: Exit Code: %p\n", (void *)(addr_t)exit_code); 
    
    PrintError("io_info1 low = 0x%.8x\n", *(uint_t*)&(guest_ctrl->exit_info1));
    PrintError("io_info1 high = 0x%.8x\n", *(uint_t *)(((uchar_t *)&(guest_ctrl->exit_info1)) + 4));
    
    PrintError("io_info2 low = 0x%.8x\n", *(uint_t*)&(guest_ctrl->exit_info2));
    PrintError("io_info2 high = 0x%.8x\n", *(uint_t *)(((uchar_t *)&(guest_ctrl->exit_info2)) + 4));

    

    if (info->mem_mode == PHYSICAL_MEM) {
      if (guest_pa_to_host_va(info, guest_state->rip, &host_addr) == -1) {
	PrintError("Could not translate guest_state->rip to host address\n");
	return -1;
      }
    } else if (info->mem_mode == VIRTUAL_MEM) {
      if (guest_va_to_host_va(info, guest_state->rip, &host_addr) == -1) {
	PrintError("Could not translate guest_state->rip to host address\n");
	return -1;
      }
    } else {
      PrintError("Invalid memory mode\n");
      return -1;
    }
    
    PrintError("Host Address of rip = 0x%p\n", (void *)host_addr);
    
    memset(buf, 0, 32);
    
    PrintError("Reading instruction stream in guest (addr=%p)\n", (void *)rip_addr);
    
    if (info->mem_mode == PHYSICAL_MEM) {
      read_guest_pa_memory(info, rip_addr - 16, 32, buf);
    } else {
      read_guest_va_memory(info, rip_addr - 16, 32, buf);
    }
    
    PrintDebug("16 bytes before Rip\n");
    PrintTraceMemDump(buf, 16);
    PrintDebug("Rip onward\n");
    PrintTraceMemDump(buf+16, 16);
    
    return -1;

  }
    break;

  }
  // END OF SWITCH (EXIT_CODE)


  // Update the low level state

  if (v3_intr_pending(info)) {

    switch (v3_get_intr_type(info)) {
    case EXTERNAL_IRQ: 
      {
	uint_t irq = v3_get_intr_number(info);

        // check to see if ==-1 (non exists)

	/*	
	  guest_ctrl->EVENTINJ.vector = irq;
	  guest_ctrl->EVENTINJ.valid = 1;
	  guest_ctrl->EVENTINJ.type = SVM_INJECTION_EXTERNAL_INTR;
	*/
	
	guest_ctrl->guest_ctrl.V_IRQ = 1;
	guest_ctrl->guest_ctrl.V_INTR_VECTOR = irq;
	guest_ctrl->guest_ctrl.V_IGN_TPR = 1;
	guest_ctrl->guest_ctrl.V_INTR_PRIO = 0xf;
#ifdef DEBUG_INTERRUPTS
	PrintDebug("Injecting Interrupt %d (EIP=%p)\n", 
		   guest_ctrl->guest_ctrl.V_INTR_VECTOR, 
		   (void *)(addr_t)info->rip);
#endif
	v3_injecting_intr(info, irq, EXTERNAL_IRQ);
	
	break;
      }
    case NMI:
      guest_ctrl->EVENTINJ.type = SVM_INJECTION_NMI;
      break;
    case EXCEPTION:
      {
	uint_t excp = v3_get_intr_number(info);

	guest_ctrl->EVENTINJ.type = SVM_INJECTION_EXCEPTION;
	
	if (info->intr_state.excp_error_code_valid) {  //PAD
	  guest_ctrl->EVENTINJ.error_code = info->intr_state.excp_error_code;
	  guest_ctrl->EVENTINJ.ev = 1;
#ifdef DEBUG_INTERRUPTS
	  PrintDebug("Injecting error code %x\n", guest_ctrl->EVENTINJ.error_code);
#endif
	}
	
	guest_ctrl->EVENTINJ.vector = excp;
	
	guest_ctrl->EVENTINJ.valid = 1;
#ifdef DEBUG_INTERRUPTS
	PrintDebug("Injecting Interrupt %d (EIP=%p)\n", 
		   guest_ctrl->EVENTINJ.vector, 
		   (void *)(addr_t)info->rip);
#endif
	v3_injecting_intr(info, excp, EXCEPTION);
	break;
      }
    case SOFTWARE_INTR:
      guest_ctrl->EVENTINJ.type = SVM_INJECTION_SOFT_INTR;
      break;
    case VIRTUAL_INTR:
      guest_ctrl->EVENTINJ.type = SVM_INJECTION_VIRTUAL_INTR;
      break;

    case INVALID_INTR: 
    default:
      PrintError("Attempted to issue an invalid interrupt\n");
      return -1;
    }

  } else {
#ifdef DEBUG_INTERRUPTS
    PrintDebug("No interrupts/exceptions pending\n");
#endif
  }

  guest_state->cr0 = info->ctrl_regs.cr0;
  guest_state->cr2 = info->ctrl_regs.cr2;
  guest_state->cr3 = info->ctrl_regs.cr3;
  guest_state->cr4 = info->ctrl_regs.cr4;
  guest_state->dr6 = info->dbg_regs.dr6;
  guest_state->dr7 = info->dbg_regs.dr7;
  guest_ctrl->guest_ctrl.V_TPR = info->ctrl_regs.cr8 & 0xff;
  guest_state->rflags = info->ctrl_regs.rflags;
  guest_state->efer = info->ctrl_regs.efer;

  guest_state->cpl = info->cpl;

  guest_state->rax = info->vm_regs.rax;
  guest_state->rip = info->rip;
  guest_state->rsp = info->vm_regs.rsp;


  set_vmcb_segments((vmcb_t*)(info->vmm_data), &(info->segments));

  if (exit_code == VMEXIT_INTR) {
    //PrintDebug("INTR ret IP = %x\n", guest_state->rip);
  }

  return 0;
}


static const uchar_t VMEXIT_CR0_READ_STR[] = "VMEXIT_CR0_READ";
static const uchar_t VMEXIT_CR1_READ_STR[] = "VMEXIT_CR1_READ";
static const uchar_t VMEXIT_CR2_READ_STR[] = "VMEXIT_CR2_READ";
static const uchar_t VMEXIT_CR3_READ_STR[] = "VMEXIT_CR3_READ";
static const uchar_t VMEXIT_CR4_READ_STR[] = "VMEXIT_CR4_READ";
static const uchar_t VMEXIT_CR5_READ_STR[] = "VMEXIT_CR5_READ";
static const uchar_t VMEXIT_CR6_READ_STR[] = "VMEXIT_CR6_READ";
static const uchar_t VMEXIT_CR7_READ_STR[] = "VMEXIT_CR7_READ";
static const uchar_t VMEXIT_CR8_READ_STR[] = "VMEXIT_CR8_READ";
static const uchar_t VMEXIT_CR9_READ_STR[] = "VMEXIT_CR9_READ";
static const uchar_t VMEXIT_CR10_READ_STR[] = "VMEXIT_CR10_READ";
static const uchar_t VMEXIT_CR11_READ_STR[] = "VMEXIT_CR11_READ";
static const uchar_t VMEXIT_CR12_READ_STR[] = "VMEXIT_CR12_READ";
static const uchar_t VMEXIT_CR13_READ_STR[] = "VMEXIT_CR13_READ";
static const uchar_t VMEXIT_CR14_READ_STR[] = "VMEXIT_CR14_READ";
static const uchar_t VMEXIT_CR15_READ_STR[] = "VMEXIT_CR15_READ";
static const uchar_t VMEXIT_CR0_WRITE_STR[] = "VMEXIT_CR0_WRITE";
static const uchar_t VMEXIT_CR1_WRITE_STR[] = "VMEXIT_CR1_WRITE";
static const uchar_t VMEXIT_CR2_WRITE_STR[] = "VMEXIT_CR2_WRITE";
static const uchar_t VMEXIT_CR3_WRITE_STR[] = "VMEXIT_CR3_WRITE";
static const uchar_t VMEXIT_CR4_WRITE_STR[] = "VMEXIT_CR4_WRITE";
static const uchar_t VMEXIT_CR5_WRITE_STR[] = "VMEXIT_CR5_WRITE";
static const uchar_t VMEXIT_CR6_WRITE_STR[] = "VMEXIT_CR6_WRITE";
static const uchar_t VMEXIT_CR7_WRITE_STR[] = "VMEXIT_CR7_WRITE";
static const uchar_t VMEXIT_CR8_WRITE_STR[] = "VMEXIT_CR8_WRITE";
static const uchar_t VMEXIT_CR9_WRITE_STR[] = "VMEXIT_CR9_WRITE";
static const uchar_t VMEXIT_CR10_WRITE_STR[] = "VMEXIT_CR10_WRITE";
static const uchar_t VMEXIT_CR11_WRITE_STR[] = "VMEXIT_CR11_WRITE";
static const uchar_t VMEXIT_CR12_WRITE_STR[] = "VMEXIT_CR12_WRITE";
static const uchar_t VMEXIT_CR13_WRITE_STR[] = "VMEXIT_CR13_WRITE";
static const uchar_t VMEXIT_CR14_WRITE_STR[] = "VMEXIT_CR14_WRITE";
static const uchar_t VMEXIT_CR15_WRITE_STR[] = "VMEXIT_CR15_WRITE";
static const uchar_t VMEXIT_DR0_READ_STR[] = "VMEXIT_DR0_READ";
static const uchar_t VMEXIT_DR1_READ_STR[] = "VMEXIT_DR1_READ";
static const uchar_t VMEXIT_DR2_READ_STR[] = "VMEXIT_DR2_READ";
static const uchar_t VMEXIT_DR3_READ_STR[] = "VMEXIT_DR3_READ";
static const uchar_t VMEXIT_DR4_READ_STR[] = "VMEXIT_DR4_READ";
static const uchar_t VMEXIT_DR5_READ_STR[] = "VMEXIT_DR5_READ";
static const uchar_t VMEXIT_DR6_READ_STR[] = "VMEXIT_DR6_READ";
static const uchar_t VMEXIT_DR7_READ_STR[] = "VMEXIT_DR7_READ";
static const uchar_t VMEXIT_DR8_READ_STR[] = "VMEXIT_DR8_READ";
static const uchar_t VMEXIT_DR9_READ_STR[] = "VMEXIT_DR9_READ";
static const uchar_t VMEXIT_DR10_READ_STR[] = "VMEXIT_DR10_READ";
static const uchar_t VMEXIT_DR11_READ_STR[] = "VMEXIT_DR11_READ";
static const uchar_t VMEXIT_DR12_READ_STR[] = "VMEXIT_DR12_READ";
static const uchar_t VMEXIT_DR13_READ_STR[] = "VMEXIT_DR13_READ";
static const uchar_t VMEXIT_DR14_READ_STR[] = "VMEXIT_DR14_READ";
static const uchar_t VMEXIT_DR15_READ_STR[] = "VMEXIT_DR15_READ";
static const uchar_t VMEXIT_DR0_WRITE_STR[] = "VMEXIT_DR0_WRITE";
static const uchar_t VMEXIT_DR1_WRITE_STR[] = "VMEXIT_DR1_WRITE";
static const uchar_t VMEXIT_DR2_WRITE_STR[] = "VMEXIT_DR2_WRITE";
static const uchar_t VMEXIT_DR3_WRITE_STR[] = "VMEXIT_DR3_WRITE";
static const uchar_t VMEXIT_DR4_WRITE_STR[] = "VMEXIT_DR4_WRITE";
static const uchar_t VMEXIT_DR5_WRITE_STR[] = "VMEXIT_DR5_WRITE";
static const uchar_t VMEXIT_DR6_WRITE_STR[] = "VMEXIT_DR6_WRITE";
static const uchar_t VMEXIT_DR7_WRITE_STR[] = "VMEXIT_DR7_WRITE";
static const uchar_t VMEXIT_DR8_WRITE_STR[] = "VMEXIT_DR8_WRITE";
static const uchar_t VMEXIT_DR9_WRITE_STR[] = "VMEXIT_DR9_WRITE";
static const uchar_t VMEXIT_DR10_WRITE_STR[] = "VMEXIT_DR10_WRITE";
static const uchar_t VMEXIT_DR11_WRITE_STR[] = "VMEXIT_DR11_WRITE";
static const uchar_t VMEXIT_DR12_WRITE_STR[] = "VMEXIT_DR12_WRITE";
static const uchar_t VMEXIT_DR13_WRITE_STR[] = "VMEXIT_DR13_WRITE";
static const uchar_t VMEXIT_DR14_WRITE_STR[] = "VMEXIT_DR14_WRITE";
static const uchar_t VMEXIT_DR15_WRITE_STR[] = "VMEXIT_DR15_WRITE";
static const uchar_t VMEXIT_EXCP0_STR[] = "VMEXIT_EXCP0";
static const uchar_t VMEXIT_EXCP1_STR[] = "VMEXIT_EXCP1";
static const uchar_t VMEXIT_EXCP2_STR[] = "VMEXIT_EXCP2";
static const uchar_t VMEXIT_EXCP3_STR[] = "VMEXIT_EXCP3";
static const uchar_t VMEXIT_EXCP4_STR[] = "VMEXIT_EXCP4";
static const uchar_t VMEXIT_EXCP5_STR[] = "VMEXIT_EXCP5";
static const uchar_t VMEXIT_EXCP6_STR[] = "VMEXIT_EXCP6";
static const uchar_t VMEXIT_EXCP7_STR[] = "VMEXIT_EXCP7";
static const uchar_t VMEXIT_EXCP8_STR[] = "VMEXIT_EXCP8";
static const uchar_t VMEXIT_EXCP9_STR[] = "VMEXIT_EXCP9";
static const uchar_t VMEXIT_EXCP10_STR[] = "VMEXIT_EXCP10";
static const uchar_t VMEXIT_EXCP11_STR[] = "VMEXIT_EXCP11";
static const uchar_t VMEXIT_EXCP12_STR[] = "VMEXIT_EXCP12";
static const uchar_t VMEXIT_EXCP13_STR[] = "VMEXIT_EXCP13";
static const uchar_t VMEXIT_EXCP14_STR[] = "VMEXIT_EXCP14";
static const uchar_t VMEXIT_EXCP15_STR[] = "VMEXIT_EXCP15";
static const uchar_t VMEXIT_EXCP16_STR[] = "VMEXIT_EXCP16";
static const uchar_t VMEXIT_EXCP17_STR[] = "VMEXIT_EXCP17";
static const uchar_t VMEXIT_EXCP18_STR[] = "VMEXIT_EXCP18";
static const uchar_t VMEXIT_EXCP19_STR[] = "VMEXIT_EXCP19";
static const uchar_t VMEXIT_EXCP20_STR[] = "VMEXIT_EXCP20";
static const uchar_t VMEXIT_EXCP21_STR[] = "VMEXIT_EXCP21";
static const uchar_t VMEXIT_EXCP22_STR[] = "VMEXIT_EXCP22";
static const uchar_t VMEXIT_EXCP23_STR[] = "VMEXIT_EXCP23";
static const uchar_t VMEXIT_EXCP24_STR[] = "VMEXIT_EXCP24";
static const uchar_t VMEXIT_EXCP25_STR[] = "VMEXIT_EXCP25";
static const uchar_t VMEXIT_EXCP26_STR[] = "VMEXIT_EXCP26";
static const uchar_t VMEXIT_EXCP27_STR[] = "VMEXIT_EXCP27";
static const uchar_t VMEXIT_EXCP28_STR[] = "VMEXIT_EXCP28";
static const uchar_t VMEXIT_EXCP29_STR[] = "VMEXIT_EXCP29";
static const uchar_t VMEXIT_EXCP30_STR[] = "VMEXIT_EXCP30";
static const uchar_t VMEXIT_EXCP31_STR[] = "VMEXIT_EXCP31";
static const uchar_t VMEXIT_INTR_STR[] = "VMEXIT_INTR";
static const uchar_t VMEXIT_NMI_STR[] = "VMEXIT_NMI";
static const uchar_t VMEXIT_SMI_STR[] = "VMEXIT_SMI";
static const uchar_t VMEXIT_INIT_STR[] = "VMEXIT_INIT";
static const uchar_t VMEXIT_VINITR_STR[] = "VMEXIT_VINITR";
static const uchar_t VMEXIT_CR0_SEL_WRITE_STR[] = "VMEXIT_CR0_SEL_WRITE";
static const uchar_t VMEXIT_IDTR_READ_STR[] = "VMEXIT_IDTR_READ";
static const uchar_t VMEXIT_GDTR_READ_STR[] = "VMEXIT_GDTR_READ";
static const uchar_t VMEXIT_LDTR_READ_STR[] = "VMEXIT_LDTR_READ";
static const uchar_t VMEXIT_TR_READ_STR[] = "VMEXIT_TR_READ";
static const uchar_t VMEXIT_IDTR_WRITE_STR[] = "VMEXIT_IDTR_WRITE";
static const uchar_t VMEXIT_GDTR_WRITE_STR[] = "VMEXIT_GDTR_WRITE";
static const uchar_t VMEXIT_LDTR_WRITE_STR[] = "VMEXIT_LDTR_WRITE";
static const uchar_t VMEXIT_TR_WRITE_STR[] = "VMEXIT_TR_WRITE";
static const uchar_t VMEXIT_RDTSC_STR[] = "VMEXIT_RDTSC";
static const uchar_t VMEXIT_RDPMC_STR[] = "VMEXIT_RDPMC";
static const uchar_t VMEXIT_PUSHF_STR[] = "VMEXIT_PUSHF";
static const uchar_t VMEXIT_POPF_STR[] = "VMEXIT_POPF";
static const uchar_t VMEXIT_CPUID_STR[] = "VMEXIT_CPUID";
static const uchar_t VMEXIT_RSM_STR[] = "VMEXIT_RSM";
static const uchar_t VMEXIT_IRET_STR[] = "VMEXIT_IRET";
static const uchar_t VMEXIT_SWINT_STR[] = "VMEXIT_SWINT";
static const uchar_t VMEXIT_INVD_STR[] = "VMEXIT_INVD";
static const uchar_t VMEXIT_PAUSE_STR[] = "VMEXIT_PAUSE";
static const uchar_t VMEXIT_HLT_STR[] = "VMEXIT_HLT";
static const uchar_t VMEXIT_INVLPG_STR[] = "VMEXIT_INVLPG";
static const uchar_t VMEXIT_INVLPGA_STR[] = "VMEXIT_INVLPGA";
static const uchar_t VMEXIT_IOIO_STR[] = "VMEXIT_IOIO";
static const uchar_t VMEXIT_MSR_STR[] = "VMEXIT_MSR";
static const uchar_t VMEXIT_TASK_SWITCH_STR[] = "VMEXIT_TASK_SWITCH";
static const uchar_t VMEXIT_FERR_FREEZE_STR[] = "VMEXIT_FERR_FREEZE";
static const uchar_t VMEXIT_SHUTDOWN_STR[] = "VMEXIT_SHUTDOWN";
static const uchar_t VMEXIT_VMRUN_STR[] = "VMEXIT_VMRUN";
static const uchar_t VMEXIT_VMMCALL_STR[] = "VMEXIT_VMMCALL";
static const uchar_t VMEXIT_VMLOAD_STR[] = "VMEXIT_VMLOAD";
static const uchar_t VMEXIT_VMSAVE_STR[] = "VMEXIT_VMSAVE";
static const uchar_t VMEXIT_STGI_STR[] = "VMEXIT_STGI";
static const uchar_t VMEXIT_CLGI_STR[] = "VMEXIT_CLGI";
static const uchar_t VMEXIT_SKINIT_STR[] = "VMEXIT_SKINIT";
static const uchar_t VMEXIT_RDTSCP_STR[] = "VMEXIT_RDTSCP";
static const uchar_t VMEXIT_ICEBP_STR[] = "VMEXIT_ICEBP";
static const uchar_t VMEXIT_WBINVD_STR[] = "VMEXIT_WBINVD";
static const uchar_t VMEXIT_MONITOR_STR[] = "VMEXIT_MONITOR";
static const uchar_t VMEXIT_MWAIT_STR[] = "VMEXIT_MWAIT";
static const uchar_t VMEXIT_MWAIT_CONDITIONAL_STR[] = "VMEXIT_MWAIT_CONDITIONAL";
static const uchar_t VMEXIT_NPF_STR[] = "VMEXIT_NPF";
static const uchar_t VMEXIT_INVALID_VMCB_STR[] = "VMEXIT_INVALID_VMCB";



const uchar_t * vmexit_code_to_str(uint_t exit_code) {
  switch(exit_code) {
  case VMEXIT_CR0_READ:
    return VMEXIT_CR0_READ_STR;
  case VMEXIT_CR1_READ:
    return VMEXIT_CR1_READ_STR;
  case VMEXIT_CR2_READ:
    return VMEXIT_CR2_READ_STR;
  case VMEXIT_CR3_READ:
    return VMEXIT_CR3_READ_STR;
  case VMEXIT_CR4_READ:
    return VMEXIT_CR4_READ_STR;
  case VMEXIT_CR5_READ:
    return VMEXIT_CR5_READ_STR;
  case VMEXIT_CR6_READ:
    return VMEXIT_CR6_READ_STR;
  case VMEXIT_CR7_READ:
    return VMEXIT_CR7_READ_STR;
  case VMEXIT_CR8_READ:
    return VMEXIT_CR8_READ_STR;
  case VMEXIT_CR9_READ:
    return VMEXIT_CR9_READ_STR;
  case VMEXIT_CR10_READ:
    return VMEXIT_CR10_READ_STR;
  case VMEXIT_CR11_READ:
    return VMEXIT_CR11_READ_STR;
  case VMEXIT_CR12_READ:
    return VMEXIT_CR12_READ_STR;
  case VMEXIT_CR13_READ:
    return VMEXIT_CR13_READ_STR;
  case VMEXIT_CR14_READ:
    return VMEXIT_CR14_READ_STR;
  case VMEXIT_CR15_READ:
    return VMEXIT_CR15_READ_STR;
  case VMEXIT_CR0_WRITE:
    return VMEXIT_CR0_WRITE_STR;
  case VMEXIT_CR1_WRITE:
    return VMEXIT_CR1_WRITE_STR;
  case VMEXIT_CR2_WRITE:
    return VMEXIT_CR2_WRITE_STR;
  case VMEXIT_CR3_WRITE:
    return VMEXIT_CR3_WRITE_STR;
  case VMEXIT_CR4_WRITE:
    return VMEXIT_CR4_WRITE_STR;
  case VMEXIT_CR5_WRITE:
    return VMEXIT_CR5_WRITE_STR;
  case VMEXIT_CR6_WRITE:
    return VMEXIT_CR6_WRITE_STR;
  case VMEXIT_CR7_WRITE:
    return VMEXIT_CR7_WRITE_STR;
  case VMEXIT_CR8_WRITE:
    return VMEXIT_CR8_WRITE_STR;
  case VMEXIT_CR9_WRITE:
    return VMEXIT_CR9_WRITE_STR;
  case VMEXIT_CR10_WRITE:
    return VMEXIT_CR10_WRITE_STR;
  case VMEXIT_CR11_WRITE:
    return VMEXIT_CR11_WRITE_STR;
  case VMEXIT_CR12_WRITE:
    return VMEXIT_CR12_WRITE_STR;
  case VMEXIT_CR13_WRITE:
    return VMEXIT_CR13_WRITE_STR;
  case VMEXIT_CR14_WRITE:
    return VMEXIT_CR14_WRITE_STR;
  case VMEXIT_CR15_WRITE:
    return VMEXIT_CR15_WRITE_STR;
  case VMEXIT_DR0_READ:
    return VMEXIT_DR0_READ_STR;
  case VMEXIT_DR1_READ:
    return VMEXIT_DR1_READ_STR;
  case VMEXIT_DR2_READ:
    return VMEXIT_DR2_READ_STR;
  case VMEXIT_DR3_READ:
    return VMEXIT_DR3_READ_STR;
  case VMEXIT_DR4_READ:
    return VMEXIT_DR4_READ_STR;
  case VMEXIT_DR5_READ:
    return VMEXIT_DR5_READ_STR;
  case VMEXIT_DR6_READ:
    return VMEXIT_DR6_READ_STR;
  case VMEXIT_DR7_READ:
    return VMEXIT_DR7_READ_STR;
  case VMEXIT_DR8_READ:
    return VMEXIT_DR8_READ_STR;
  case VMEXIT_DR9_READ:
    return VMEXIT_DR9_READ_STR;
  case VMEXIT_DR10_READ:
    return VMEXIT_DR10_READ_STR;
  case VMEXIT_DR11_READ:
    return VMEXIT_DR11_READ_STR;
  case VMEXIT_DR12_READ:
    return VMEXIT_DR12_READ_STR;
  case VMEXIT_DR13_READ:
    return VMEXIT_DR13_READ_STR;
  case VMEXIT_DR14_READ:
    return VMEXIT_DR14_READ_STR;
  case VMEXIT_DR15_READ:
    return VMEXIT_DR15_READ_STR;
  case VMEXIT_DR0_WRITE:
    return VMEXIT_DR0_WRITE_STR;
  case VMEXIT_DR1_WRITE:
    return VMEXIT_DR1_WRITE_STR;
  case VMEXIT_DR2_WRITE:
    return VMEXIT_DR2_WRITE_STR;
  case VMEXIT_DR3_WRITE:
    return VMEXIT_DR3_WRITE_STR;
  case VMEXIT_DR4_WRITE:
    return VMEXIT_DR4_WRITE_STR;
  case VMEXIT_DR5_WRITE:
    return VMEXIT_DR5_WRITE_STR;
  case VMEXIT_DR6_WRITE:
    return VMEXIT_DR6_WRITE_STR;
  case VMEXIT_DR7_WRITE:
    return VMEXIT_DR7_WRITE_STR;
  case VMEXIT_DR8_WRITE:
    return VMEXIT_DR8_WRITE_STR;
  case VMEXIT_DR9_WRITE:
    return VMEXIT_DR9_WRITE_STR;
  case VMEXIT_DR10_WRITE:
    return VMEXIT_DR10_WRITE_STR;
  case VMEXIT_DR11_WRITE:
    return VMEXIT_DR11_WRITE_STR;
  case VMEXIT_DR12_WRITE:
    return VMEXIT_DR12_WRITE_STR;
  case VMEXIT_DR13_WRITE:
    return VMEXIT_DR13_WRITE_STR;
  case VMEXIT_DR14_WRITE:
    return VMEXIT_DR14_WRITE_STR;
  case VMEXIT_DR15_WRITE:
    return VMEXIT_DR15_WRITE_STR;
  case VMEXIT_EXCP0:
    return VMEXIT_EXCP0_STR;
  case VMEXIT_EXCP1:
    return VMEXIT_EXCP1_STR;
  case VMEXIT_EXCP2:
    return VMEXIT_EXCP2_STR;
  case VMEXIT_EXCP3:
    return VMEXIT_EXCP3_STR;
  case VMEXIT_EXCP4:
    return VMEXIT_EXCP4_STR;
  case VMEXIT_EXCP5:
    return VMEXIT_EXCP5_STR;
  case VMEXIT_EXCP6:
    return VMEXIT_EXCP6_STR;
  case VMEXIT_EXCP7:
    return VMEXIT_EXCP7_STR;
  case VMEXIT_EXCP8:
    return VMEXIT_EXCP8_STR;
  case VMEXIT_EXCP9:
    return VMEXIT_EXCP9_STR;
  case VMEXIT_EXCP10:
    return VMEXIT_EXCP10_STR;
  case VMEXIT_EXCP11:
    return VMEXIT_EXCP11_STR;
  case VMEXIT_EXCP12:
    return VMEXIT_EXCP12_STR;
  case VMEXIT_EXCP13:
    return VMEXIT_EXCP13_STR;
  case VMEXIT_EXCP14:
    return VMEXIT_EXCP14_STR;
  case VMEXIT_EXCP15:
    return VMEXIT_EXCP15_STR;
  case VMEXIT_EXCP16:
    return VMEXIT_EXCP16_STR;
  case VMEXIT_EXCP17:
    return VMEXIT_EXCP17_STR;
  case VMEXIT_EXCP18:
    return VMEXIT_EXCP18_STR;
  case VMEXIT_EXCP19:
    return VMEXIT_EXCP19_STR;
  case VMEXIT_EXCP20:
    return VMEXIT_EXCP20_STR;
  case VMEXIT_EXCP21:
    return VMEXIT_EXCP21_STR;
  case VMEXIT_EXCP22:
    return VMEXIT_EXCP22_STR;
  case VMEXIT_EXCP23:
    return VMEXIT_EXCP23_STR;
  case VMEXIT_EXCP24:
    return VMEXIT_EXCP24_STR;
  case VMEXIT_EXCP25:
    return VMEXIT_EXCP25_STR;
  case VMEXIT_EXCP26:
    return VMEXIT_EXCP26_STR;
  case VMEXIT_EXCP27:
    return VMEXIT_EXCP27_STR;
  case VMEXIT_EXCP28:
    return VMEXIT_EXCP28_STR;
  case VMEXIT_EXCP29:
    return VMEXIT_EXCP29_STR;
  case VMEXIT_EXCP30:
    return VMEXIT_EXCP30_STR;
  case VMEXIT_EXCP31:
    return VMEXIT_EXCP31_STR;
  case VMEXIT_INTR:
    return VMEXIT_INTR_STR;
  case VMEXIT_NMI:
    return VMEXIT_NMI_STR;
  case VMEXIT_SMI:
    return VMEXIT_SMI_STR;
  case VMEXIT_INIT:
    return VMEXIT_INIT_STR;
  case VMEXIT_VINITR:
    return VMEXIT_VINITR_STR;
  case VMEXIT_CR0_SEL_WRITE:
    return VMEXIT_CR0_SEL_WRITE_STR;
  case VMEXIT_IDTR_READ:
    return VMEXIT_IDTR_READ_STR;
  case VMEXIT_GDTR_READ:
    return VMEXIT_GDTR_READ_STR;
  case VMEXIT_LDTR_READ:
    return VMEXIT_LDTR_READ_STR;
  case VMEXIT_TR_READ:
    return VMEXIT_TR_READ_STR;
  case VMEXIT_IDTR_WRITE:
    return VMEXIT_IDTR_WRITE_STR;
  case VMEXIT_GDTR_WRITE:
    return VMEXIT_GDTR_WRITE_STR;
  case VMEXIT_LDTR_WRITE:
    return VMEXIT_LDTR_WRITE_STR;
  case VMEXIT_TR_WRITE:
    return VMEXIT_TR_WRITE_STR;
  case VMEXIT_RDTSC:
    return VMEXIT_RDTSC_STR;
  case VMEXIT_RDPMC:
    return VMEXIT_RDPMC_STR;
  case VMEXIT_PUSHF:
    return VMEXIT_PUSHF_STR;
  case VMEXIT_POPF:
    return VMEXIT_POPF_STR;
  case VMEXIT_CPUID:
    return VMEXIT_CPUID_STR;
  case VMEXIT_RSM:
    return VMEXIT_RSM_STR;
  case VMEXIT_IRET:
    return VMEXIT_IRET_STR;
  case VMEXIT_SWINT:
    return VMEXIT_SWINT_STR;
  case VMEXIT_INVD:
    return VMEXIT_INVD_STR;
  case VMEXIT_PAUSE:
    return VMEXIT_PAUSE_STR;
  case VMEXIT_HLT:
    return VMEXIT_HLT_STR;
  case VMEXIT_INVLPG:
    return VMEXIT_INVLPG_STR;
  case VMEXIT_INVLPGA:
    return VMEXIT_INVLPGA_STR;
  case VMEXIT_IOIO:
    return VMEXIT_IOIO_STR;
  case VMEXIT_MSR:
    return VMEXIT_MSR_STR;
  case VMEXIT_TASK_SWITCH:
    return VMEXIT_TASK_SWITCH_STR;
  case VMEXIT_FERR_FREEZE:
    return VMEXIT_FERR_FREEZE_STR;
  case VMEXIT_SHUTDOWN:
    return VMEXIT_SHUTDOWN_STR;
  case VMEXIT_VMRUN:
    return VMEXIT_VMRUN_STR;
  case VMEXIT_VMMCALL:
    return VMEXIT_VMMCALL_STR;
  case VMEXIT_VMLOAD:
    return VMEXIT_VMLOAD_STR;
  case VMEXIT_VMSAVE:
    return VMEXIT_VMSAVE_STR;
  case VMEXIT_STGI:
    return VMEXIT_STGI_STR;
  case VMEXIT_CLGI:
    return VMEXIT_CLGI_STR;
  case VMEXIT_SKINIT:
    return VMEXIT_SKINIT_STR;
  case VMEXIT_RDTSCP:
    return VMEXIT_RDTSCP_STR;
  case VMEXIT_ICEBP:
    return VMEXIT_ICEBP_STR;
  case VMEXIT_WBINVD:
    return VMEXIT_WBINVD_STR;
  case VMEXIT_MONITOR:
    return VMEXIT_MONITOR_STR;
  case VMEXIT_MWAIT:
    return VMEXIT_MWAIT_STR;
  case VMEXIT_MWAIT_CONDITIONAL:
    return VMEXIT_MWAIT_CONDITIONAL_STR;
  case VMEXIT_NPF:
    return VMEXIT_NPF_STR;
  case VMEXIT_INVALID_VMCB:
    return VMEXIT_INVALID_VMCB_STR;
  }
  return NULL;
}
