#include "negated_errno.h"

static void
get_error(struct tcb *tcp, const bool check_errno)
{
	if (check_errno && is_negated_errno(hppa_r28)) {
		tcp->u_rval = -1;
		tcp->u_error = -hppa_r28;
	} else {
		tcp->u_rval = hppa_r28;
	}
}
