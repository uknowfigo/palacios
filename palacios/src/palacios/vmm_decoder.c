#include <palacios/vmm_decoder.h>



/* The full blown instruction parser... */
int v3_parse_instr(struct guest_info * info,
		   char * instr_ptr,
		   uint_t * instr_length, 
		   struct x86_operand * src_operand,
		   struct x86_operand * dst_operand,
		   struct x86_operand * extra_operand) {

  V3_Assert(src_operand != NULL);
  V3_Assert(dst_operand != NULL);
  V3_Assert(extra_operand != NULL);
  V3_Assert(instr_length != NULL);
  V3_Assert(info != NULL);

  
  // Ignore prefixes for now
  while (is_prefix_byte(*instr)) {
    instr++;
    *instr_length++;
  }


  // Opcode table lookup, see xen/kvm





  return 0;
}