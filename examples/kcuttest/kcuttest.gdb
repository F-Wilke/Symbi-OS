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
  p modules
  set $modoff=(char *)&((struct module *)0)->list
  set $modptr=(struct module *)((char *)modules.next-$modoff)
  p $modoff 
  p $modptr
  p $modptr->name
  set $modtext=$modptr->mem[0].base
  set $moddata=$modptr->mem[1].base
  set $modroata=$modptr->mem[2].base
  dir ../examples/kcuttest
  dir ../examples/kcuttest/.ext
  add-symbol-file ../examples/kcuttest/.ext/idt_adaptor.ko $modtext -s .data $moddata
end

define setup
   set print pretty
   set radix 10
   target remote :4242
   hb native_load_idt
   hb ef_stack_badword_error
end
