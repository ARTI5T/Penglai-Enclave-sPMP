#ifndef _SM_DEVICE_IRQ_H
#define _SM_DEVICE_IRQ_H

#include <sm/thread.h>
#include <sm/device/irqchip/plic.h>
#include <sbi/sbi_trap.h>

#define ENABLE_NESTED_INTERRUPT 0

struct enclave_trap_handler_t {
    unsigned long tvec;
    // int change_ptbr;
};

struct hart_trap_state_t {
    unsigned long prev_mip;
    unsigned long prev_mstatus;
    unsigned long prev_ptbr;
    unsigned long prev_mepc;
    int handler_running;
    struct thread_state_t context;
    struct cpu_state_t prev_cpu_state;
};

struct enclave_t;

struct hart_trap_state_t *get_hart_trap_state(int hart_id);

int platform_register_enclave_irq_listen(struct enclave_t *handler_enclave, struct irq_src_t *src);
int platform_unregister_enclave_irq_listen(struct enclave_t *handler_enclave, struct irq_src_t *src);
int platform_irqfn(struct sbi_trap_regs *regs);
int platform_uret(struct sbi_trap_regs *regs);
int platform_set_mstatus(unsigned long mstatus);
int platform_handle_interrupt_controller_mmio(struct sbi_trap_regs *regs, struct sbi_trap_info *info, unsigned long paddr);
int is_interrupt_controller_mmio_address(unsigned long addr, unsigned long *paddr);

int platform_protect_interrupt_controller_mmio(uintptr_t ptr, unsigned long size);

#endif /* _SM_DEVICE_IRQ_H */