#include <sys/syscall.h>

// some code from gemini

/* --- 1. Minimal Syscall Wrappers --- */
static inline long sys_write(int fd, const char *buf, unsigned long len) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(SYS_write), "D"(fd), "S"(buf), "d"(len) : "rcx", "r11", "memory");
    return ret;
}

static inline void sys_exit(int code) {
    asm volatile ("syscall" : : "a"(SYS_exit), "D"(code) : "rcx", "r11");
}

static inline long sys_execve(const char *filename, char *const argv[], char *const envp[]) {
    long ret;
    asm volatile ("syscall" : "=a"(ret) : "a"(SYS_execve), "D"(filename), "S"(argv), "d"(envp) : "rcx", "r11", "memory");
    return ret;
}

/* --- 2. Minimal String Helpers (Since we have no string.h) --- */
int my_strlen(const char *str) {
    const char *s = str;
    while (*s) s++;
    return s - str;
}

// Checks if 'str' starts with 'prefix'. Returns 1 (true) or 0 (false).
int starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) return 0;
    }
    return 1;
}

/* --- 3. Custom getenv Implementation --- */
char *get_env_value(char **envp, const char *key) {
    for (char **e = envp; *e; e++) {
        char *current = *e;
        
        // Check if the current string starts with the key
        if (starts_with(current, key)) {
            // Check if the next character is '=', ensuring exact match
            // e.g., ensure "HOME" doesn't match "HOMEPATH"
            int key_len = my_strlen(key);
            if (current[key_len] == '=') {
                // Return the address of the value (character after '=')
                return &current[key_len + 1];
            }
        }
    }
    return (char *)0; // NULL
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/* --- 4. Entry Point --- */
void c_entry(int argc, char **argv) {
    char **envp = &argv[argc + 1];
#ifndef KLDD_SH_PATH
    #error KLDD_SH_PATH UNDEFINED
#endif
    char *script_path = TOSTRING(KLDD_SH_PATH);
    //    char *script_path = "/lib64/ld-linux-x86-64.so.2";
    char *newargv[argc+2]; // +1 new argv[0] and +1 for null entry
    int debug=0;
    
    if (get_env_value(envp, "KLD_DEBUG")) debug=1;

    if (debug) {
      sys_write(2, "[KLD.SO]:", 9);
      for (int i=0; i<argc; i++) {
	sys_write(2, " ", 1);
	sys_write(2, argv[i], my_strlen(argv[i]));
      }
      sys_write(2, "\n", 1);
    }
    
    // We insert the pointer to our own binary name with the pointer 
    // to our script's path. We do NOT change argv[1] or higher.
    newargv[0] = script_path;
    // copy orig
    for (int i=0; i<argc+1; i++) { newargv[i+1] = argv[i]; }
    
    // Now when we execve, argv[0] is the script name ($0), 
    // and argv[1] remains the program name ($1).
    sys_execve(script_path, (char *const *)newargv, (char *const *)envp);

    // If execve returns, it failed
    sys_exit(1);
}
