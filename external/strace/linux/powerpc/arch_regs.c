struct pt_regs ppc_regs; /* not static */

#define ARCH_REGS_FOR_GETREGS ppc_regs
#define ARCH_PC_REG ppc_regs.nip
