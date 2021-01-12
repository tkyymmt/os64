#include <stdint.h>

#include "x86.h"
#include "isr.h"


#define IDT_GATE(x)		((x) << 0)
#define IDT_D(x)		((x) << 3)
#define IDT_DPL(x)		((x) << 5)
#define IDT_P(x)		((x) << 7)

#define IDT_EXCEPTION	(IDT_GATE(6) | IDT_D(1) | IDT_DPL(0) | IDT_P(1))
#define IDT_IRQ			(IDT_GATE(6) | IDT_D(1) | IDT_DPL(0) | IDT_P(1))
#define IDT_SYSCALL		(IDT_GATE(6) | IDT_D(1) | IDT_DPL(3) | IDT_P(1))


struct idt_entry {
    uint16_t offset_00_15;
    uint16_t segment_selector;
    uint16_t ist :3;
    uint16_t _zero1 :5;
    uint16_t type :4;
    uint16_t _zero2 :1;
    uint16_t dpl :2;
    uint16_t p :1;
    uint16_t offset_16_31;
    uint64_t offset_32_63;
    uint64_t _reserved;
}__attribute__((packed));

#define MAX_INTR_NO 256
struct idt_entry idt[MAX_INTR_NO];


#define SS_KERNEL_CODE 0x0008
#define DESC_TYPE_INTR 14
void set_idt_entry(int intr_no, void *isr)
{
	idt[intr_no].offset_00_15 = (uint64_t)isr;
	idt[intr_no].segment_selector = SS_KERNEL_CODE;
	idt[intr_no].type = DESC_TYPE_INTR;
	idt[intr_no].p = 1;
	idt[intr_no].offset_16_31 = (uint64_t)isr >> 16;
	idt[intr_no].offset_32_63 = (uint64_t)isr >> 32;
}

uint64_t idtr[2];

void idt_init()
{
    for (int i = 0; i < MAX_INTR_NO; i++)
        set_idt_entry(i, default_handler);
    
    idtr[0] = ((uint64_t)idt << 16) | (sizeof(idt) - 1);
    idtr[1] = ((uint64_t)idt >> 48);

    asm volatile("lidt %0" :: "m" (idtr));
}