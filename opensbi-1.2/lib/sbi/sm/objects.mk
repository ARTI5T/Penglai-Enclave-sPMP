penglai-objs-y += enclave.o
penglai-objs-y += pmp.o
penglai-objs-y += sm.o
penglai-objs-y += thread.o
penglai-objs-y += utils.o
penglai-objs-y += platform/pmp/platform.o
penglai-objs-y += attest.o

penglai-objs-y += gm/miracl/mrcore.o
penglai-objs-y += gm/miracl/mrarth0.o
penglai-objs-y += gm/miracl/mrarth1.o
penglai-objs-y += gm/miracl/mrarth2.o
penglai-objs-y += gm/miracl/mrcurve.o
penglai-objs-y += gm/miracl/mrxgcd.o
penglai-objs-y += gm/miracl/mrarth3.o
penglai-objs-y += gm/miracl/mrjack.o
penglai-objs-y += gm/miracl/mrbits.o
penglai-objs-y += gm/miracl/mrmonty.o
penglai-objs-y += gm/miracl/mrrand.o
penglai-objs-y += gm/miracl/mrsroot.o
penglai-objs-y += gm/miracl/mrlucas.o
penglai-objs-y += gm/SM2_sv.o
penglai-objs-y += gm/SM3.o

penglai-objs-$(CONFIG_PENGLAI_FEATURE_SECURE_INTERRUPT) += device/irqchip/plic.o
penglai-objs-$(CONFIG_PENGLAI_FEATURE_SECURE_INTERRUPT) += device/irqchip/inst_sim.o