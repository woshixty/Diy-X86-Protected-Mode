#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define PDE_P   (1 << 0)
#define PDE_W   (1 << 1)
#define PDE_U   (1 << 2)
#define PDE_PS  (1 << 7)

#define MAP_ADDR 0x80000000

uint8_t map_phy_buffer[4096] __attribute__((aligned(4096))) = { 0x36 };

// 页表项
static uint32_t page_table[1024] __attribute__((aligned(4096))) = { PDE_U };

// 页目录表
uint32_t page_dir[1024] __attribute__((aligned(4096))) = {
    [0] = (0) | PDE_P | PDE_W | PDE_U | PDE_PS
};

// IDT Table
struct {
    uint16_t offset_l; 
    uint16_t selector;
    uint16_t attr; 
    uint16_t offset_h; 
} idt_table[256] __attribute__((aligned(8)));;

// 8字节对齐
struct {
    uint16_t limit_l; 
    uint16_t base_l; 
    uint16_t basehl_attr; 
    uint16_t base_limit;
} gdt_table[256] __attribute__((aligned(8))) = {
    [KERNEL_CODE_SEG / 8] = {
        .limit_l = 0xFFFF,      // 表示段的大小为4GB 
        .base_l = 0x0000,       // 段基址为0
        .basehl_attr = 0x9A00,  // 0x9A表示代码段，0x00表示可读，0x1表示可执行，0x00表示非扩展段，0x1表示32位段
        .base_limit = 0x00CF    // 0x00表示段的属性，0xCF表示段的属性，0x00表示段的属性
    },

    [KERNEL_DATA_SEG / 8] = {
        .limit_l = 0xFFFF,      // 表示段的大小为4GB 
        .base_l = 0x0000,       // 段基址为0
        .basehl_attr = 0x9200,  // 0x92表示数据段，0x00表示可读，0x00表示可写，0x00表示非扩展段，0x1表示32位段
        .base_limit = 0x00CF    // 0x00表示段的属性，0xCF表示段的属性，0x00表示段的属性
    }
};

void outb(uint8_t data, uint16_t port) 
{
    // __asm__ __volatile__("outb %[v], %[p]"::[p]"d"(port), [v]"a"(data));
    __asm__ __volatile__("outb %[v], %[p]" : : [p] "d" (port), [v] "a" (data));
}

void timer_init(void);
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
    
    page_dir[MAP_ADDR >> 22] = (uint32_t)page_table | PDE_P | PDE_W | PDE_U;
    page_table[MAP_ADDR >> 12 & 0x3FF] = (uint32_t)map_phy_buffer | PDE_P | PDE_W | PDE_U;
}