#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <setjmp.h>

#include "pfadaptor.kh"

#define PAGE_SIZE 4096

static sigjmp_buf fault_jmp;
static volatile int fault_caught = 0;

static void segv_handler(int sig, siginfo_t *si, void *unused)
{
    pid_t mypid = getpid();
    fault_caught = 1;
    printf("\t%d: Caught signal %d at address %p\n", mypid, sig, si->si_addr);
    siglongjmp(fault_jmp, 1);
}

/**
 * Performs a complex memory mapping test:
 * 1. mmap a file
 * 2. Use mincore to find two consecutive non-present pages A and B
 * 3. Touch page A to make it present
 * 4. Set stack pointer to 2 giant words (16 bytes) above the border between A and B
 * 5. Perform a write access to another unmapped page
 */
int mmap_stack_test(unsigned operation)
{
    pid_t mypid = getpid();
    int fd = -1;
    void *mapped_area = NULL;
    size_t map_size = 2 * 1024 * 1024; // 2 MB to ensure we have unmapped pages
    unsigned char *vec = NULL;
    size_t num_pages;
    int page_A_idx = -1, page_B_idx = -1, page_C_idx = -1;
    void *page_A, *page_B, *page_C;
    int ret = 0;
    struct sigaction sa;
    
    printf("%d: MMAP STACK TEST: BEGIN", mypid);
    
    // Set up signal handler for SIGSEGV
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }
    
    // Create a temporary file for mapping
    char temp_file[] = "/tmp/mmap_test_XXXXXX";
    fd = mkstemp(temp_file);
    if (fd < 0) {
        perror("mkstemp");
        return -1;
    }
    unlink(temp_file); // Unlink immediately so it's cleaned up on exit
    
    // Make the file large enough
    if (ftruncate(fd, map_size) < 0) {
        perror("ftruncate");
        close(fd);
        return -1;
    }
    
    // mmap the file with MAP_PRIVATE so pages aren't immediately resident
    mapped_area = mmap(NULL, map_size, PROT_READ | PROT_WRITE, 
                       MAP_ANON | MAP_PRIVATE, -1, 0);
    if (mapped_area == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    
    printf("\t%d: Mapped area at %p, size %zu bytes (%zu pages)\n", 
           mypid, mapped_area, map_size, map_size / PAGE_SIZE);
    
    // Allocate vector for mincore (one byte per page)
    num_pages = map_size / PAGE_SIZE;
    vec = malloc(num_pages);
    if (!vec) {
        perror("malloc");
        ret = -1;
        goto cleanup;
    }
    
    // Check which pages are resident using mincore
    if (mincore(mapped_area, map_size, vec) < 0) {
        perror("mincore");
        ret = -1;
        goto cleanup;
    }
    
    // Find two consecutive non-present pages (A and B)
    // mincore sets bit 0 if page is resident
    for (size_t i = num_pages - 3; i > 0; i--) {
        if (!(vec[i] & 1) && !(vec[i+1] & 1) && !(vec[i+2] & 1)) {
            page_A_idx = i;
            page_B_idx = i + 1;
            page_C_idx = i + 2;
            break;
        }
    }
    
    if (page_A_idx < 0) {
        printf("\t%d: Could not find two consecutive non-present pages\n", mypid);
        ret = -1;
        goto cleanup;
    }
    
    page_A = (char *)mapped_area + (page_A_idx * PAGE_SIZE);
    page_B = (char *)mapped_area + (page_B_idx * PAGE_SIZE);
    page_C = (char *)mapped_area + (page_C_idx * PAGE_SIZE);
    
    printf("\t%d: Found consecutive non-present pages:\n", mypid);
    printf("\t\tPage A at %p (index %d)\n", page_A, page_A_idx);
    printf("\t\tPage B at %p (index %d)\n", page_B, page_B_idx);
    printf("\t\tPage C at %p (index %d)\n", page_C, page_C_idx);
    printf("\t\tBorder between A and B at %p\n", page_B);
    
    // Touch page A to make it present
    printf("\t%d: Touching page B...\n", mypid);
    *(volatile char *)page_B = 0x42;
    printf("\t%d: Page B touched (wrote 0x42)\n", mypid);

    unsigned long df_cnt=0, pf_cnt=0;

    pf_cnt = pf_adaptor_pf_cnt_get();
    df_cnt = pf_adaptor_df_cnt_get();

    printf("\t%d: current df counter %lu, pf counter: %lu\n", mypid, df_cnt, pf_cnt);
    
    // Verify page B is now resident
    if (mincore(page_B, PAGE_SIZE, vec) == 0) {
        printf("\t%d: Page B residency after touch: %s\n", mypid,
               (vec[0] & 1) ? "RESIDENT" : "NOT RESIDENT");
    }
    
    // Calculate new stack pointer: 2 giant words (16 bytes on x86_64) above border
    // The border is at the start of page B
    // ERROR CODE
    // RIP
    // CS
    // RFLAGS
    // [RSP]
    // [SS]
    void *new_stack_ptr;
    if (operation == 2) {
        printf("\t%d: Operation 2 selected - will cause general protection fault by writing to reserved CR4 bit, set up stack to allow the exception frame\n", mypid);
        new_stack_ptr = (char *)page_B + 0x40; //8 qwords for the exception frame and a call
    }
    else {
        new_stack_ptr = (char *)page_B + 16;
    }

    void *unmapped_page = (char *)mapped_area + map_size + PAGE_SIZE;
    
    printf("\t\tNew stack pointer: %p (16 bytes into page B)\n", new_stack_ptr);
    printf("\t\tTarget out of core page: %p (beyond mapped region)\n", page_C);
    
    // Set up the longjmp target
    if (sigsetjmp(fault_jmp, 1) == 0) {
        printf("\t%d: Executing stack pointer manipulation and out of core write...\n", mypid);
        
        // Save current stack pointer and manipulate it
        // This is extremely dangerous - we're putting the stack in page B (unmapped)
        // Any function call or stack operation will fault
        register void *old_rsp __asm__("rsp");
        register void *saved_rsp = old_rsp;
        
        // Set stack pointer to the calculated location
        __asm__ volatile (
            "mov %0, %%rsp\n\t"
            :
            : "r" (new_stack_ptr)
            : "memory"
        );
        
        // printf("Stack pointer set to %p\n", new_stack_ptr);
        
        // Now attempt to write to an unmapped page
        // This should trigger a page fault
        // printf("\t%d: Attempting write to out of core page at %p...\n", mypid, page_C);
        if (operation == 0) {
            // Write to page C which is not present
            *(volatile char *)page_C = 0x99;
        } else {
            // cause gen prot fault by writing to reserved cr4 bit
            __asm__ volatile (
                "mov %%cr4, %%rax\n\t"
                "or $0x8000, %%rax\n\t" // Set reserved bit 15
                "mov %%rax, %%cr4\n\t"
                :
                :
                : "rax", "memory"
            );
        }

        printf("\t%d: Write completed\n", mypid);
        
        // Restore stack pointer if we survived
        __asm__ volatile (
            "mov %0, %%rsp\n\t"
            :
            : "r" (saved_rsp)
            : "memory"
        );

        pf_cnt = pf_adaptor_pf_cnt_get();
        df_cnt = pf_adaptor_df_cnt_get();

        printf("\t%d: current df counter %lu, pf counter: %lu\n", mypid, df_cnt, pf_cnt);
        
    } else {
        printf("\t%d: Caught fault during test\n", mypid);
        printf("\t%d: Fault was at iteration %d\n", mypid, fault_caught);
        ret = 0; // This is expected behavior
    }
    
    printf("\t%d: MMAP STACK TEST: END\n", mypid);
    
cleanup:
    if (vec) free(vec);
    if (mapped_area && mapped_area != MAP_FAILED) {
        munmap(mapped_area, map_size);
    }
    if (fd >= 0) {
        close(fd);
    }
    
    // Restore default signal handler
    signal(SIGSEGV, SIG_DFL);
    
    return ret;
}
