#include <linux/module.h>
#include "app.h"

void set_app_got(app_got_t* got);

//  Define GOT
app_got_t* app_got;

//  Define the module metadata.
#define MODULE_NAME "kcutpf"
MODULE_AUTHOR("BU SESA");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BU SESA DYNPRIV PROCESS EXTENSION");
MODULE_VERSION("0.1b");

//  Define the name parameter.
static char *name = "dynpriv extension";
module_param(name, charp, S_IRUGO);
MODULE_PARM_DESC(name, "The name to display in /var/log/kern.log");

extern int extension_init(void) __attribute__((weak));
static int __init ext_init(void)
{
  pr_info("%s: module loaded at 0x%p\n", MODULE_NAME, extension_init);
  pr_info("%s: args: name=%s\n", MODULE_NAME, name);
  if (&extension_init != NULL) return extension_init();
  return 0;
}

extern void extension_exit(void) __attribute__((weak));
static void __exit ext_exit(void)
{
    pr_info("%s: goodbye %s\n", MODULE_NAME, name);
    pr_info("%s: module unloaded from 0x%p\n", MODULE_NAME, extension_exit);
    if (&extension_exit != NULL) extension_exit();	    
}


void set_app_got(app_got_t* got) {
    app_got = got;
    // pr_info("%s: app_got set to 0x%p\n", MODULE_NAME, app_got);
}

module_init(ext_init);
module_exit(ext_exit);
