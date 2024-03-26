#ifndef _SM_DEVICE_IRQCHIP_PLIC_H
#define _SM_DEVICE_IRQCHIP_PLIC_H

#include <sbi_utils/irqchip/plic.h>

/**
 * @attention:
 * Currently we only support 1 PLIC chip.
*/

struct irq_src_t {
    unsigned int chip;
    unsigned int irq_src;
};

int set_plic_chip(const struct plic_data *plic);
int secure_irq_add_plic_per_hart(const struct plic_data *plic, const int m_context_id);

#endif /* _SM_DEVICE_IRQCHIP_PLIC_H */