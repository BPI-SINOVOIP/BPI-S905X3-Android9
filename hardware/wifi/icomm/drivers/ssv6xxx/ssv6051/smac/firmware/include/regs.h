#ifndef _REGS_H_
#define _REGS_H_

#include <config.h>

// Frank_Yu 2012/12/22
// Special Notice : This is ONLY place to switch between sim & soc.
#if (CONFIG_SIM_PLATFORM == 0)	
	#include "../../../include/ssv6200_reg.h"
	#include "../../../include/ssv6200_aux.h"
	#include "ssv_regs.h"
	#define	REG32(addr)			(*(volatile u32 *)(addr))
	#define TO_SIM_ADDR
	#define TO_PHY_ADDR
#else
	#include "ssv_regs.h"
	#include "../../sim/src/include/sim_regs.h"
#endif


#endif /* _REGS_H_ */

