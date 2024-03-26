# 安全中断方案
保护目标：
- enclave上下文不允许泄露
- 安全中断在相应的enclave中处理，否则忽略
- enclave 在U-mode处理中断

PLIC中断源：
- 简单实现：
  - Normal INT具有低优先级，Delegate到S-Mode处理，允许嵌套中断。
  - Trusted INT具有高优先级，拉起M-mode handler，M-mode转发到enclave U-mode，不允许嵌套中断（理论上也可以嵌套）
  - 保护PLIC MMIO区域。对该区域的读写陷入sm，sm判断地址是否正确，模拟指令执行。

BUG：
- PLIC PMP，读取到的指令不对，可能是页表没设置对
- Enclave退出后，随机一个hart会卡死

编译：
- platform/generic/config/defconfig:`CONFIG_PENGLAI_FEATURE_SECURE_INTERRUPT=y`