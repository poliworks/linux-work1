## Traduzindo do Kinoshita-Language pra Liguagem Humana:
### Exercício 1
#### 1. Localize onde a _system call sys_read_ chama, indiretamente, a função de leitura do driver da tty.
Dado a implementacão da _system call sys_read_ em <fs/read_write.c>:
```c
SYSCALL_DEFINE3(read, unsigned int, fd, char __user *, buf, size_t, count)
{
  struct file *file;
  ssize_t ret = -EBADF;
  int fput_needed;

  file = fget_light(fd, &fput_needed);
  if (file) {
    loff_t pos = file_pos_read(file);
    ret = vfs_read(file, buf, count, &pos); // here we call driver!
    file_pos_write(file, pos);
    fput_light(file, fput_needed);
  }

  return ret;
}
```
Assim, identificamos que o driver do tty é chamado em `vfs_read`, cuja implementação:

```c
ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
  ssize_t ret;

  if (!(file->f_mode & FMODE_READ))
    return -EBADF;
  if (!file->f_op || (!file->f_op->read && !file->f_op->aio_read))
    return -EINVAL;
  if (unlikely(!access_ok(VERIFY_WRITE, buf, count)))
    return -EFAULT;

  ret = rw_verify_area(READ, file, pos, count);
  if (ret >= 0) {
    count = ret;
    if (file->f_op->read)
      ret = file->f_op->read(file, buf, count, pos); // tty driver call!
    else
      ret = do_sync_read(file, buf, count, pos);
    if (ret > 0) {
      fsnotify_access(file);
      add_rchar(current, ret);
    }
    inc_syscr(current);
  }

  return ret;
}
```
No entanto, é possível observar que ele encapsula a função em um `struct` na chamada `file->f_op->read`. Vejamos as definições em <include/linux/fs.h>:
```c
struct file {
  ...
  const struct file_operations  *f_op;
  ...
}
```
e
```c
struct file_operations {
  ...
  ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
  ...
};
```
#### 2. Localize onde o driver chama o _scheduler_ pedindo para escalonar algum outro processo enquanto o usuário não digita nada.
Como visto na resposta anterior, o driver é chamado pela função `tty_read`, chamado pela invocação `file->f_op->read`. Vejamos essa função:
```c
static ssize_t tty_read(struct file *file, char __user *buf, size_t count,
      loff_t *ppos)
{
  int i;
  struct inode *inode = file->f_path.dentry->d_inode;
  struct tty_struct *tty = file_tty(file);
  struct tty_ldisc *ld;

  if (tty_paranoia_check(tty, inode, "tty_read"))
    return -EIO;
  if (!tty || (test_bit(TTY_IO_ERROR, &tty->flags)))
    return -EIO;

  /* We want to wait for the line discipline to sort out in this
     situation */
  ld = tty_ldisc_ref_wait(tty); // Wait is called!
  if (ld->ops->read)
    i = (ld->ops->read)(tty, file, buf, count);
  else
    i = -EIO;
  tty_ldisc_deref(ld);
  if (i > 0)
    inode->i_atime = current_fs_time(inode->i_sb);
  return i;
}
```
É possível observar que a função `tty_ldisc_ref_wait` é responsável por fazer esse processo esperar a entrada de dados. Assim, vejamos seu código:
```c
struct tty_ldisc *tty_ldisc_ref_wait(struct tty_struct *tty)
{
  struct tty_ldisc *ld;

  /* wait_event is a macro */
  wait_event(tty_ldisc_wait, (ld = tty_ldisc_try(tty)) != NULL);
  return ld;
}
```
A função não apresenta muita coisa, porém isso é porque muito está escapsulado no macro `wait_event`. Vejamos:
```c
/**
 * wait_event - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 */
#define wait_event(wq, condition)                       \
do {                                                    \
  if (condition)                                        \
    break;                                              \
  __wait_event(wq, condition);                          \
} while (0)

#define __wait_event(wq, condition)                     \
do {                                                    \
  DEFINE_WAIT(__wait);                                  \
                                                        \
  for (;;) {                                            \
    prepare_to_wait(&wq, &__wait, TASK_UNINTERRUPTIBLE);\
    if (condition)                                      \
      break;                                            \
    schedule();                                         \
  }                                                     \
  finish_wait(&wq, &__wait);                            \
} while (0)
```
#### 3. Coloque um breakpoint no `__wakeup` do _scheduler_ para ver quando o processo volta a ser escalonado
```
(gdb) bt
#0  __wake_up (q=0xc0212904, mode=1, nr_exclusive=1, key=0x0)
    at kernel/sched.c:4338
#1  0xc0130528 in credit_entropy_bits (r=<value optimized out>,
    nbits=<value optimized out>) at drivers/char/random.c:556
#2  0xc0130dc0 in add_timer_randomness (state=0xc0231e08,
    num=<value optimized out>) at drivers/char/random.c:676
#3  0xc014c3e8 in input_event (dev=0xc71c3c00, type=4, code=3, value=28)
    at drivers/input/input.c:355
#4  0xc01514b8 in atkbd_interrupt (serio=0xc71abe00,
    data=<value optimized out>, flags=0) at drivers/input/keyboard/atkbd.c:401
#5  0xc0149ec8 in serio_interrupt (serio=<value optimized out>,
    data=<value optimized out>, dfl=<value optimized out>)
    at drivers/input/serio/serio.c:996
#6  0xc014a028 in amba_kmi_int (irq=<value optimized out>, dev_id=0xc71a13e0)
    at drivers/input/serio/ambakmi.c:47
#7  0xc0061e30 in handle_irq_event_percpu (desc=0xc02097c8, action=0xc716ac60)
    at kernel/irq/handle.c:126
#8  0xc0061fb8 in handle_irq_event (desc=0xc02097c8) at kernel/irq/handle.c:182
#9  0xc00639b4 in handle_level_irq (irq=<value optimized out>, desc=0xc02097c8)
    at kernel/irq/chip.c:344
#10 0xc003582c in generic_handle_irq_desc (irq=<value optimized out>,
    desc=<value optimized out>) at include/linux/irqdesc.h:109
#11 generic_handle_irq (irq=<value optimized out>, desc=<value optimized out>)
    at include/linux/irqdesc.h:114
#12 fpga_irq_handle (irq=<value optimized out>, desc=<value optimized out>)
    at arch/arm/plat-versatile/fpga-irq.c:45
#13 0xc0022070 in generic_handle_irq_desc (irq=31, regs=<value optimized out>)
    at include/linux/irqdesc.h:109
#14 generic_handle_irq (irq=31, regs=<value optimized out>)
    at include/linux/irqdesc.h:114
#15 asm_do_IRQ (irq=31, regs=<value optimized out>) at arch/arm/kernel/irq.c:90
#16 0xc002bb34 in __irq_svc () at arch/arm/kernel/entry-armv.S:42
```
### Exercício 2
#### 1. Localize onde o kernel identifica quem gerou a interrupção de hardware (teclado, timer, mouse, etc.).
#### 2. Localize onde o kernel passa o tratamento da interrupção para o driver adequado. Veja `amba_kmi_int`.
#### 3. Quando usamos a _system call_ `getchar()`, o caractér do teclado é lido de algum port. Na verdade, se lê a posição da tecla. Localize isso no código.
#### 4. O que faz readb(KMIDATA)?
#### 5. O código lido deve ser guardado em um buffer enquanto o usuário não tecla ENTER. Localize isso no código.
#### 6. Localize onde a função __wakeup() acorda o processo que lia da tty (dentro do kernel).
#### 7. Quando o usuário tecla ENTER, o processamento é diferente: os dados devem ser transferidos para o buffer de usuário. Localize isso no código.
