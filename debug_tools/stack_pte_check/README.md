# Stack PTE Corruption Checker

A Linux kernel module that uses kprobes to check for corrupted Page Table Entries (PTEs) in process stack regions.

## Overview

This debug tool hooks into safe syscall handlers and inspects PTEs in the current process's stack for a configurable binary pattern. It's designed to help detect memory corruption issues in modified Linux kernels.

## Features

- **Kprobe-based monitoring**: Hooks into `getpid`, `read`, and `write` syscalls
- **Safe PTE walking**: Uses `walk_page_range()` with proper locking
- **Configurable pattern matching**: Customizable mask and pattern via module parameters
- **Rate limiting**: Optional rate-limited warnings to prevent log spam
- **Statistics tracking**: Counts checks performed and pattern matches found

## Building

```bash
make
```

This will build `stack_pte_probe.ko` against the currently running kernel.

To build for a different kernel:
```bash
make KDIR=/path/to/kernel/build
```

## Usage

### Basic Usage

```bash
# Load the module with default settings
sudo insmod stack_pte_probe.ko

# View kernel log for warnings
dmesg -w

# Unload the module
sudo rmmod stack_pte_probe
```

### With Custom Parameters

```bash
# Load with custom mask and pattern
sudo insmod stack_pte_probe.ko pte_mask=0xFFFF pte_pattern=0xDEAD

# Load without rate limiting (show all matches)
sudo insmod stack_pte_probe.ko rate_limit=0

# Load with different pattern
sudo insmod stack_pte_probe.ko pte_mask=0xFF00 pte_pattern=0xBE00
```

### Module Parameters

- **pte_mask** (default: `0xFFFF`)
  - Mask applied to each PTE value before comparison
  - Allows checking specific bits while ignoring others
  
- **pte_pattern** (default: `0xDEAD`)
  - Pattern to match against masked PTE values
  - Warning is printed if `(pte_val & pte_mask) == pte_pattern`

- **rate_limit** (default: `1`)
  - Enable rate limiting of warning messages
  - Set to `0` to disable rate limiting (not recommended for production)

### Reading Statistics

When the module is unloaded, it prints statistics:

```bash
sudo rmmod stack_pte_probe
dmesg | tail
```

Output example:
```
[stack_pte_probe] Statistics: checks=1234 matches=5
[stack_pte_probe] Module unloaded
```

## How It Works

1. **Kprobe Registration**: The module registers kprobes on safe syscall handlers:
   - `__x64_sys_getpid`
   - `__x64_sys_read`
   - `__x64_sys_write`
   - `symbi_elevate` (Symbi-OS specific)
   - `symbi_lower` (Symbi-OS specific)

2. **Safety Checks**: Before inspecting PTEs, the probe handler verifies:
   - Current task has valid `mm_struct` (not a kernel thread)
   - Not in atomic context
   - Interrupts are enabled
   - Not a kernel thread (double-check)

3. **Stack VMA Discovery**: Locates the stack VMA using `find_vma()` and `mm->start_stack`

4. **Manual PTE Walking**: Since `walk_page_range()` is not exported for kernel modules, this implementation manually traverses the page table hierarchy:
   - PGD (Page Global Directory) level
   - P4D (Page 4th Directory) level
   - PUD (Page Upper Directory) level
   - PMD (Page Middle Directory) level
   - PTE (Page Table Entry) level
   
   Uses `pte_offset_kernel()` for direct kernel mapping access.

5. **Pattern Matching**: Each PTE is checked against the configured mask and pattern

6. **Warning Output**: Matching PTEs trigger a warning with:
   - Process command name and PID
   - Virtual address
   - Raw PTE value
   - Masked value and pattern

## Safety Considerations

This module follows kernel best practices for debug instrumentation:

- **Uses `mmap_read_trylock()`** instead of `mmap_read_lock()` to avoid deadlocks
- **Probes simple syscalls** that don't manipulate page tables or VMAs
- **Avoids probing in dangerous contexts**: atomic, IRQ-disabled, allocator paths
- **Rate limits warnings** to prevent flooding the kernel log
- **Manual page table walking** avoids dependency on unexported kernel symbols

### When NOT to Use

Do not use this module if:
- You're running in production (debug-only tool)
- The kernel is heavily loaded (adds overhead to every probed syscall)
- You need to probe memory-management syscalls (unsafe - choose different hooks)

## Example Output

When a matching PTE is found:

```
[stack_pte_probe] BAD STACK PTE: comm=myapp pid=1234 addr=0x7ffd12345678 pte=0x800000012dead867 (masked=0xdead pattern=0xdead)
```

## Graceful Failure Handling

The module is designed to work even if some probe symbols don't exist:

- **Partial Success**: If some symbols (like `symbi_elevate`/`symbi_lower`) don't exist, those probes will be skipped with a warning, but the module will still load with the available probes
- **Complete Failure**: The module will only fail to load if **none** of the symbols exist
- **Load Messages**: Check `dmesg` after loading to see which probes were successfully registered:

```bash
sudo insmod stack_pte_probe.ko
dmesg | grep stack_pte_probe
# Look for: "Successfully registered N/M kprobes"
```

Example output on non-Symbi-OS kernel:
```
[stack_pte_probe] Registered kprobe at __x64_sys_getpid: ffffffffa1234567
[stack_pte_probe] Registered kprobe at __x64_sys_read: ffffffffa1234890
[stack_pte_probe] Registered kprobe at __x64_sys_write: ffffffffa1234abc
[stack_pte_probe] Failed to register kprobe for symbi_elevate: -2 (symbol may not exist)
[stack_pte_probe] Failed to register kprobe for symbi_lower: -2 (symbol may not exist)
[stack_pte_probe] Successfully registered 3/5 kprobes
```

## Troubleshooting

### Module fails to load completely

This means **none** of the symbols were found. Check if they exist:

```bash
# Check if symbols exist in kallsyms
sudo grep '__x64_sys_getpid' /proc/kallsyms
sudo grep '__x64_sys_read' /proc/kallsyms
sudo grep '__x64_sys_write' /proc/kallsyms
sudo grep 'symbi_elevate' /proc/kallsyms
sudo grep 'symbi_lower' /proc/kallsyms
```

If symbols are missing, your kernel may use different naming conventions. Check `kernel.org/doc/html/latest/trace/kprobes.html` for your kernel version.

### No warnings appear

- The pattern may not exist in any stack PTEs
- Try different mask/pattern values
- Check module is loaded: `lsmod | grep stack_pte_probe`
- Verify probes are active: `dmesg | grep stack_pte_probe`

### Too many warnings

- Enable rate limiting: reload with `rate_limit=1`
- Adjust the mask to be more specific
- Choose a different pattern less likely to match

## Makefile Targets

```bash
make           # Build the module
make clean     # Clean build artifacts
make install   # Build and install module
make uninstall # Remove the module
make log       # Tail kernel log (dmesg -w)
make info      # Show module information
```

## Architecture

This module is designed for **x86-64** (uses `__x64_sys_*` symbols). For other architectures:

1. Update syscall symbol names in `stack_pte_probe.c`
2. Verify `VM_GROWSDOWN` flag is correct for your architecture
3. Test in a safe environment first

## References

- [Kernel Kprobes Documentation](https://docs.kernel.org/trace/kprobes.html)
- [Linux Page Table Walk API](https://docs.kernel.org/mm/page_tables.html)
- [Memory Management Locking](https://www.kernel.org/doc/html/latest/mm/page_table_check.html)

## License

GPL v2 (same as the Linux kernel)

## Author

Symbi-OS Debug Tools
