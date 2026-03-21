#ifndef __IDT_ADAPTOR_KH__
#define __IDT_ADAPTOR_KH__

gate_desc * idt_adaptor_clone(void);
void        idt_adaptor_activate(const gate_desc *idt);
void        idt_adaptor_release(const gate_desc *idt);

static inline void
read_gate(gate_desc *idt, int n, gate_desc *gate)
{
  write_idt_entry(gate, 0, &(idt[n])); 
}

static inline void
gate_set_offset(gate_desc *gate, unsigned long addr)
{
  gate->offset_low      = (u16) addr;
  gate->offset_middle   = (u16) (addr >> 16);
#ifdef CONFIG_X86_64
  gate->offset_high     = (u32) (addr >> 32);
#endif
}

#endif
