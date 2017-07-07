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
Na função `early_irq_init`, o kernel faz a inicialização da estrutura `irc_desc`. Podemos ver a implementação a seguir:
```c

int __init early_irq_init(void)
{
	int i, initcnt, node = first_online_node;
	struct irq_desc *desc;

	init_irq_default_affinity();

	/* Let arch update nr_irqs and return the nr of preallocated irqs */
	initcnt = arch_probe_nr_irqs();
	printk(KERN_INFO "NR_IRQS: %d, nr_irqs: %d, preallocated irqs: %d\n",
	       NR_IRQS, nr_irqs, initcnt);

	if (WARN_ON(nr_irqs > IRQ_BITMAP_BITS))
		nr_irqs = IRQ_BITMAP_BITS;

	if (WARN_ON(initcnt > IRQ_BITMAP_BITS))
		initcnt = IRQ_BITMAP_BITS;

	if (initcnt > nr_irqs)
		nr_irqs = initcnt;

	for (i = 0; i < initcnt; i++) {
		desc = alloc_desc(i, node, 0, NULL, NULL);
		set_bit(i, allocated_irqs);
		irq_insert_desc(i, desc);
	}
	return arch_early_irq_init();
}

```
*kernel/irq/irqdesc.c*

A estrutura `irq_desc` é a base para o gerenciamento de interrupções no kernel do Linux. O array desta estrutura (que tem o mesmo nome - `irq_desc`), mantém o controle de todas as fontes de interrupção. Esta estrutura `irq_desc` está definida a seguir:
```c
/**
 * struct irq_desc - interrupt descriptor
 * @irq_common_data:	per irq and chip data passed down to chip functions
 * @kstat_irqs:		irq stats per cpu
 * @handle_irq:		highlevel irq-events handler
 * @preflow_handler:	handler called before the flow handler (currently used by sparc)
 * @action:		the irq action chain
 * @status:		status information
 * @core_internal_state__do_not_mess_with_it: core internal status information
 * @depth:		disable-depth, for nested irq_disable() calls
 * @wake_depth:		enable depth, for multiple irq_set_irq_wake() callers
 * @irq_count:		stats field to detect stalled irqs
 * @last_unhandled:	aging timer for unhandled count
 * @irqs_unhandled:	stats field for spurious unhandled interrupts
 * @threads_handled:	stats field for deferred spurious detection of threaded handlers
 * @threads_handled_last: comparator field for deferred spurious detection of theraded handlers
 * @lock:		locking for SMP
 * @affinity_hint:	hint to user space for preferred irq affinity
 * @affinity_notify:	context for notification of affinity changes
 * @pending_mask:	pending rebalanced interrupts
 * @threads_oneshot:	bitfield to handle shared oneshot threads
 * @threads_active:	number of irqaction threads currently running
 * @wait_for_threads:	wait queue for sync_irq to wait for threaded handlers
 * @nr_actions:		number of installed actions on this descriptor
 * @no_suspend_depth:	number of irqactions on a irq descriptor with
 *			IRQF_NO_SUSPEND set
 * @force_resume_depth:	number of irqactions on a irq descriptor with
 *			IRQF_FORCE_RESUME set
 * @rcu:		rcu head for delayed free
 * @kobj:		kobject used to represent this struct in sysfs
 * @dir:		/proc/irq/ procfs entry
 * @debugfs_file:	dentry for the debugfs file
 * @name:		flow handler name for /proc/interrupts output
 */
struct irq_desc {
	struct irq_common_data	irq_common_data;
	struct irq_data		irq_data;
	unsigned int __percpu	*kstat_irqs;
	irq_flow_handler_t	handle_irq;
#ifdef CONFIG_IRQ_PREFLOW_FASTEOI
	irq_preflow_handler_t	preflow_handler;
#endif
	struct irqaction	*action;	/* IRQ action list */
	unsigned int		status_use_accessors;
	unsigned int		core_internal_state__do_not_mess_with_it;
	unsigned int		depth;		/* nested irq disables */
	unsigned int		wake_depth;	/* nested wake enables */
	unsigned int		irq_count;	/* For detecting broken IRQs */
	unsigned long		last_unhandled;	/* Aging timer for unhandled count */
	unsigned int		irqs_unhandled;
	atomic_t		threads_handled;
	int			threads_handled_last;
	raw_spinlock_t		lock;
	struct cpumask		*percpu_enabled;
	const struct cpumask	*percpu_affinity;
#ifdef CONFIG_SMP
	const struct cpumask	*affinity_hint;
	struct irq_affinity_notify *affinity_notify;
#ifdef CONFIG_GENERIC_PENDING_IRQ
	cpumask_var_t		pending_mask;
#endif
#endif
	unsigned long		threads_oneshot;
	atomic_t		threads_active;
	wait_queue_head_t       wait_for_threads;
#ifdef CONFIG_PM_SLEEP
	unsigned int		nr_actions;
	unsigned int		no_suspend_depth;
	unsigned int		cond_suspend_depth;
	unsigned int		force_resume_depth;
#endif
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry	*dir;
#endif
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
	struct dentry		*debugfs_file;
#endif
#ifdef CONFIG_SPARSE_IRQ
	struct rcu_head		rcu;
	struct kobject		kobj;
#endif
	int			parent_irq;
	struct module		*owner;
	const char		*name;
} ____cacheline_internodealigned_in_smp;

#ifdef CONFIG_SPARSE_IRQ
extern void irq_lock_sparse(void);
extern void irq_unlock_sparse(void);
#else
static inline void irq_lock_sparse(void) { }
static inline void irq_unlock_sparse(void) { }
extern struct irq_desc irq_desc[NR_IRQS];
#endif

static inline struct irq_desc *irq_data_to_desc(struct irq_data *data)
{
	return container_of(data->common, struct irq_desc, irq_common_data);
}

static inline unsigned int irq_desc_get_irq(struct irq_desc *desc)
{
	return desc->irq_data.irq;
}

static inline struct irq_data *irq_desc_get_irq_data(struct irq_desc *desc)
{
	return &desc->irq_data;
}

static inline struct irq_chip *irq_desc_get_chip(struct irq_desc *desc)
{
	return desc->irq_data.chip;
}

static inline void *irq_desc_get_chip_data(struct irq_desc *desc)
{
	return desc->irq_data.chip_data;
}

static inline void *irq_desc_get_handler_data(struct irq_desc *desc)
{
	return desc->irq_common_data.handler_data;
}

static inline struct msi_desc *irq_desc_get_msi_desc(struct irq_desc *desc)
{
	return desc->irq_common_data.msi_desc;
}

/*
 * Architectures call this to let the generic IRQ layer
 * handle an interrupt.
 */
static inline void generic_handle_irq_desc(struct irq_desc *desc)
{
	desc->handle_irq(desc);
}

int generic_handle_irq(unsigned int irq);

#ifdef CONFIG_HANDLE_DOMAIN_IRQ
/*
 * Convert a HW interrupt number to a logical one using a IRQ domain,
 * and handle the result interrupt number. Return -EINVAL if
 * conversion failed. Providing a NULL domain indicates that the
 * conversion has already been done.
 */
int __handle_domain_irq(struct irq_domain *domain, unsigned int hwirq,
			bool lookup, struct pt_regs *regs);

static inline int handle_domain_irq(struct irq_domain *domain,
				    unsigned int hwirq, struct pt_regs *regs)
{
	return __handle_domain_irq(domain, hwirq, true, regs);
}
#endif

/* Test to see if a driver has successfully requested an irq */
static inline int irq_desc_has_action(struct irq_desc *desc)
{
	return desc->action != NULL;
}

static inline int irq_has_action(unsigned int irq)
{
	return irq_desc_has_action(irq_to_desc(irq));
}

/**
 * irq_set_handler_locked - Set irq handler from a locked region
 * @data:	Pointer to the irq_data structure which identifies the irq
 * @handler:	Flow control handler function for this interrupt
 *
 * Sets the handler in the irq descriptor associated to @data.
 *
 * Must be called with irq_desc locked and valid parameters. Typical
 * call site is the irq_set_type() callback.
 */
static inline void irq_set_handler_locked(struct irq_data *data,
					  irq_flow_handler_t handler)
{
	struct irq_desc *desc = irq_data_to_desc(data);

	desc->handle_irq = handler;
}

/**
 * irq_set_chip_handler_name_locked - Set chip, handler and name from a locked region
 * @data:	Pointer to the irq_data structure for which the chip is set
 * @chip:	Pointer to the new irq chip
 * @handler:	Flow control handler function for this interrupt
 * @name:	Name of the interrupt
 *
 * Replace the irq chip at the proper hierarchy level in @data and
 * sets the handler and name in the associated irq descriptor.
 *
 * Must be called with irq_desc locked and valid parameters.
 */
static inline void
irq_set_chip_handler_name_locked(struct irq_data *data, struct irq_chip *chip,
				 irq_flow_handler_t handler, const char *name)
{
	struct irq_desc *desc = irq_data_to_desc(data);

	desc->handle_irq = handler;
	desc->name = name;
	data->chip = chip;
}

static inline int irq_balancing_disabled(unsigned int irq)
{
	struct irq_desc *desc;

	desc = irq_to_desc(irq);
	return desc->status_use_accessors & IRQ_NO_BALANCING_MASK;
}

static inline int irq_is_percpu(unsigned int irq)
{
	struct irq_desc *desc;

	desc = irq_to_desc(irq);
	return desc->status_use_accessors & IRQ_PER_CPU;
}

static inline void
irq_set_lockdep_class(unsigned int irq, struct lock_class_key *class)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (desc)
		lockdep_set_class(&desc->lock, class);
}

#ifdef CONFIG_IRQ_PREFLOW_FASTEOI
static inline void
__irq_set_preflow_handler(unsigned int irq, irq_preflow_handler_t handler)
{
	struct irq_desc *desc;

	desc = irq_to_desc(irq);
	desc->preflow_handler = handler;
}
#endif

#endif
```
*include/linux/irqdesc.h*

Como podemos observar, ela depende da configuração `CONFIG_SPARSE_IRQ` 


#### 2. Localize onde o kernel passa o tratamento da interrupção para o driver adequado. Veja `amba_kmi_int`.
```c
static irqreturn_t amba_kmi_int(int irq, void *dev_id)
{
	struct amba_kmi_port *kmi = dev_id;
	unsigned int status = readb(KMIIR);
	int handled = IRQ_NONE;

	while (status & KMIIR_RXINTR) {
		serio_interrupt(kmi->io, readb(KMIDATA), 0);
		status = readb(KMIIR);
		handled = IRQ_HANDLED;
	}

	return handled;
}
```
Podemos notar que o `amba_kmi_init` é chamado passando-se o `dev_id` (device id)
o `dev_id` é então atribuído ao `kmi` (que é um `amba_kmi_port`) e é passado o `kmi->io` para o `serio_interrupt`
#### 3. Quando usamos a _system call_ `getchar()`, o caractér do teclado é lido de algum port. Na verdade, se lê a posição da tecla. Localize isso no código.
No driver /drivers/input/keyboard/matrix_keypad.c, temos
```c
static void matrix_keypad_scan(struct work_struct *work)
{
	struct matrix_keypad *keypad =
		container_of(work, struct matrix_keypad, work.work);
	struct input_dev *input_dev = keypad->input_dev;
	const struct matrix_keypad_platform_data *pdata = keypad->pdata;
	uint32_t new_state[MATRIX_MAX_COLS];
	int row, col, code;

	/* de-activate all columns for scanning */
	activate_all_cols(pdata, false);

	memset(new_state, 0, sizeof(new_state));

	/* assert each column and read the row status out */
	for (col = 0; col < pdata->num_col_gpios; col++) {

		activate_col(pdata, col, true);

		for (row = 0; row < pdata->num_row_gpios; row++)
			new_state[col] |=
				row_asserted(pdata, row) ? (1 << row) : 0;

		activate_col(pdata, col, false);
	}

	for (col = 0; col < pdata->num_col_gpios; col++) {
		uint32_t bits_changed;

		bits_changed = keypad->last_key_state[col] ^ new_state[col];
		if (bits_changed == 0)
			continue;

		for (row = 0; row < pdata->num_row_gpios; row++) {
			if ((bits_changed & (1 << row)) == 0)
				continue;

			code = MATRIX_SCAN_CODE(row, col, keypad->row_shift);
			input_event(input_dev, EV_MSC, MSC_SCAN, code);
			input_report_key(input_dev,
					 keypad->keycodes[code],
					 new_state[col] & (1 << row));
		}
	}
	input_sync(input_dev);

	memcpy(keypad->last_key_state, new_state, sizeof(new_state));

	activate_all_cols(pdata, true);

	/* Enable IRQs again */
	spin_lock_irq(&keypad->lock);
	keypad->scan_pending = false;
	enable_row_irqs(keypad);
	spin_unlock_irq(&keypad->lock);
}
```
#### 4. O que faz readb(KMIDATA)?
#### 5. O código lido deve ser guardado em um buffer enquanto o usuário não tecla ENTER. Localize isso no código.
#### 6. Localize onde a função __wakeup() acorda o processo que lia da tty (dentro do kernel).
Em /drivers/tty/tty_io.c, temos
```c
void tty_wakeup(struct tty_struct *tty)
{
	struct tty_ldisc *ld;

	if (test_bit(TTY_DO_WRITE_WAKEUP, &tty->flags)) {
		ld = tty_ldisc_ref(tty);
		if (ld) {
			if (ld->ops->write_wakeup)
				ld->ops->write_wakeup(tty);
			tty_ldisc_deref(ld);
		}
	}
	wake_up_interruptible_poll(&tty->write_wait, POLLOUT);
}
```
#### 7. Quando o usuário tecla ENTER, o processamento é diferente: os dados devem ser transferidos para o buffer de usuário. Localize isso no código.
