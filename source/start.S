/**
 * 功能：16位与32位的启动混合代码
 *
 *创建时间：2022年8月31日
 *作者：李述铜
 *联系邮箱: 527676163@qq.com
 *相关信息：此工程为《从0写x86 Linux操作系统》的前置课程，用于帮助预先建立对32位x86体系结构的理解。整体代码量不到200行（不算注释）
 *课程请见：https://study.163.com/course/introduction.htm?courseId=1212765805&_trace_c_p_k2_=0bdf1e7edda543a8b9a0ad73b5100990
 */
	#include "os.h"

	// 声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start

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

	jmp .				// -exec x /32x 0x7E00

	.org 0x1fe			// 510字节处是BIOS引导扇区的标志
	.byte 0x55, 0xaa	// BIOS引导扇区的标志

_start_32:
	.fill 64*1024, 1, 0x35