/**
 * Simulates lx/sx at the (protected) IRQ chip MMIO region.
*/

#include <sm/device/irqchip/inst_sim.h>
#include <sm/print.h>

static int c_lxsp(unsigned long *regs_array, unsigned int inst, unsigned long addr) {
    int dest_reg = (inst >> 7) & 0x1F;
    unsigned long value = 0;
    int ret = 0;

    char funct3 = (inst >> 12) & 0x07;
    switch (funct3) {
        case 0b010:
            // C.LWSP
            value = (unsigned long)(*(int *)addr);
            break;
        case 0b011:
            // C.LDSP
            value = (unsigned long)(*(long *)addr);
            break;
        default:
            ret = -1;
    }

    regs_array[dest_reg] = value;
    regs_array[32] += 2;
    return ret;
}

static int c_sxsp(unsigned long *regs_array, unsigned int inst, unsigned long addr) {
    int dest_reg = (inst >> 2) & 0x1F;
    unsigned long value = regs_array[dest_reg];
    int ret = 0;

    char funct3 = (inst >> 12) & 0x07;
    switch (funct3) {
        case 0b010:
            // C.SWSP
            *(int *)addr = (int) value;
            break;
        case 0b011:
            // C.SDSP
            *(long *)addr = value;
            break;
        default:
            ret = -1;
    }

    regs_array[32] += 2;
    return ret;
}

static int c_lx(unsigned long *regs_array, unsigned int inst, unsigned long addr) {
    int dest_reg = ((inst >> 2) & 0x3) + 8;
    unsigned long value = 0;
    int ret = 0;

    char funct3 = (inst >> 12) & 0x07;
    switch (funct3) {
        case 0b010:
            // C.LW
            value = (unsigned long)(*(int *)addr);
            break;
        case 0b011:
            // C.LD
            value = (unsigned long)(*(long *)addr);
            break;
        default:
            ret = -1;
    }

    regs_array[dest_reg] = value;
    regs_array[32] += 2;
    return ret;
}

static int c_sx(unsigned long *regs_array, unsigned int inst, unsigned long addr) {
    int dest_reg = ((inst >> 2) & 0x3) + 8;
    unsigned long value = regs_array[dest_reg];
    int ret = 0;

    char funct3 = (inst >> 12) & 0x07;
    switch (funct3) {
        case 0b010:
            // C.SW
            *(int *)addr = (int) value;
            break;
        case 0b011:
            // C.SD
            *(long *)addr = value;
            break;
        default:
            ret = -1;
    }

    regs_array[32] += 2;
    return ret;
}

static int lx(unsigned long *regs_array, unsigned int inst, unsigned long addr) {
    int dest_reg = (inst >> 7) & 0x1F;
    unsigned long value = 0;
    int ret = 0;

    char funct3 = (inst >> 12) & 0x07;
    switch (funct3) {
        case 0b000:
            // lb
            value = (unsigned long)(*(char *)addr);
            break;
        case 0b001:
            // lh
            value = (unsigned long)(*(short *)addr);
            break;
        case 0b010:
            // lw
            value = (unsigned long)(*(int *)addr);
            break;
        case 0b100:
            // ld
            value = (unsigned long)(*(long *)addr);
            break;
        case 0b101:
            // lbu
            value = (unsigned long)(*(unsigned char *)addr);
            break;
        case 0b110:
            // lhu
            value = (unsigned long)(*(unsigned short *)addr);
            break;
        case 0b111:
            // lwu
            value = (unsigned long)(*(unsigned int *)addr);
            break;
        default:
            ret = -1;
    }

    regs_array[dest_reg] = value;
    regs_array[32] += 4;
    return ret;
}

static int sx(unsigned long *regs_array, unsigned int inst, unsigned long addr) {
    int dest_reg = (inst >> 7) & 0x1F;
    unsigned long value = regs_array[dest_reg];
    int ret = 0;

    char funct3 = (inst >> 12) & 0x07;
    switch (funct3) {
        case 0b000:
            // sb
            *(char *)addr = (char) value;
            break;
        case 0b001:
            // sh
            *(short *)addr = (short) value;
            break;
        case 0b010:
            // sw
            *(int *)addr = (int) value;
            break;
        case 0b100:
            // sd
            *(long *)addr = value;
            break;
        default:
            ret = -1;
    }

    regs_array[32] += 4;
    return ret;
}

int inst_load(struct sbi_trap_regs *regs, unsigned int inst, unsigned long addr) {
    printm("INST: %x\n", inst);
    switch (inst & 0x3) {
        case 0b11:
            return lx((unsigned long *)regs, inst, addr);
        case 0b10:
            return c_lxsp((unsigned long *)regs, inst, addr);
        case 0b00:
            return c_lx((unsigned long *)regs, inst, addr);
        default:
            return -1;
    }
}

int inst_store(struct sbi_trap_regs *regs, unsigned int inst, unsigned long addr) {
    printm("INST: %x\n", inst);
    switch (inst & 0x3) {
        case 0b11:
            return sx((unsigned long *)regs, inst, addr);
        case 0b10:
            return c_sxsp((unsigned long *)regs, inst, addr);
        case 0b00:
            return c_sx((unsigned long *)regs, inst, addr);
        default:
            return -1;
    }
}
