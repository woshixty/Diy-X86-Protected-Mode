#include "os.h"

	// 声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start, timer_init

	.extern os_init

	// 指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
  	.code16

	// 以下是代码区
 	.text
_start:
	mov $0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %gs
	mov %ax, %fs
	mov $_start, %esp
	
read_self_all:
	mov $_start_32, %bx	// 告诉BIOS要读入数据的地址
	mov $0x2, %cx		// 0x2 -> CH 表示读第2个扇区，0x0 -> CL 表示读第0个扇区
	mov $0x240, %ax		// 0x40 -> AL 表示读64个扇区，0x2 -> AH 表示读磁盘 
	mov $0x80, %dx		// 表示计算机上的第一个硬盘
	int $0x13
	jc read_self_all

	cli					// 禁止中断
	lgdt [gdt_desc]		// 将数据段描述符表加载到GDTR寄存器中
	lidt [idt_desc]		// 将中断描述符表加载到IDTR寄存器中
	mov $1, %eax
	lmsw %ax
	jmp $KERNEL_CODE_SEG, $_start_32

	.org 0x1fe			// 510字节处是BIOS引导扇区的标志
	.byte 0x55, 0xaa	// BIOS引导扇区的标志

	.code32
	.text
_start_32:
	mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov %ax, %gs
	mov %ax, %fs
 	mov $_start, %esp

	call os_init

	mov $page_dir, %eax		// 将页目录表的地址加载到EAX寄存器中
	mov %eax, %cr3			// 将页目录表的地址加载到CR3寄存器中

	mov %cr4, %eax			// 读取CR4寄存器的值
	orl $(1 << 4), %eax		// 设置PSE位，表示启用大页表
	mov %eax, %cr4			// 设置PSE位，表示启用大页表

	mov %cr0, %eax			// 读取CR0寄存器的值
	orl $(1 << 31), %eax	// 设置PE位，表示进入保护模式
	mov %eax, %cr0			// 将修改后的值写入CR0寄存器

	push $APP_DATA_SEG
	push $task0_dpl3_start + 1024 * 4
	push $0					// $0x202
	push $APP_CODE_SEG
	push $task_0_entry
	iret
	
timer_init:
	push %ds
	pusha
	mov $0x20, %al
	outb %al, $0x20
	popa
	pop %ds
 	iret

task_0_entry:
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %gs
	mov %ax, %fs
	jmp .

gdt_desc:
	.word (256*8) - 1	// 描述符表的大小
	.long gdt_table		// 描述符表的地址
idt_desc:
	.word (256*8) - 1	// 描述符表的大小
	.long idt_table		// 描述符表的地址