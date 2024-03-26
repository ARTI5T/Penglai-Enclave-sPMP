#include <sm/device/irq.h>
#include <sm/device/irqchip/plic.h>
#include <sm/device/irqchip/inst_sim.h>
#include <sm/enclave.h>
#include <sbi/sbi_trap.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_bitops.h>
#include <sbi/sbi_bitmap.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_irqchip.h>
#include <sbi/sbi_platform.h>
#include <sbi/riscv_io.h>
#include <sm/print.h>
#include <sm/platform/pmp/enclave_mm.h>

#define PLIC_MAX_SOURCES 1024
#define PLIC_MAX_CONTEXT 4 * 64 // 15872 is too much
#define PLIC_MAX_NR 1
#define PLIC_SECURE_PRIORITY 0x3

#define PLIC_PRIORITY_BASE 0x0
#define PLIC_PENDING_BASE 0x1000
#define PLIC_ENABLE_BASE 0x2000
#define PLIC_ENABLE_STRIDE 0x80
#define PLIC_CONTEXT_BASE 0x200000
#define PLIC_CONTEXT_STRIDE 0x1000
#define PLIC_MMIO_SIZE 0x4000000

struct secure_irq_handler_t {
    u8 orig_priority;
    int eid;
};

struct plic_secure_irq_t {
    struct secure_irq_handler_t assigned_handler[PLIC_MAX_SOURCES];
    const struct plic_data *data;
};
static struct plic_secure_irq_t plic_chips[PLIC_MAX_NR];
static const int n_plic = 1;
static unsigned int hart_pending_irq[MAX_HARTS];
static struct hart_trap_state_t hart_trap_state[MAX_HARTS];

/** @note: Currently we only support 1 PLIC chip.
 *  @todo: Configure PMP to protect PLIC registers.
 */

static inline unsigned int plic_claim() {
    int context_id = current_hartid() * 2;
    /* M-mode context */
    unsigned long context_addr = plic_chips[0].data->addr + PLIC_CONTEXT_BASE + context_id * PLIC_CONTEXT_STRIDE + 0x04;
    return (unsigned int) readl((void *)context_addr);
}

static inline void plic_complete(unsigned int int_source) {
    int context_id = current_hartid() * 2;
    /* M-mode context */
    unsigned long context_addr = plic_chips[0].data->addr + PLIC_CONTEXT_BASE + context_id * PLIC_CONTEXT_STRIDE + 0x04;
    writel(int_source, (void *)context_addr);
}

static inline struct enclave_t* get_handler_enclave(unsigned long int_source) {
    int eid = plic_chips[0].assigned_handler[int_source].eid;
    if (eid >= 0) {
        return get_enclave(eid);
    }
    return NULL;
}

static inline int check_irq_src(struct irq_src_t *src) {
    return src->chip < n_plic && src->irq_src < PLIC_MAX_SOURCES;
}

static void plic_set_ie(const struct plic_data *plic, int context_id, int src_id, int enabled)
{
	volatile void *plic_ie;

	plic_ie = (char *)plic->addr +
		   PLIC_ENABLE_BASE + PLIC_ENABLE_STRIDE * context_id +
		   src_id / 32 * 4;

    unsigned int val = readl(plic_ie);
    if (enabled) {
        val |= (1 << (src_id % 32));
    } else {
        val &= ~(1 << (src_id % 32));
    }
	writel(val, plic_ie);
}

int is_interrupt_controller_mmio_address(unsigned long addr, unsigned long *paddr) {
    unsigned long satp = csr_read(CSR_SATP);
    pte_t *pte = (pte_t *)((satp & SATP64_PPN) << RISCV_PGSHIFT);
    *paddr = get_enclave_paddr_from_va(pte, (uintptr_t) addr);
    printm("[Penglai Interrupt] paddr %p %p %p\n", (void*)(*paddr), (void*)plic_chips[0].data->addr, (void*)(plic_chips[0].data->addr + PLIC_MMIO_SIZE));
    return *paddr >= plic_chips[0].data->addr && *paddr < plic_chips[0].data->addr + PLIC_MMIO_SIZE;
}

static inline int get_context_id(unsigned long addr) {
    return (addr - plic_chips[0].data->addr - PLIC_CONTEXT_BASE) / PLIC_CONTEXT_STRIDE;
}

/** @todo: Perform other checks on registers like priority and enable */
static inline int is_m_mode_context(unsigned long addr) {
    /** If addr points to claim/complete */
    if (addr >= plic_chips[0].data->addr + PLIC_CONTEXT_BASE
     && addr <= plic_chips[0].data->addr + PLIC_CONTEXT_BASE + PLIC_MAX_CONTEXT * PLIC_CONTEXT_STRIDE) {
        int context = get_context_id(addr);
        return context % 2 == 0;
    }
    return 0;
}

struct hart_trap_state_t *get_hart_trap_state(int hart_id) {
    return &hart_trap_state[hart_id];
}

/**
 * @brief Cold boot initializer
*/
int set_plic_chip(const struct plic_data *plic) {
    // if (n_plic >= PLIC_MAX_NR) {
    //     return -1;
    // }
    // plic_chips[n_plic].data = plic;
    // n_plic++;
    plic_chips[0].data = plic;
    sbi_irqchip_set_irqfn(platform_irqfn);
    
    printm("[Penglai Interrupt] Initializing PLIC\n");

    for (int i = 0; i < PLIC_MAX_SOURCES; ++i) {
        plic_chips[0].assigned_handler[i].eid = -1;
    }

    return 0;
}

/**
 * @brief Per-hart PLIC initializer (warm-boot)
*/
int secure_irq_add_plic_per_hart(const struct plic_data *plic, const int m_context_id) {
    plic_set_thresh(plic, m_context_id, PLIC_SECURE_PRIORITY - 1);
    sbi_irqchip_set_irqfn(platform_irqfn);
    
    /** FIXME: Bug in PLIC MMIO protection */
    // if (platform_protect_interrupt_controller_mmio(plic->addr, PLIC_MMIO_SIZE)) {
    //     return -2;
    // }

    printm("[Penglai Interrupt] PLIC context %d is set priority=%d\n", m_context_id, PLIC_SECURE_PRIORITY - 1);
    
    return 0;
}

int platform_register_enclave_irq_listen(struct enclave_t *handler_enclave, struct irq_src_t *src) {
    if (!check_irq_src(src)) {
        printm("[Penglai Interrupt] Registration: Invalid PLIC chip\n");
        //return -1;
        return 0;
    }
    unsigned int irq_number = src->irq_src;
    struct secure_irq_handler_t *handler = &plic_chips[src->chip].assigned_handler[irq_number];
    const struct sbi_platform *plat = sbi_platform_thishart_ptr();
    
    if (handler->eid >= 0) {
        printm("[Penglai Interrupt] Registration: IRQ already registered.\n");
        //return -2;
        return 0;
    }

    if (irq_number < PLIC_MAX_SOURCES && irq_number > 0) {
        handler->eid = handler_enclave->eid;
        handler->orig_priority = plic_get_priority(plic_chips[src->chip].data, irq_number);
        /** Disable the S-mode IE bit of the source and set its priority */
        int n_harts = sbi_platform_hart_count(plat);
        for (int i = 0; i < n_harts; ++i) {
            plic_set_ie(plic_chips[src->chip].data, i * 2, irq_number, 1);
            plic_set_ie(plic_chips[src->chip].data, i * 2 + 1, irq_number, 0);
        }
        plic_set_priority(plic_chips[src->chip].data, irq_number, handler->orig_priority + PLIC_SECURE_PRIORITY);
        printm("[Penglai Interrupt] Registered enclave %p irq %d orig_priority %d\n", handler_enclave, irq_number, handler->orig_priority);
        return 0;
    }

    printm("[Penglai Interrupt] Invalid IRQ number: %d.\n", irq_number);
    //return -1;
    return 0;
}

int platform_unregister_enclave_irq_listen(struct enclave_t *handler_enclave, struct irq_src_t *src) {
    if (!check_irq_src(src)) {
        //return -1;
        return 0;
    }
    unsigned int irq_number = src->irq_src;
    struct secure_irq_handler_t *handler = &plic_chips[src->chip].assigned_handler[irq_number];
    const struct sbi_platform *plat = sbi_platform_thishart_ptr();
    if (handler_enclave != NULL && handler->eid != handler_enclave->eid) {
        //return -2;
        return 0;
    }
    if (irq_number < PLIC_MAX_SOURCES && irq_number > 0) {
        /** Restore the S-mode IE bit and priority */
        int n_harts = sbi_platform_hart_count(plat);
        for (int i = 0; i < n_harts; ++i) {
            plic_set_ie(plic_chips[src->chip].data, i * 2, irq_number, 0);
            plic_set_ie(plic_chips[src->chip].data, i * 2 + 1, irq_number, 1);
        }
        plic_set_priority(plic_chips[src->chip].data, irq_number, handler->orig_priority);
        handler->eid = -1;
        handler->orig_priority = 0;
        return 0;
    }
    //return -1;
    return 0;
}

/**
 * @brief: The trap handler for PLIC IRQ.
*/
int platform_irqfn(struct sbi_trap_regs *regs) {
    int hart_id = current_hartid();
    printm("[Penglai Interrupt] hart %d\n", hart_id);

    if (unlikely(hart_pending_irq[hart_id] > 0)) {
        return SBI_EALREADY;
    }

	unsigned int int_source = plic_claim();

    if (int_source == 0) {
        /** Claimed by other hart */
        printm("[Penglai Interrupt] hart %d IRQ claimed by other hart. Do nothing.\n", hart_id);
        return 0;
    }

    struct enclave_t *handler = get_handler_enclave(int_source);

    printm("[Penglai Interrupt] hart %d irq %u\n", hart_id, int_source);

    if (int_source == 0) {
        /** Claimed by other harts */
        return 0;
    }

    if (handler == NULL) {
        plic_complete(int_source);
        return 0;
    }

    hart_pending_irq[hart_id] = int_source;
    unsigned long params[4] = {0 /* Chip 0 */, int_source};
    return invoke_enclave_trap_handler((uintptr_t*) regs, handler, params);  
}

/**
 * @brief: Simulates a `uret` instruction at the end of a enclave trap handler.
 *         Automatically performs PLIC completion.
*/
int platform_uret(struct sbi_trap_regs *regs) {
    int hart_id = current_hartid();

    if (hart_pending_irq[hart_id] == 0) {
        return SBI_EALREADY;
    }

    plic_complete(hart_pending_irq[hart_id]);
    hart_pending_irq[hart_id] = 0;
    return restore_from_enclave_trap_handler((uintptr_t*) regs);
}

/** @note: Currently we ignore this. Intended to make changes to the MSTATUS register in the enclave trap handler. */
int platform_set_mstatus(unsigned long mstatus) {
    return 0;
}

/**
 * @brief: 
 *   (1) The original design is to protect the entire PLIC MMIO section and perform read/write in M-mode.
 *   (2) The original design may introduce performance overhead. Either better protection strategy or fast-path is needed.
 *   (3) FIXME: Even though the original design is simple, it is still buggy:
 *       - The instruction read from MEPC is incorrect. (0xcccccccc)
 *       - Therefore, PLIC MMIO protection is currently disabled.
*/
int platform_handle_interrupt_controller_mmio(struct sbi_trap_regs *regs, struct sbi_trap_info *info, unsigned long paddr) {
    printm("[Penglai Interrupt] Illegal access %d @ %p\n", (int) info->cause, (void*)paddr);
    unsigned long satp = csr_read(CSR_SATP);
    pte_t *pte = (pte_t *)((satp & SATP64_PPN) << RISCV_PGSHIFT);
    unsigned int *inst = (unsigned int*)get_enclave_paddr_from_va(pte, (uintptr_t) info->epc);
    printm("[Penglai Interrupt] Inst paddr %p: %x, %x, %x\n", inst, *(inst - 1), *(inst), *(inst + 1));

    if (is_m_mode_context(paddr)) {
        return SBI_EDENIED;
    }

    if (info->cause == CAUSE_LOAD_ACCESS) {
        return inst_load(regs, *inst, paddr);
    } else if (info->cause == CAUSE_STORE_ACCESS) {
        return inst_store(regs, *inst, paddr);
    }
    return SBI_EFAIL;
}