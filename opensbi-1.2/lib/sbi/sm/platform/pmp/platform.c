#include "enclave_mm.c"
#include "platform_thread.c"

#include <sm/print.h>

int platform_init()
{
  struct pmp_config_t pmp_config;

  //Clear pmp1, this pmp is reserved for allowing kernel
  //to config page table for enclave in enclave's memory.
  //There is no need to broadcast to other hart as every
  //hart will execute this function.
  //clear_pmp(1);
  clear_pmp_and_sync(1);
  printm("[Penglai Monitor@%s] init platfrom and prepare PMP\n", __func__);
  //config the PMP 0 to protect security monitor
  pmp_config.paddr = (uintptr_t)SM_BASE;
  pmp_config.size = (unsigned long)SM_SIZE;//0x80024588
  pmp_config.mode = PMP_A_NAPOT;
  pmp_config.perm = PMP_NO_PERM;
  set_pmp_and_sync(0, pmp_config);

  //config the last PMP to allow kernel to access memory
  pmp_config.paddr = 0;
  pmp_config.size = -1UL;
  pmp_config.mode = PMP_A_NAPOT;
  pmp_config.perm = PMP_R | PMP_W | PMP_X;
  //set_pmp(NPMP-1, pmp_config);
  set_pmp_and_sync(NPMP-1, pmp_config);

  printm("[Penglai Monitor@%s] setting initial PMP ready\n", __func__);
  return 0;
}

#ifdef CONFIG_PENGLAI_FEATURE_SECURE_INTERRUPT
int platform_protect_interrupt_controller_mmio(uintptr_t ptr, unsigned long size)
{
  printm("[Penglai Monitor@%s] Setting interrupt controller PMP ready @ %p with size %d\n", __func__, (void *)ptr, (int)size);
  struct pmp_config_t pmp_config;
  pmp_config.paddr = ptr;
  pmp_config.size = size;
  pmp_config.mode = PMP_A_NAPOT;
  pmp_config.perm = PMP_NO_PERM;
  set_pmp_and_sync(NPMP-2, pmp_config);
  printm("[Penglai Monitor@%s] setting interrupt controller PMP ready\n", __func__);
  return 0;
}
#endif