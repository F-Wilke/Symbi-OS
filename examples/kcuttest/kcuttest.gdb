define efec
   if $argc == 0
       echo requires stack base address.\n
   else
      set $stkbase=$arg0
      set $efec=((unsigned long *)$stkbase)[0]
      set $efrip=((unsigned long *)$stkbase)[1]
      set $efcs=((unsigned long *)$stkbase)[2]
      set $efrflags=((unsigned long *)$stkbase)[3]
      set $efrsp=((unsigned long *)$stkbase)[4]
      set $efss=((unsigned long *)$stkbase)[5]
      printf "stk[0]:%016lx = EC:\t0x%016lx\nstk[1]:%016lx = RIP:\t0x%016lx\nstk[2]:%016lx = CS:\t0x%016lx\nstk[3]:%016lx = RFGS:\t0x%016lx\nstk[4]:%016lx = RSP:\t0x%016lx\nstk[5]:%016lx = SS:\t0x%016lx\n",$stkbase+0*8,$efec,$stkbase+1*8,$efrip,$stkbase+2*8,$efcs,$stkbase+3*8,$efrflags,$stkbase+4*8,$efrsp,$stkbase+5*8,$efss
   end      
end

define ef
   if $argc == 0
       echo requires stack base address.\n
   else
      set $stkbase=$arg0
      set $efrip=((unsigned long *)$stkbase)[0]
      set $efcs=((unsigned long *)$stkbase)[1]
      set $efrflags=((unsigned long *)$stkbase)[2]
      set $efrsp=((unsigned long *)$stkbase)[3]
      set $efss=((unsigned long *)$stkbase)[4]	
      printf "stk[0]:%016lx = RIP:\t0x%016lx\nstk[1]:%016lx = CS:\t0x%016lx\nstk[2]:%016lx = RFGS:\t0x%016lx\nstk[3]:%016lx = RSP:\t0x%016lx\nstk[4]:%016lx = SS:\t0x%016lx\n",$stkbase+0*8,$efrip,$stkbase+1*8,$efcs,$stkbase+2*8,$efrflags,$stkbase+3*8,$efrsp,$stkbase+4*8,$efss
   end      
end

define dstk
   set $stkcnt=8
   set $i=0
   if $argc == 0
       echo requires stack base address.\n
   else
      if $argc == 2
        set $stkcnt=$arg1
      end
       set $stkbase=$arg0
       while $i < $stkcnt
          printf "stk[%d]:%016lx = %016lx\n", $i, ($stkbase + (8*$i)),  *((unsigned long *)($stkbase + (8*$i))) 
          set $i=$i +1
       end 
   end
end


### functions to work with ists assumes ef_stack debugging on

define istsetup
  set $ist0=ef_stacks[0] + 0x1000
  set $ist1=ef_stacks[1] + 0x1000
  set $ist2=ef_stacks[2] + 0x1000
  set $ist3=ef_stacks[3] + 0x1000
  p /x ef_stacks
  p /x { $ist0, $ist1, $ist2, $ist3 }
end
document istsetup
Call this first to set ist0-ist3 convience variables for first four core
These will make using the other ist commands easier (see their documentation)
end

define ist
  set $istcpu=0
  if $argc > 0
    set $istcpu=$arg0
  end
  p /x ef_stacks[$istcpu]
end
document ist
Pass cpu to lookup ist stack pointer for a given cpu (ist + one page). Eg.
  ist 0
  ist 2
end


define istbottom
  set $istcpu=0
  if $argc > 0
    set $istcpu=$arg0
  end
  p /x ef_stacks[$istcpu] + 0x1000
end
document istbottom
Pass cpu to lookup ist stack pointer for a given cpu (ist + one page).  Eg.
  istbottom 0
  istbottom 2
end

define iststk
    if $argc>1
      set $istcnt=$arg1
    else
      set $istcnt=20 
    end
    set $isttop=$arg0 - (8*$istcnt)
    dstk $isttop $istcnt
end
document iststk
Dump some number of quads of the specified ist stack with respect to the bottom
default is 20 quads otherwise second argument.  Eg.
   iststk $ist0
   iststk $ist1 512 
end

define istefec
   set $istbase=($arg0-(6*8))
   efec $istbase
end
document istefec
Dump exception frame with error code at the end of the ist page. Eg.
  istefec $ist2
end

define istef
   set $istbase=($arg0-(5*8))
   efec $istbase
end
document istef
Dump exception frame with NO error code at the end of the ist page. Eg.
  istef $ist2
end

define istefrip
  if $argc>1
     set $isticnt=$arg1
  else
     set $isticnt=10
  end
  set $istrip=((unsigned long *)$arg0)[-5]
  printf "ist: %016lx.pc=%016lx\n", $arg0, $istrip
  eval "x/%di $istrip", $isticnt
end
document istefrip
Dump the value of the rip in the specified ist ef and disassmble
instructions at that location.  Eg.
  istefrip $ist3
  istefrip $ist3 4
end

define istefrsp
  set $istrsp=((unsigned long *)$arg0)[-2]
  printf "ist: %016lx.rsp=%016lx\n", $arg0, $istrsp
  if $argc>1 
    dstk $istrsp $arg1
  else
    dstk $istrsp
  end
end
document istefrsp
Dump the value of the rsp in specified ist ef and dump values at
that location.  Eg.
istefrsp $ist0 
istefrsp $ist0 4 
end

define modsetup
  #
  # Walk the kernel module list for the named module and load its symbols.
  # Usage:   modsetup <module_name> <source_dir>
  # Example: modsetup idt_adaptor ../examples/kcuttest
  #
  # The .ko is expected at: <source_dir>/<module_name>.ko
  #

  # Byte offset of the 'list' field inside struct module
  set $modoff   = (unsigned long)&((struct module *)0)->list

  # Circular list sentinal (the global 'modules' list_head itself)
  set $listhead = (struct list_head *)&modules
  set $listptr  = modules.next
  set $found    = 0

  # Walk until we wrap back to the list head
  while $listptr != $listhead
    set $modptr = (struct module *)((char *)$listptr - $modoff)
    if $_streq($modptr->name, "$arg0")
      set $found = 1
      loop_break
    end
    set $listptr = $listptr->next
  end

  if $found
    printf "Found module: %s\n", $modptr->name
    set $modtext   = $modptr->mem[0].base
    set $moddata   = $modptr->mem[1].base
    set $modrodata = $modptr->mem[2].base
    dir $arg1
    dir $arg1/.ext
    add-symbol-file $arg1/$arg0.ko $modtext -s .data $moddata
  else
    printf "Module '%s' is not loaded.\n", "$arg0"
  end
end
document modsetup
  Load debug symbols for a kernel module found by name in the module list.

  Usage:
    modsetup <module_name> <source_dir>

  Arguments:
    module_name  Name exactly as it appears in /proc/modules (e.g. idt_adaptor)
    source_dir   Path to the module build tree    (e.g. ../examples/kcuttest)

  The command walks the kernel 'modules' linked list, finds the entry whose
  name matches module_name, and calls add-symbol-file with the text/data
  base addresses extracted from module->mem[].

  The .ko file is assumed to live at:  <source_dir>/.ext/<module_name>.ko
end

define kcutmodsetup
       modsetup kcuttest ../examples/kcuttest
       modsetup kcutevac ../examples/kcutevac
       modsetup kcuttcp  ../examples/kcuttcp
       modsetup kcutnmi  ../examples/kcutnmi
       modsetup kcutef   ../examples/kcutef	
       modsetup kcutidt  ../examples/kcutidt
end

define setup
   printf "SETUP: Make sure that you are in your Symbi-OS linux directory and you are debugging vmlinux\n"
   printf "       Be sure:\n"
   printf "          1) gdbinit: autoload full path to linux/scripts/gdb/vmdlinux-gdb.py"
   printf "          2) gdbinit: ensure substitute-path is set as needed: eg. /home/user /home/jappavoo"
   printf "          3) fixlinks: be sure any full path linux to source are fixed locally (see fixlinks)"
  
   set print pretty
   set radix 10
   target remote :4242
#   lx-symbols ../examples
#   hb native_load_idt
#   hb ef_stack_badword_error
end
