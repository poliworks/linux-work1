#include <linux/linkage.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>

#include <linux/sched.h>
#include  <linux/slab.h>

// #include <../kernel/sched.c>

struct cfsunit {
    u64 vruntime;
    pid_t pid;
    struct rb_node *node;
    struct cfsunit *next, *prev;
};
struct cfsunit* order_rb_tree(struct rb_node *root);

struct cfsunit* order_rb_tree(struct rb_node *root) {
//    printk("order rb_node %p\n", root);
    struct cfsunit *right, *left, *aux;
    struct cfsunit *result;
    result = kmalloc(sizeof(struct cfsunit), GFP_KERNEL);

    result->node = root;
    if(root->rb_left != NULL && root->rb_right != NULL) {
        left = order_rb_tree(root->rb_left); /*ordered left size*/
        right = order_rb_tree(root->rb_right); /*ordered right size*/

         /*get last on left size list*/
        aux = left;
        while(aux->next != NULL) {
            aux = aux->next;
        }

        /*puts result on the middle*/
        aux->next = result;
        result->next = right;
        result->prev = aux;
        right->prev = result;

        return left; /* left -> result -> right*/
    } else if (root->rb_left == NULL && root->rb_right != NULL) {
        right = order_rb_tree(root->rb_right);

        /* puts result as first on list */
        result->prev = NULL;
        result->next = right;
        right->prev = result;

        return result;
    } else if (root->rb_left != NULL && root->rb_right == NULL) {
        left = order_rb_tree(root->rb_left);

        /* puts result as last on list*/
         aux = left;
        while(aux->next != NULL) {
            aux = aux->next;
        }
        aux->next = result;
        result->prev = aux;
        result->next = NULL;

        return left;
    } else if (root->rb_left == NULL && root->rb_right == NULL) {
        result->next = NULL;
        result->prev = NULL;
        return result;
    }
    return result; // should never return here
}

asmlinkage long sys_cfs() {
//    struct rq *rq = this_rq();
//    printk("rq = %p", rq);
   //      a
   //    /   \
   //   b     c
   //  / \     \
   // d   e     f
//  struct rb_node a, b, c, d, e, f;
//  a.rb_right = &c;
//  a.rb_left = &b;
//  b.rb_right = &e;
//  b.rb_left = &d;
//  c.rb_right = &f;
//  c.rb_left = NULL;
//  d.rb_right = NULL;
//  d.rb_left = NULL;
//  e.rb_right = NULL;
//  e.rb_left = NULL;
//  f.rb_right = NULL;
//  f.rb_left = NULL;
//  // output should be [ d b e a c f ]
//  printk("a %p\n", &a);
//  printk("b %p\n", &b);
//  printk("c %p\n", &c);
//  printk("d %p\n", &d);
//  printk("e %p\n", &e);
//  printk("f %p\n", &f);
  struct cfsunit *cfsu, *aux;
//  printk("d %p\n", &d);
//  printk("b %p\n", &b);
//  printk("e %p\n", &e);
//  printk("a %p\n", &a);
//  printk("c %p\n", &c);
//  printk("f %p\n", &f);
  struct sched_entity *se_right, *se_left, *sce;
  struct task_struct *ts = NULL, *testcurrent = NULL, *rootcfsunit;
  struct rb_node root = current->se.run_node;
//  printk("root: %p", root);
//  printk("Init Test\n");
//  testcurrent = current;
//  while(testcurrent != NULL) {
//    printk("%p\n", testcurrent);
//    struct rb_root *testroot = &(testcurrent->pi_waiters);
//    struct rb_node *n = &(testcurrent->se.run_node);
//    printk("testroot pi_waiters: %p %p %p\n", n, n->rb_left,n->rb_right);
//    if(testcurrent == testcurrent->real_parent) {
//        rootcfsunit = testcurrent;
//        testcurrent = NULL;
//    } else {
//        testcurrent = testcurrent->real_parent;
//    }
//  }
  cfsu = order_rb_tree(&root);
  aux = cfsu;
  printk("\n         pid |   vruntime\n");
  while(aux != NULL) {
//    printk("%p\n", aux->node);
    sce = container_of(aux->node, struct sched_entity, run_node);
    ts = container_of(sce, struct task_struct, se);
    printk(" %11ld | %lld\n", (long)ts->pid, sce->vruntime);
    aux = aux->next;
  }
  return 0;
}

int __init init_module_cfs(void) {
  printk(KERN_INFO "Initialize CFS");
  return 0;
}
void __exit cleanup_module_cfs(void) {
  printk(KERN_INFO "Cleanup CFS");
}
