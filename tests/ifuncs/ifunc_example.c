#include <stdio.h>
#include <string.h>
#include <cpuid.h>

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
    
    printf("Demonstrating ifunc usage:\n");
    
    // The first call will trigger the resolver
    my_strcpy(buffer, source);
    printf("Copied string: %s\n", buffer);
    
    // Subsequent calls will use the already resolved function
    my_strcpy(buffer, "Second call");
    printf("Copied string: %s\n", buffer);
    
    return 0;
}