#include <stdio.h>
#include <stdint.h>
#include <string.h>

// External assembly function
extern void pf_adaptor_asm_fix_ist_err_code(void);

// Mock page fault handler that the assembly will jump to
// This needs to match the symbol referenced in the assembly
void* pf_adaptor_asm_exc_page_fault = NULL;

// Structure to simulate exception frame
typedef struct {
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} exception_frame_t;

uint64_t fake_target_stack[10];

// Test function that calls the assembly with a simulated exception frame
void test_assembly_function(void) {
    printf("Calling assembly function...\n");
    
    // Allocate a fake exception frame on the stack
    // The assembly expects: error_code, rip, cs, rflags, rsp, ss

    
    // Call the assembly - it will:
    // 1. Push registers
    // 2. Copy the frame
    // 3. Jump to pf_adaptor_asm_exc_page_fault
    __asm__ __volatile__(
        "call pf_adaptor_asm_fix_ist_err_code\n"
        : : : "memory"
    );
}

int main() {
    printf("Page Fault Adaptor Assembly Test\n");
    printf("==================================\n\n");
    
    printf("Assembly function address: %p\n", (void*)pf_adaptor_asm_fix_ist_err_code);
    
    printf("\nNOTE: This assembly code is designed for kernel mode (IST stacks).\n");
    printf("Attempting to test in userspace with mocked exception frame...\n\n");
    
    // Set the handler to point to after the test
    pf_adaptor_asm_exc_page_fault = &&after_assembly_test;
    
    printf("Calling assembly test function...\n");
    
#ifdef ERR_CODE

    uint64_t fake_frame[6];
    fake_frame[0] = 0x00;           // error_code
    fake_frame[1] = 0x12345678;     // rip (fake)
    fake_frame[2] = 0x10;           // cs
    fake_frame[3] = 0x202;          // rflags
    fake_frame[4] = (uint64_t)&fake_target_stack[9]; // rsp (negative to trigger kernel path)
    // fake_frame[4] = 0x8000000000000000ULL; // rsp (negative to trigger kernel path)
    fake_frame[5] = 0x18;           // ss

    __asm__ __volatile__(
        "jmp pf_adaptor_asm_fix_ist_err_code\n"
        : : : "memory"
    );
    
#else
    uint64_t fake_frame[5];
    fake_frame[0] = 0x12345678;     // rip (fake)
    fake_frame[1] = 0x10;           // cs
    fake_frame[2] = 0x202;          // rflags
    fake_frame[3] = (uint64_t)&fake_target_stack[9]; // rsp (negative to trigger kernel path)
    // fake_frame[3] = 0x8000000000000000ULL; // rsp (negative to trigger kernel path)
    fake_frame[4] = 0x18;           // ss

    __asm__ __volatile__(
        "jmp pf_adaptor_asm_fix_ist_no_err_code\n"
        : : : "memory"
    );
#endif



after_assembly_test:
    // printf("Successfully jumped to exit point from assembly!\n");
    // printf("Test completed.\n");
    
    return 0;
}
