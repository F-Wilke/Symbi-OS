#include <linux/module.h>
#include <linux/init.h>

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/ptrace.h>
#include <linux/uaccess.h>
#include <linux/signal.h>
#include <linux/pid.h>
#include <linux/slab.h>
#include <linux/random.h>

//taken from https://elixir.bootlin.com/linux/v6.16/source/tools/testing/selftests/kvm/include/x86/processor.h#L1167
struct idt_entry {
	uint16_t offset0;
	uint16_t selector;
	uint16_t ist : 3;
	uint16_t : 5;
	uint16_t type : 4;
	uint16_t : 1;
	uint16_t dpl : 2;
	uint16_t p : 1;
	uint16_t offset1;
	uint32_t offset2; uint32_t reserved;
};



//  Define the module metadata.
#define MODULE_NAME "greeter"
MODULE_AUTHOR("Dave Kerr");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple kernel module to greet a user");
MODULE_VERSION("0.1");


void print_idt_entries(void) {
    char idtr[10];
    struct idt_entry *idt;
    
    asm volatile ("sidt %0" : "=m" (idtr));

    idt = (struct idt_entry *)(*(uint64_t *)&idtr[2]); //base address is loaded to 2 byte offset
    printk("IDT base address: %p\n", idt);
    
    for (unsigned int i = 0; i < 256; i++) {
        uint64_t offset = ((uint64_t)idt[i].offset2 << 32) |
                          ((uint64_t)idt[i].offset1 << 16) |
                          (uint64_t)idt[i].offset0;
        uint16_t selector = idt[i].selector;
        uint8_t ist = idt[i].ist;
        uint8_t type_attr = (idt[i].type & 0x0F) | ((idt[i].dpl & 0x03) << 5) | ((idt[i].p & 0x01) << 7);
        
        printk("IDT Entry %3u: Offset=0x%016llx Selector=0x%04x IST=%u TypeAttr=0x%02x\n",
                i, offset, selector, ist, type_attr);
    }
}


//  Define the name parameter.
static char *name = "Bilbo";
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");


static int __init greeter_init(void)
{
    pr_info("%s: module loaded at 0x%p\n", MODULE_NAME, greeter_init);
    pr_info("%s: greetings %s\n", MODULE_NAME, name);

    return 0;
}

static void __exit greeter_exit(void)
{
    pr_info("%s: goodbye %s\n", MODULE_NAME, name);
    pr_info("%s: module unloaded from 0x%p\n", MODULE_NAME, greeter_exit);
}

module_init(greeter_init);
module_exit(greeter_exit);
