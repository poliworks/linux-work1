8.9 9 Rode o seguinte programa como init:
```c
#include <stdio.h>

void main() {
  int c;
  printf("Hello World!\n");
  while(1) {
    c = getchar();
    printf ("teclou %d ", c);
  };
}
```
A função getchar chamará o sys_read no kernel que esperará o usuário teclar algo seguido de ENTER. sys_read chamará indiretamente a função de leitura do driver da tty. Onde isso ocorre?

Se o usuário não tiver teclado nada, o driver chamará scheduler pedindo para se escalonar algum outro processo. Onde isso ocorre?

Quando o usuário teclar algo, a função __wakeup() em kernel/sched.c serah chamada, pois o kernel avisará o processo de usuário (no caso init) de que algo foi teclado. Coloque um breakpoint em __wakeup e veja o caminho (usando bt no gdb) entre a interrupção e o wakeup. Fazendo isso, talvez voce obtenha:

```
(gdb) bt
#0 __wakeup (q=0xc02128fc, mode=1, nr_exclusive=1, key=0x0)
  at kernel/sched.c:4338
#1 0xc01302e8 in credit_entropybits (r=<value optimized out>, nbits=<value optimized out>)
  at drivers/char/random.c:556
#2 0xc0130b80 in add_timerrandomness (state=0xc0231e08, num=<value optimized out>)
  at drivers/char/random.c:676
#3 0xc014c1a8 in input_event (dev=0xc71c3c00, type=4, code=3, value=28)
  at drivers/input/input.c:355
#4 0xc0151278 in atkbd_interrupt (serio=0xc71abe00, data=<value optimized out>, flags=0)
  at drivers/input/keyboard/atkbd.c:401
#5 0xc0149c88 in serio_interrupt (serio=<value optimized out>, data=<value optimized out>, dfl=<value optimized out>)
  at drivers/input/serio/serio.c:996
#6 0xc0149de8 in amba_kmiint (irq=<value optimized out>, devid=0xc71a13e0)
  at drivers/input/serio/ambakmi.c:47
#7 0xc0061e30 in handle_irqeventpercpu (desc=0xc02097c8, action=0xc7169c60)
  at kernel/irq/handle.c:126
#8 0xc0061fb8 in handle_irqevent (desc=0xc02097c8)
  at kernel/irq/handle.c:182
#9 0xc00639b4 in handle_levelirq (irq=<value optimized out>, desc=0xc02097c8)
  at kernel/irq/chip.c:344
#10 0xc003582c in generic_handleirqdesc (irq=<value optimized out>, desc=<value optimized out>)
  at include/linux/irqdesc.h:109
#11 generichandleirq (irq=<value optimized out>, desc=<value optimized out>)
  at include/linux/irqdesc.h:114
#12 fpga_irqhandle (irq=<value optimized out>, desc=<value optimized out>)
  at arch/arm/plat-versatile/fpga-irq.c:45
#13 0xc0022070 in generic_handleirqdesc (irq=31, regs=<value optimized out>)
  at include/linux/irqdesc.h:109
#14 generic_handleirq (irq=31, regs=<value optimized out>)
  at include/linux/irqdesc.h:114
#15 asm_doIRQ (irq=31, regs=<value optimized out>)
  at arch/arm/kernel/irq.c:90
#16 0xc002bb34 in __irqsvc ()
  at arch/arm/kernel/entry-armv.S:42
```

Caso não tenha obtido o trace acima, basta colocar breakpoints nas rotinas intermediárias. Toda rotina de interrupcão associada a um driver deve ser registrada. Temos uma sequencia de drivers usando outros drivers, mas a primeira interrupcao do primeiro driver foi ambakmiint (subindo da interrupcao de baixo para cima). Fazendo uma busca em sua declaracao:
```
~/Downloads/linux-2.6.39/drivers$ grep -r amba_kmi_int *
input/serio/ambakmi.c:static irqreturnt amba_kmi_int(int irq, void *dev_id) input/serio/ambakmi.c: ret = request_irq(kmi->irq, amba_kmi_int, 0, "kmi-pl050", kmi);
```

O registro dessa interrupcao, eh feito por amba_kmi_open. Para ver sobre amba kmi: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0447j/Cfhhieif.html

Quando ocorre a interrupcao de hardware, eh necessário saber quem gerou a interrupcao: teclado, timer, mouse, etc. Localize isso no código.
Uma vez localizado o hardware, eh necessário passar o tratamento da interrupçao para o driver adequado. Ver amba_kmiint. Localize isso no código.
O caracter (ou caracteres) teclado é lido de algum port. Na verdade, se lê a posição da tecla. Localize isso no código.
O que faz readb(KMIDATA)?
O código lido deve ser guardado em um buffer enquanto o usuario nao tecla ENTER. Localize isso no código.
A função __wakeup acorda o processo que lia da tty (dentro do kernel). Localize isso no código. Explique melhor o que faz __wakeup. Localize isso no código.
Quando o usuário tecla ENTER, o processamento é diferente: os dados devem ser transferidos para o buffer de usuário. Localize isso no código.

## Traduzindo do Kinoshita-Language pra Liguagem Humana:
### Exercício 1
1. Localize onde a _system call_ `sys_read` chama, indiretamente, a função de leitura do driver da tty.
2. Localize onde o driver chama o _scheduler_ pedindo para escalonar algum outro processo enquanto o usuário não digita nada.
3. Coloque um breakpoint no ___wakeup_ do _scheduler_ para ver quando o processo volta a ser escalonado
### Exercício 2
1. Localize onde o kernel identifica quem gerou a interrupção de hardware (teclado, timer, mouse, etc.).
2. Localize onde o kernel passa o tratamento da interrupção para o driver adequado. Veja amba_kmiint.
3. Quando usamos a _system call getchar()_, o caractér do teclado é lido de algum port. Na verdade, se lê a posição da tecla. Localize isso no código.
4. O que faz readb(KMIDATA)?
5. O código lido deve ser guardado em um buffer enquanto o usuário não tecla ENTER. Localize isso no código.
6. Localize onde a funćão __wakeup() acorda o processo que lia da tty (dentro do kernel).
7. Quando o usuário tecla ENTER, o processamento é diferente: os dados devem ser transferidos para o buffer de usuário. Localize isso no código.
