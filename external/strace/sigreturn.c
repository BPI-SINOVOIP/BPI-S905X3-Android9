#include "defs.h"
#include "ptrace.h"
#include "nsig.h"
#include "regs.h"

#if defined HAVE_ASM_SIGCONTEXT_H && !defined HAVE_STRUCT_SIGCONTEXT
# include <asm/sigcontext.h>
#endif

#include "arch_sigreturn.c"

SYS_FUNC(sigreturn)
{
	arch_sigreturn(tcp);

	return RVAL_DECODED;
}
