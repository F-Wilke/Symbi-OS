#include <stdio.h>
#include <string.h>
#include <cpuid.h>
#include <stdlib.h>

#include "LINF/sym_all.h"

#include "kallsyms.h"
#include "load_mod.h"

static int module_loaded = 0;

//macro to define a function as an ifunc and to define a simple resolver that calls the base_resolver
#define DECLARE_IFUNC(name, rettype, args) \
    rettype name args __attribute__((ifunc(#name "_resolver"))); \
    static rettype (*name##_resolver(void)) args { \
        return (rettype (*) args) base_resolver(#name); \
    }  


static void* base_resolver(char* symbol_name) {
    printf("base_resolver: Resolving symbol %s\n", symbol_name);
    sym_elevate();

    if (!module_loaded) {
        int rc;
        
        printf("Loading kallsyms module\n");
        
        rc = load_included_module();
        if (rc != 0) {
            printf("Failed to load kallsyms module: %d\n", rc);
            exit(1);
        }
        printf("Loaded kallsyms module\n");

        module_loaded = 1;
    }
    
    unsigned long addr = kallsyms_lookup_name(symbol_name);
    printf("Resolved symbol %s to address %p\n", symbol_name, (void*)addr);
    sym_lower();
    
    if (addr == 0) {
        printf("Symbol %s not found!\n", symbol_name);
        //exit program! We have a linkage problem
        exit(1);
    }
    return (void*)addr;
}


DECLARE_IFUNC(kernel_add, int, (int a, int b)) 


DECLARE_IFUNC(current_pid, int, (void)) 



// Two different implementations of a string copy function
static char* strcpy_generic(char* dest, const char* src) {
    printf("Using generic strcpy implementation\n");
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

static char* strcpy_optimized(char* dest, const char* src) {
    printf("Using optimized strcpy implementation\n");
    // This is just a demonstration - in reality this would use
    // SIMD instructions or other CPU-specific optimizations
    return strcpy(dest, src);
}

// Resolver function that decides which implementation to use
static char* (*strcpy_resolver(void))(char*, const char*) {
    unsigned int eax, ebx, ecx, edx;

#ifdef E_EARLY
    printf("strcpy_resolver: calling elevate\n");
    sym_elevate();
    printf("strcpy_resolver: called elevate\n");

    sym_lower();
    printf("strcpy_resolver: called sym_lower\n");
#endif
    // Check if we have CPUID support and can detect CPU features
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        // Check for SSE2 support (bit 26 in EDX)
        if (edx & (1 << 26)) {
            printf("CPU supports SSE2, using optimized version\n");
            return strcpy_optimized;
        }
    }
    
    printf("Using generic version\n");
    return strcpy_generic;
}

// Define the ifunc - this will call strcpy_resolver at runtime
// to determine which implementation to use
char* my_strcpy(char* dest, const char* src) __attribute__((ifunc("strcpy_resolver")));

int main() {
    char buffer[100];
    const char* source = "Hello, ifunc world!";

    printf("main: calling elevate\n");
    sym_elevate();
    printf("main: called elevate\n");

    //run kernel_add
    int sum = kernel_add(3, 4);
    printf("kernel_add(3, 4) = %d\n", sum);

    //run current_pid
    int pid = current_pid();
    printf("current_pid() = %d\n", pid);

    sym_lower();
    printf("main: called sym_lower\n");

    // printf("Demonstrating ifunc usage:\n");
    
    // // The first call will trigger the resolver
    // my_strcpy(buffer, source);
    // printf("Copied string: %s\n", buffer);
    
    // // Subsequent calls will use the already resolved function
    // my_strcpy(buffer, "Second call");
    // printf("Copied string: %s\n", buffer);
    
    printf("DONE\n");
    return 0;
}