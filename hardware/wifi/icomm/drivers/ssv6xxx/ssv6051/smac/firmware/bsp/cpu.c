#include <config.h>
#include "cpu.h"
#include <regs.h>

/*lint -save -e843 */
u32 gCpuRegs[18] = {0};
/*lint -restore */


void cpu_get_registers(void)
{
    asm volatile (
     "STMDB         SP!, {R0-R1}    \n\t"
     "LDR           R0, =gCpuRegs   \n\t"
     "ADD           R0, R0, #72     \n\t"
     "STMDB         R0!, {R1-R15}   \n\t"
     "LDR           R1, [SP]        \n\t"
     "STMDB         R0!, {R1}       \n\t"
     "MRS           R1, SPSR        \n\t"
     "STMDB         R0!, {R1}       \n\t"
     "MRS           R1, CPSR        \n\t"
     "STMDB         R0!, {R1}       \n\t"
     "LDMIA         SP!, {R0-R1}    \n\t"
     );
}


void cpu_dump_register(void)
{
    s32 i;
    //cpu_get_registers();
    for(i=0; i<16; i++) 
        printf("R%d: 0x%08x\n", i, gCpuRegs[i]);
    printf("SPSR: 0x%08x\n", gCpuRegs[16]);
    printf("CPSR: 0x%08x\n", gCpuRegs[17]);
}


void cpu_enable_clock (s32 enable)
{
    SET_MCU_CLK_EN(enable ? 1 : 0);
}

