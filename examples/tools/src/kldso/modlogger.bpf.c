/*#include <linux/module.h>
#include <uapi/linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/module/module_load")
int handle_load(struct trace_event_raw_module_load *ctx) {
    char name[64];
    bpf_probe_read_kernel_str(&name, sizeof(name), ctx->name);
    bpf_printk("MODULE_LOAD: %s\n", name);
    return 0;
}

SEC("tracepoint/module/module_free")
int handle_unload(struct trace_event_raw_module_free *ctx) {
    char name[64];
    bpf_probe_read_kernel_str(&name, sizeof(name), ctx->name);
    bpf_printk("MODULE_UNLOAD: %s\n", name);
    return 0;
}

char _license[] SEC("license") = "GPL"; 
*/

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 
 * Tracepoint arguments can be accessed via a generic struct.
 * For module_load and module_free, the name is usually at a 
 * specific offset, but bpf_printk can often access ctx fields 
 * if we define a minimal structure.
 */

struct trace_module_args {
    unsigned short common_type;
    unsigned char common_flags;
    unsigned char common_preempt_count;
    int common_pid;
    const char *name; // The module name we want
};

SEC("tracepoint/module/module_load")
int handle_load(struct trace_module_args *ctx) {
    // Note: We use bpf_probe_read_kernel_str if the verifier 
    // complains about direct access, but bpf_printk is usually 
    // flexible with tracepoint contexts.
    bpf_printk("MOD_LOAD: %s\n", ctx->name);
    return 0;
}

SEC("tracepoint/module/module_free")
int handle_unload(struct trace_module_args *ctx) {
    bpf_printk("MOD_UNLOAD: %s\n", ctx->name);
    return 0;
}

char _license[] SEC("license") = "GPL";
