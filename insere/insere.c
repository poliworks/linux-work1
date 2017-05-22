#include<linux/linkage.h>
#include<linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>

int insert(int n);
int remove();
int getSize();

asmlinkage long sys_insere(int i) {
  int x = insert(i);
  return x;
}
asmlinkage long sys_remove() {
  int nerror = remove();
  return nerror;
}

struct Item {
    int value;
    struct list_head list;
};

struct Item itemList;
struct list_head hatefulList = LIST_HEAD_INIT(hatefulList);

int getSize() {
    struct Item *aItem;
    struct list_head *l;
    int size = 0;
    list_for_each(l, &hatefulList) {
        size = size + 1;
    }
    return size;
}

int insert(int n) {
    int size = getSize();
    if (size > 99) {
        return -1;
    } else {
        struct Item *newItem;
        newItem = kmalloc(sizeof(*newItem), GFP_KERNEL);
        newItem->value = n;
        list_add_tail(&(newItem->list), &hatefulList);
        return 0;
    }
}

int remove() {
    if (list_empty(&hatefulList)) {
        return -1;
    } else {
        //struct Item firstItem = list_first_entry(&hatefulList, Item, list);
        //kfree(nextItem);
        list_del(hatefulList.next);
        return 0;
    }
}

int __init init_module(void) {
  printk(KERN_INFO "Initialize InsereModule");
  return 0;
}
void __exit cleanup_module(void) {
  printk(KERN_INFO "Cleanup InsereModule");
}
