#ifndef OS_H
#define OS_H

#define KERNEL_CODE_SEG 0x08 // 内核代码段选择子
#define KERNEL_DATA_SEG 0x10 // 内核数据段选择子
#define APP_CODE_SEG    ((3 * 8) | 0x03) // 用户代码段选择子
#define APP_DATA_SEG    ((4 * 8) | 0x03) // 用户数据段选择子

#define TASK0_TSS_SEG   ((5 * 8))
#define TASK1_TSS_SEG   ((6 * 8))

#endif // OS_H
