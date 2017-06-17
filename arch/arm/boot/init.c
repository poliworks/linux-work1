#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/time.h>

#define __NR_insere 374
#define __NR_remove 375
#define __NR_cfs 376

int syscall_insere(int i);
int syscall_remove();
int syscall_cfs();
void p_sleep(long long int i);
void insere_process();
void remove_process();
void msleep(int ms);

int main(void) {
  printf("Init!\n");
  int x = 0;
  int a = fork();
  printf("\na = %d\n", a);
  if (a == 0) {
    while(1) {
//        printf("Init\n");
        msleep(1000);
        syscall_cfs();
    }
  } else {
//    while(1) {}
    int b = fork();

  printf("b = %d\n", b);
    if (b == 0) {
        int filho = 0;
        while(1) {

//           msleep(1000);
           filho++;
//            printf("Filho!\n");
        }
    } else {
        int neto = 0;
        while(1) {
            msleep(1000);
            neto++;
//            printf("Neto!\n");
        }
    }
//      printf("Filho Fork");
//      while(1) {
//        x++;
//      }
//    } else {
//      printf("Neto");
//      while(1) {
//        x--;
//      }
//    }

  }
  return 0;
}

int syscall_insere(int i) {
  return syscall(__NR_insere, i);
}
int syscall_cfs() {
  return syscall(__NR_cfs);
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
    msleep(50);
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
    msleep(100);
  }
  return;
}

void msleep(int ms) {
  usleep(ms * 1000);
  return;
}
