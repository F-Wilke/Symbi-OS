#include <stdio.h>
#include <string.h>
#include "L0/sym_lib.h"

#include "load_mod.h"

//DECLARE_IFUNC(name, rettype, args)
//these functions are defined in the kernel module we will load
DECLARE_IFUNC(print_idt_entries, void, (void)) 


//native kernel function that will be resolved at load time
extern int _printk(const char *fmt, ...);


int main() {

    printf("main: calling elevate\n");
    sym_elevate();
    printf("main: called elevate\n");

    //run print_idt_entries
    print_idt_entries();
    printf("printf: called print_idt_entries()\n");

    sym_lower();
    
    printf("DONE\n");
    return 0;
}

//sudo LD_BIND_NOW=1 LD_LIBRARY_PATH=../../Symlib/dynam_build:. ./main