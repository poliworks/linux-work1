#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/time.h>

#define __NR_insere 374
#define __NR_remove 375

int syscall_insere(int i);
int syscall_remove();
void p_sleep(long long int i);
void insere_process();
void remove_process();
void msleep(int ms);

int main(void) {
  printf("Hello world!\n");
  int insert_process = fork();
  if (insert_process == 0) {
    insere_process();
  } else {
    remove_process();
  }
  while(1) { }
  return 0;
}

int syscall_insere(int i) {
  return syscall(__NR_insere, i);
}

int syscall_remove() {
  return syscall(__NR_remove);
}

void insere_process() {
  int i = 1;
  while(1) {
    int x = syscall_insere(i);
    i++;
    if (x == -1) {
      printf("ERROR: Its over 100!\n");
    } else {
      printf("Inserted %d!\n", i);
    }
    msleep(500);
  }
  return;
}

void remove_process() {
  while(1) {
    int nerror = syscall_remove();
    if (nerror == -1) {
      printf("ERROR: List is empty!\n");
    } else {
      printf("Removed %d!\n", nerror);
    }
    msleep(1000);
  }
  return;
}

void msleep(int ms) {
  usleep(ms * 1000);
  return;
}
