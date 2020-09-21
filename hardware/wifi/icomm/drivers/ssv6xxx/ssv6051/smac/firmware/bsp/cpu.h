#ifndef _CPU_H_
#define _CPU_H_


extern u32 gCpuRegs[];

void cpu_get_registers(void);
void cpu_dump_register(void);

void cpu_enable_clock (s32 enable);

#endif /* _CPU_H_ */

