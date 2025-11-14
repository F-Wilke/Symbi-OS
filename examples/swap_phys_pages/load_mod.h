#include <stdint.h>
#include <stdio.h>
#include <elf.h>
#include "LINF/sym_all.h"
#include "L1/stack_switch.h"

#define	ENOMEM		12	/* Out of memory */
#define	EFAULT		14	/* Bad address */

#define PAGE_SHIFT 12 //just for this host!
#define PAGE_SIZE  (1UL << PAGE_SHIFT)
#define COPY_CHUNK_SIZE (16*PAGE_SIZE)


#define PRINTF(fmt, ...) do { \
    printf(fmt, ##__VA_ARGS__); \
} while(0)



//macro to define a function as an ifunc and to define a simple resolver that calls the base_resolver
#define DECLARE_IFUNC(name, rettype, args) \
    rettype name args __attribute__((ifunc(#name "_resolver"))); \
    static rettype (*name##_resolver(void)) args { \
        return (rettype (*) args) base_resolver(#name); \
    }  



// These symbols are provided by the linker (from greeter.o)
extern const uint8_t _binary_greeter_ko_start[];
extern const uint8_t _binary_greeter_ko_end[];
extern const uint8_t _binary_greeter_ko_size;




int load_included_module();
void* base_resolver(char* symbol_name);


// extern int load_module(struct load_info *info, char *uargs,int flags);
extern int __x64_sys_init_module(void* args);




