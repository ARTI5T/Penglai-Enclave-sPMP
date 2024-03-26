#ifndef _SM_DEVICE_IRQCHIP_INST_SIM_H
#define _SM_DEVICE_IRQCHIP_INST_SIM_H

#include <sbi/sbi_trap.h>

int inst_load(struct sbi_trap_regs *regs, unsigned int inst, unsigned long addr);
int inst_store(struct sbi_trap_regs *regs, unsigned int inst, unsigned long addr);

#endif