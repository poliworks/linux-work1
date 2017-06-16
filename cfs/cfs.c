#include <linux/linkage.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/kernel.h>

asmlinkage long sys_cfs() {
  printk("We did it again!");
}

int __init init_module(void) {
  printk(KERN_INFO "Initialize CFS");
  return 0;
}
void __exit cleanup_module(void) {
  printk(KERN_INFO "Cleanup CFS");
}
