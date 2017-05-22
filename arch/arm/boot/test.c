#include <stdio.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#define __NR_insere 374
#define __NR_remove 375
int insere(int i);
int poporemove();
void poposleep(long long int i);
int main(void) {
  // printf("Foo");
  // long int x = syscall(__NR_insere);
  printf("Hello world!\n");
  // printf("What");
  // printf("%ld", x);
  long long int throttle = 1000;
  int insert_process = fork();
  if (insert_process == 0) {
    int i = 0;
    while(1){
      long int x = 1;// insere(i);
      i++;
      if (x == -1) {
        printf("ERROR: Its over 100!\n");
      } else {
        printf("Inserted!\n");
      }
      poposleep(throttle);
    }
  } else {
    while(1) {
      int nerror = poporemove();
      if (nerror == -1) {
        printf("ERROR: List is empty!\n");
      } else {
        printf("Removed!\n");
      }
      poposleep(throttle*100);
    }
  }
  while(1){}
  return 0;
}

int insere(int i) {
  return syscall(__NR_insere, i);
}

int poporemove() {
  return syscall(__NR_remove);
}

void poposleep(long long int i) {
  long long int x = 0;
  while (x < i) {
    x++;
    printf("%10lld\b\b\b\b\b\b\b\b\b\b", x);
  }
}
