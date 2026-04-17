To represent all standard symbol types using the **Absolute Symbol (`SHN_ABS`)** method, we map the entries to the corresponding ELF Bindings (`STB`) and Types (`STT`).

In the ELF symbol table:
*   **Functions** use `STT_FUNC`.
*   **Data, RODATA, and BSS** all use `STT_OBJECT`.
*   **Absolute Constants** often use `STT_NOTYPE`.
*   **Weak** symbols use `STB_WEAK`.

Here is the updated `main` function with a comprehensive `entries[]` list and a matching `test.c` to demonstrate how they appear to the system.

### Updated `main` function for `kallsyms.c`

```c
int main() {
    SymbolEntry entries[] = {
        // 1. Global Function (Standard 'T' type, but 'A' because it's Absolute)
        {"global_func",   0x7FFF00001000, 0,  BIND_GLOBAL, TYPE_FUNC},

        // 2. Weak Function (Appears as 'W' in nm)
        // If the app defines its own 'weak_func', the app's version wins.
        {"weak_func",     0x7FFF00002000, 0,  BIND_WEAK,   TYPE_FUNC},

        // 3. Global Initialized Data (Standard 'D' type, but 'A' because it's Absolute)
        {"global_data",   0x7FFF00003000, 8,  BIND_GLOBAL, TYPE_DATA},

        // 4. Global Read-Only Data (Standard 'R' type)
        // Note: For Absolute symbols, the 'RO' is enforced by your external memory protection
        {"global_rodata", 0x7FFF00004000, 4,  BIND_GLOBAL, TYPE_RODATA},

        // 5. Global BSS / Uninitialized Data (Standard 'B' type)
        {"global_bss",    0x7FFF00005000, 256, BIND_GLOBAL, TYPE_BSS},

        // 6. Weak Data Symbol (Appears as 'V' or 'W' in nm)
        {"weak_data",     0x7FFF00006000, 4,  BIND_WEAK,   TYPE_DATA},

        // 7. A Pure Absolute Constant (No type, just a value)
        {"absolute_val",  0xDEADBEEF,     0,  BIND_GLOBAL, TYPE_ABS}
    };

    create_library("libstubs.so", entries, 7);
    return 0;
}
```

### Updated `test.c` to verify all types

To avoid the `TEXTREL` warning we saw earlier, remember to compile this with **`-fPIC`**.

```c
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
```

### How to Build and Check

1.  **Generate the library:**
    ```bash
    gcc kallsyms.c -o kallsyms -lelf
    ./kallsyms
    ```

2.  **Verify with `nm` (Static Table):**
    ```bash
    nm libstubs.so
    ```
    You will see:
    *   `A` for Absolute Global symbols.
    *   `W` for Weak Functions.
    *   `V` or `W` for Weak Data.

3.  **Verify with `readelf` (Detailed Types):**
    ```bash
    readelf -s libstubs.so
    ```
    This is the best way to see that the types are preserved:
    *   `global_func` will show `FUNC` and `ABS`.
    *   `global_data` will show `OBJECT` and `ABS`.
    *   `absolute_val` will show `NOTYPE` and `ABS`.

4.  **Link and Run the Test:**
    ```bash
    gcc -fPIC test.c -L. -lstubs -o test
    LD_LIBRARY_PATH=. ./test
    ```

### Summary of Types
| Type | ELF Binding | ELF Type | nm Flag (ABS) | Description |
| :--- | :--- | :--- | :--- | :--- |
| **FUNC** | `STB_GLOBAL` | `STT_FUNC` | `A` | Standard Global Function |
| **WEAK FUNC**| `STB_WEAK` | `STT_FUNC` | `W` | Overridable Function |
| **DATA** | `STB_GLOBAL` | `STT_OBJECT` | `A` | Initialized Variable |
| **RODATA** | `STB_GLOBAL` | `STT_OBJECT` | `A` | Const/Read-only Variable |
| **BSS** | `STB_GLOBAL` | `STT_OBJECT` | `A` | Uninitialized Variable |
| **WEAK DATA**| `STB_WEAK` | `STT_OBJECT` | `V` | Overridable Variable |
| **ABS** | `STB_GLOBAL` | `STT_NOTYPE` | `A` | Pure numeric constant |