define modsetup
  p modules
  set $modoff=(char *)&((struct module *)0)->list
  set $modptr=(struct module *)((char *)modules.next-$modoff)
  p $modoff 
  p $modptr
  p $modptr->name
  set $modtext=$modptr->mem[0].base
  set $moddata=$modptr->mem[1].base
  set $modroata=$modptr->mem[2].base
  dir examples/kcuttest
  dir examples/kcuttest/.ext
  add-symbol-file examples/kcuttest/.ext/idt_adaptor.ko $modtext -s .data $moddata
end

define setup
   set print pretty
   set radix 16
   target remote :4242
   hb stop_machine
end
