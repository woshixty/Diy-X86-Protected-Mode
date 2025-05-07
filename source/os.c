#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

void do_syscall(int func, char * str, char color) {
    static int row = 0;
    if(func == 2) {
        unsigned short * dest = (unsigned short *)0xB8000 + 80 * row;
        while (*str)
        {
            *dest++ = *str++ | (color << 8);
        }
        row = (row >= 25) ? 0 : row + 1;
    }
}

void sys_show (char * str, char color)
{
    uint32_t addr[] = {0, SYSCALL_SEG};
    __asm__ __volatile__("lcalll *(%[a])"::[a]"r"(addr));
}

void task_0 (void) {
    char * str = "task a: 1234";
    uint8_t color = 0;

    for(;;) {
        sys_show(str, color++);
    }
}

/**
 * @brief 任务1
 */
void task_1 (void) {
    char * str = "task b: 5678";
    uint8_t color = 0xff;
    for (;;) {
        sys_show(str, color--);
    }
}

/**
 * @brief 系统页表
 * 下面配置中只做了一个处理，即将0x0-4MB虚拟地址映射到0-4MB的物理地址，做恒等映射。
 */
#define PDE_P			(1 << 0)
#define PDE_W			(1 << 1)
#define PDE_U			(1 << 2)
#define PDE_PS			(1 << 7)

#define MAP_ADDR 0x80000000

uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = { 0x36 };
static uint32_t page_table[1024] __attribute__((aligned(4096))) = { PDE_U };
uint32_t page_dir[1024] __attribute__((aligned(4096))) = {
    [0] = (0) | PDE_P | PDE_W | PDE_U | PDE_PS
};

uint32_t task0_dpl3_start[1024];

// IDT Table
struct {
    uint16_t offset_l; 
    uint16_t selector;
    uint16_t attr; 
    uint16_t offset_h; 
} idt_table[256] __attribute__((aligned(8)));;

uint32_t task0_dpl0_stack[1024], task0_dpl3_stack[1024], task1_dpl0_stack[1024], task1_dpl3_stack[1024];

/**
 * @brief 任务0的任务状态段
 */
uint32_t task0_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,  (uint32_t)task0_dpl0_stack + 4*1024, KERNEL_DATA_SEG , /* 后边不用使用 */ 0x0, 0x0, 0x0, 0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)page_dir, (uint32_t)task_0/*入口地址*/, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task1_dpl0_stack + 4*1024/* 栈 */, 0x1, 0x2, 0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, 0x0, 0x0,
};

uint32_t task1_tss[] = {
    // prelink, esp0, ss0, esp1, ss1, esp2, ss2
    0,  (uint32_t)task1_dpl0_stack + 4*1024, KERNEL_DATA_SEG , /* 后边不用使用 */ 0x0, 0x0, 0x0, 0x0,
    // cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi,
    (uint32_t)page_dir, (uint32_t)task_1/*入口地址*/, 0x202, 0xa, 0xc, 0xd, 0xb, (uint32_t)task1_dpl3_stack + 4*1024/* 栈 */, 0x1, 0x2, 0x3,
    // es, cs, ss, ds, fs, gs, ldt, iomap
    APP_DATA_SEG, APP_CODE_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, APP_DATA_SEG, 0x0, 0x0,
};

// 8字节对齐
struct {
    uint16_t limit_l; 
    uint16_t base_l; 
    uint16_t basehl_attr; 
    uint16_t base_limit;
} gdt_table[256] __attribute__((aligned(8))) = {
    [KERNEL_CODE_SEG / 8] = { 0xFFFF, 0x0000, 0x9A00, 0x00CF },
    [KERNEL_DATA_SEG / 8] = { 0xFFFF, 0x0000, 0x9200, 0x00CF },

    [APP_CODE_SEG / 8] = { 0xFFFF, 0x0000, 0xFA00, 0x00CF },
    [APP_DATA_SEG / 8] = { 0xFFFF, 0x0000, 0xF300, 0x00CF },

    [TASK0_TSS_SEG / 8] = { 0x68, 0, 0xE900, 0x0 },
    [TASK1_TSS_SEG / 8] = { 0x68, 0, 0xE900, 0x0 },

    [SYSCALL_SEG / 8] = { 0x0000, KERNEL_CODE_SEG, 0xEC03, 0 }
};

void outb(uint8_t data, uint16_t port) 
{
    // __asm__ __volatile__("outb %[v], %[p]"::[p]"d"(port), [v]"a"(data));
    __asm__ __volatile__("outb %[v], %[p]" : : [p] "d" (port), [v] "a" (data));
}

void task_sched(void) {
    static int task_tss = TASK0_TSS_SEG;

    task_tss = (task_tss == TASK0_TSS_SEG) ? TASK1_TSS_SEG: TASK0_TSS_SEG;
    
    uint32_t addr[] = {0, task_tss};
    __asm__ __volatile__("ljmpl *(%[a])"::[a]"r"(addr));
}

void timer_init(void);
void syscall_handler(void);
void os_init(void) {
    outb(0x11, 0x20);
    outb(0x11, 0xA0);
    outb(0x20, 0x21);
    outb(0x28, 0xA1);
    outb(1 << 2, 0x21);
    outb(2, 0xA1);
    outb(0x1, 0x21);
    outb(0x1, 0xA1);
    outb(0xFE, 0x21);
    outb(0xFF, 0xA0);

    int tmo = 1193180 / 100;
    outb(0x36, 0x43); // 设置计数器模式
    outb(tmo & 0xFF, 0x40);
    outb(tmo >> 8, 0x40);

    idt_table[0x20].offset_l = (uint32_t)timer_init & 0xFFFF;
    idt_table[0x20].offset_h = (uint32_t)timer_init >> 16;
    idt_table[0x20].selector = KERNEL_CODE_SEG;
    idt_table[0x20].attr = 0x8E00; // 0x8E表示中断门，0x00表示可读，0x1表示可执行，0x00表示非扩展段，0x1表示32位段

    gdt_table[TASK0_TSS_SEG / 8].base_l = (uint16_t)(uint32_t)task0_tss;
    gdt_table[TASK1_TSS_SEG / 8].base_l = (uint16_t)(uint32_t)task1_tss;
    gdt_table[SYSCALL_SEG / 8].limit_l = (uint16_t)(uint32_t)syscall_handler;
    
    page_dir[MAP_ADDR >> 22] = (uint32_t)page_table | PDE_P | PDE_W | PDE_U;
    page_table[MAP_ADDR >> 12 & 0x3FF] = (uint32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U;
}