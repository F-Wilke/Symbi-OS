#include <stdio.h>
#include <string.h>
#include "L0/sym_lib.h"
#include <stdlib.h>
#include "load_mod.h"

//DECLARE_IFUNC(name, rettype, args)
//these functions are defined in the kernel module we will load
DECLARE_IFUNC(swap_phys_pages, void, (void* vaddr1, void* vaddr2)) 


//native kernel function that will be resolved at load time
extern int _printk(const char *fmt, ...);


int main() {
    //allocate two page sized buffers aligned to page boundaries
    char* str1 = (char*)aligned_alloc(4096, 4096);
    char* str2 = (char*)aligned_alloc(4096, 4096);

    strcpy(str1, "This is string 1");
    strcpy(str2, "This is string 2");

    printf("Before swap:\n");
    printf("str1: %s\n", str1);
    printf("str2: %s\n", str2);


    sym_elevate();

    //run print_idt_entries


    swap_phys_pages(str1, str2);
    printf("After swap:\n");
    printf("str1: %s\n", str1);
    printf("str2: %s\n", str2);

    sym_lower();

    free(str1);
    free(str2);
    
    printf("DONE\n");
    return 0;
}

//sudo LD_BIND_NOW=1 LD_LIBRARY_PATH=../../Symlib/dynam_build:. ./main