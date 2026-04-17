#include <stdio.h>
#include <stdint.h>

/* Helper macro for visibility and extern declaration */
#define IMPORT __attribute__((visibility("default"))) extern

// Functions
IMPORT void global_func(void);
IMPORT void weak_func(void) __attribute__((weak));

// Data types
IMPORT int      global_data;
IMPORT const int global_rodata;
IMPORT char     global_bss[256];
IMPORT int      weak_data __attribute__((weak));

// Absolute constant
// Note: To get the value of an absolute symbol, you take its ADDRESS
IMPORT char absolute_val; 

int main() {
    printf("--- Function Addresses ---\n");
    printf("global_func:   %p\n", (void*)global_func);
    printf("weak_func:     %p (Weak)\n", (void*)weak_func);

    printf("\n--- Data Addresses ---\n");
    printf("global_data:   %p\n", (void*)&global_data);
    printf("global_rodata: %p (Read-Only)\n", (void*)&global_rodata);
    printf("global_bss:    %p (BSS)\n", (void*)global_bss);
    printf("weak_data:     %p (Weak Data)\n", (void*)&weak_data);

    printf("\n--- Absolute Constant ---\n");
    // Since 'absolute_val' is just a symbol at address 0xDEADBEEF,
    // we cast the pointer to that address to a uintptr_t to see the value.
    printf("absolute_val:  0x%lx\n", (uintptr_t)&absolute_val);

    return 0;
}
