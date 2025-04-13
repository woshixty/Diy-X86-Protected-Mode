/**
 * 功能：32位代码，完成多任务的运行
 *
 *创建时间：2022年8月31日
 *作者：李述铜
 *联系邮箱: 527676163@qq.com
 *相关信息：此工程为《从0写x86 Linux操作系统》的前置课程，用于帮助预先建立对32位x86体系结构的理解。整体代码量不到200行（不算注释）
 *课程请见：https://study.163.com/course/introduction.htm?courseId=1212765805&_trace_c_p_k2_=0bdf1e7edda543a8b9a0ad73b5100990
 */
#include "os.h"

typedef unsigned char uint8_t;
typedef unsigned char uint16_t;
typedef unsigned char uint32_t;

// 8字节对齐
struct {
    uint16_t limit_l; 
    uint16_t base_l; 
    uint16_t basehl_attr; 
    uint16_t base_limit;
} gdt_table[256] __attribute__((aligned(8))) = {
    [KERNEL_CODE_DEG / 8] = {
        .limit_l = 0xFFFF,      // 表示段的大小为4GB 
        .base_l = 0x0000,       // 段基址为0
        .basehl_attr = 0x9A00,  // 0x9A表示代码段，0x00表示可读，0x1表示可执行，0x00表示非扩展段，0x1表示32位段
        .base_limit = 0x00CF    // 0x00表示段的属性，0xCF表示段的属性，0x00表示段的属性
    },

    [KERNEL_DATA_DEG / 8] = {
        .limit_l = 0xFFFF,      // 表示段的大小为4GB 
        .base_l = 0x0000,       // 段基址为0
        .basehl_attr = 0x9200,  // 0x92表示数据段，0x00表示可读，0x00表示可写，0x00表示非扩展段，0x1表示32位段
        .base_limit = 0x00CF    // 0x00表示段的属性，0xCF表示段的属性，0x00表示段的属性
    }
};