OSH LAB 01
=======================

​																						  邵军阳

​																					PB16120156

## 一、实验环境

​	·Ubuntu 17.10 64位
	·gdb
	·qemu
	·gcc
	·busybox
	通过qemu+gdb跟踪linux-4.15.14 kernel启动过程

### 系统搭建

#### 1)内核编译

在kernel.org下载了linux-4.15.14的kernel
编译：

```
cd Downloads/linux-4.15.14
make clean
make mrproper
make menuconfig
make x86_64_deconfig
make
```


其中，menuconfig使用了默认配置

#### 2)安装qemu

`sudo apt-get qemu`

#### 3)安装busybox，为了方便使用，选择了自己下载并编译

```
wget http://busybox.net/downloads/busybox-1.28.1.tar.bz2
make menuconfig
make
make install
```

在menuconfig的时候选择Settings->Build Options->Build static binary（no shared libs）

#### 4)下载一些可能用到的库

```
sudo apt-get install git libglib2.0-dev libfdt-dev libpixman-1-dev zliblg-dev

sudo apt-get install libelf-dev
```

#### 4)制作rootfs.img

参考了github上的一篇文章:(基本是使用了他的做法)
`http://freemandealer.github.io/2015/10/04/debug-kernel-with-qemu-2/`

我的代码：

```
sudo -i
#进入root模式
cd busybox-1.28.1
dd if=/dev/zero of=rootfs.img bs=1M count=10
mkfs.ext3 rootfs.img
mkdir rootfs
mount -t ext3 -o loop rootfs.img rootfs
make install CONFIG_PREFIX=rootfs
mkdir rootfs/proc rootfs/dev rootfs/etc
cp examples/bootfloppy/* rootfs/etc/ 
#上面那行语句不知道为什么我没执行成功，所以手动完成了这个copy的步骤..
umount rootfs
```

然后手动把rootfs.img复制到linux-4.15.14文件夹下

### qemu+gdb调试内核

![52250919844](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522509198442.png)

使用qemu打开虚拟机，此时跳出来一个黑色的Qemu窗口

<!--这里如果不设置断点一直运行则会出现panic错误提醒，助教说是因为initramfs没设置好，但是我暂时还不会怎么处理，所以最后只能选择只调试内核态了（也有可能是qemu版本太旧的原因，据说sudo install装下来的qemu版本是很旧的，而用新版qemu的同学好像没有这个问题）-->

然后在linux-4.15.14目录的terminal输入

```
gdb -tui
(gdb)file vmlinux
(gdb)target remote:1234
(gdb)break start_kernel
(gdb)c
```

![1](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1.png)

然后出现这个界面，说明断点成功啦

## 二、寻找开机中的关键事件

### start_kernel函数

```
asmlinkage __visible void __init start_kernel(void)
{
	char *command_line;
	char *after_dashes;

	set_task_stack_end_magic(&init_task);
	smp_setup_processor_id();
	debug_objects_early_init();

	cgroup_init_early();

	local_irq_disable();
	early_boot_irqs_disabled = true;

	/*
	 * Interrupts are still disabled. Do necessary setups, then
	 * enable them.
	 */
	boot_cpu_init();
	page_address_init();
	pr_notice("%s", linux_banner);
	setup_arch(&command_line);
	/*
	 * Set up the the initial canary and entropy after arch
	 * and after adding latent and command line entropy.
	 */
	add_latent_entropy();
	add_device_randomness(command_line, strlen(command_line));
	boot_init_stack_canary();
	mm_init_cpumask(&init_mm);
	setup_command_line(command_line);
	setup_nr_cpu_ids();
	setup_per_cpu_areas();
	boot_cpu_state_init();
	smp_prepare_boot_cpu();	/* arch-specific boot-cpu hooks */

	build_all_zonelists(NULL);
	page_alloc_init();

	pr_notice("Kernel command line: %s\n", boot_command_line);
	parse_early_param();
	after_dashes = parse_args("Booting kernel",
				  static_command_line, __start___param,
				  __stop___param - __start___param,
				  -1, -1, NULL, &unknown_bootoption);
	if (!IS_ERR_OR_NULL(after_dashes))
		parse_args("Setting init args", after_dashes, NULL, 0, -1, -1,
			   NULL, set_init_arg);

	jump_label_init();

	/*
	 * These use large bootmem allocations and must precede
	 * kmem_cache_init()
	 */
	setup_log_buf(0);
	vfs_caches_init_early();
	sort_main_extable();
	trap_init();
	mm_init();

	ftrace_init();

	/* trace_printk can be enabled here */
	early_trace_init();

	/*
	 * Set up the scheduler prior starting any interrupts (such as the
	 * timer interrupt). Full topology setup happens at smp_init()
	 * time - but meanwhile we still have a functioning scheduler.
	 */
	sched_init();
	/*
	 * Disable preemption - early bootup scheduling is extremely
	 * fragile until we cpu_idle() for the first time.
	 */
	preempt_disable();
	if (WARN(!irqs_disabled(),
		 "Interrupts were enabled *very* early, fixing it\n"))
		local_irq_disable();
	radix_tree_init();

	/*
	 * Set up housekeeping before setting up workqueues to allow the unbound
	 * workqueue to take non-housekeeping into account.
	 */
	housekeeping_init();

	/*
	 * Allow workqueue creation and work item queueing/cancelling
	 * early.  Work item execution depends on kthreads and starts after
	 * workqueue_init().
	 */
	workqueue_init_early();

	rcu_init();

	/* Trace events are available after this */
	trace_init();

	context_tracking_init();
	/* init some links before init_ISA_irqs() */
	early_irq_init();
	init_IRQ();
	tick_init();
	rcu_init_nohz();
	init_timers();
	hrtimers_init();
	softirq_init();
	timekeeping_init();
	time_init();
	sched_clock_postinit();
	printk_safe_init();
	perf_event_init();
	profile_init();
	call_function_init();
	WARN(!irqs_disabled(), "Interrupts were enabled early\n");
	early_boot_irqs_disabled = false;
	local_irq_enable();

	kmem_cache_init_late();

	/*
	 * HACK ALERT! This is early. We're enabling the console before
	 * we've done PCI setups etc, and console_init() must be aware of
	 * this. But we do want output early, in case something goes wrong.
	 */
	console_init();
	if (panic_later)
		panic("Too many boot %s vars at `%s'", panic_later,
		      panic_param);

	lockdep_info();

	/*
	 * Need to run this when irqs are enabled, because it wants
	 * to self-test [hard/soft]-irqs on/off lock inversion bugs
	 * too:
	 */
	locking_selftest();

	/*
	 * This needs to be called before any devices perform DMA
	 * operations that might use the SWIOTLB bounce buffers. It will
	 * mark the bounce buffers as decrypted so that their usage will
	 * not cause "plain-text" data to be decrypted when accessed.
	 */
	mem_encrypt_init();

#ifdef CONFIG_BLK_DEV_INITRD
	if (initrd_start && !initrd_below_start_ok &&
	    page_to_pfn(virt_to_page((void *)initrd_start)) < min_low_pfn) {
		pr_crit("initrd overwritten (0x%08lx < 0x%08lx) - disabling it.\n",
		    page_to_pfn(virt_to_page((void *)initrd_start)),
		    min_low_pfn);
		initrd_start = 0;
	}
#endif
	page_ext_init();
	kmemleak_init();
	debug_objects_mem_init();
	setup_per_cpu_pageset();
	numa_policy_init();
	acpi_early_init();
	if (late_time_init)
		late_time_init();
	calibrate_delay();
	pid_idr_init();
	anon_vma_init();
#ifdef CONFIG_X86
	if (efi_enabled(EFI_RUNTIME_SERVICES))
		efi_enter_virtual_mode();
#endif
	thread_stack_cache_init();
	cred_init();
	fork_init();
	proc_caches_init();
	buffer_init();
	key_init();
	security_init();
	dbg_late_init();
	vfs_caches_init();
	pagecache_init();
	signals_init();
	proc_root_init();
	nsfs_init();
	cpuset_init();
	cgroup_init();
	taskstats_init_early();
	delayacct_init();

	check_bugs();

	acpi_subsystem_init();
	arch_post_acpi_subsys_init();
	sfi_init_late();

	if (efi_enabled(EFI_RUNTIME_SERVICES)) {
		efi_free_boot_services();
	}

	/* Do the rest non-__init'ed, we're now alive */
	rest_init();
}

/* Call all constructor functions linked into the kernel. */
```

第一个断点设在`start_kernel`函数

可以从代码看到它调用了很多个函数

`start_kernel`函数是内核的入口，是与体系架构无关的通用处理函数的入口函数

`start_kernel`函数的主要目的是完成内核初始化并启动祖先进程(1号进程)

首先看到的是两个变量：

```
char *command_line;
char *after_dashes;
```

第一个变量表示内核命令行的全局指针，第二个变量将包含`parse_args`函数通过输入字符串中的参数'name=value'，寻找特定的关键字和调用正确的处理程序

然后执行到`set_task_stack_end_magic(&init_task);`

#### set_task_stack_end_magic(&init_task)函数

在该函数中初始化了系统第一个task_struct结构体，主要作用是设置了该任务的堆栈

使用gdb跟踪到这个函数的入口

![52255038511](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522550385116.png)

可以看到它设置了一个栈，并且设置了栈底防止溢出。

#### 激活第一个CPU

通过`boot_cpu_init`函数实现

断点跳到这个函数：

![52255128482](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522551284824.png)

该函数先取得cpu的ID，然后将cpu状态设置为online、active、present、possible

#### 体系架构初始化

接下来程序会运行到函数setup_arch

![52255887200](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522558872008.png)

这是体系架构初始化的函数，定义在setup.c中

#### 初始化硬件中断:trap_init

![52256005274](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522560052744.png)

函数中设置了很多中断门

#### 建立内核的内存分配器:mm_init

![52256013332](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522560133326.png)

可以看到这里设置了很多初始化状态，建立内核的内存分配器

#### 初始化串口:console_init

![52255966189](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522559661890.png)

#### rest_init():最后的初始化工作

![52255221101](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522552211015.png)

gdb断点跳转到此

`start_kernel`函数调用 `rest_init()`函数进行最后的初始化工作

![52255239204](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522552392043.png)

可以看到rcu调度器被调用启动

然后跳转到kernel_thread函数

![52255265714](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522552657140.png)

该函数调用了 _do_fork 进行新线程的创建.

最后调用cpu_startup_entry,再在其中调用cpu_idle_loop函数进入睡眠循环.节省CPU 能耗.（即0号进程）

## 三、实验总结

上面那些事件是我在网上查了很多资料之后断点找到的，rest_init之后再往下运行就遇到panic了

![52256110506](https://github.com/OSH-2018/1-JunyangShao/blob/master/pics-lab01/1522561105067.png)

这是我第一次接触linux，很多东西都还很生涩，犯了很多奇怪的错误，但是也从中锻炼到自己解决问题的能力。说实话，这个kernel启动的过程很多细节我在具体上都还是不理解的，希望后续学习能做的更好吧。
