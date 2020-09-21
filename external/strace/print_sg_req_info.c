/*
 * Decode struct sg_req_info.
 *
 * Copyright (c) 2017 Dmitry V. Levin <ldv@altlinux.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "defs.h"

#ifdef HAVE_SCSI_SG_H

#include DEF_MPERS_TYPE(struct_sg_req_info)

# include <scsi/sg.h>

typedef struct sg_req_info struct_sg_req_info;

#endif /* HAVE_SCSI_SG_H */

#include MPERS_DEFS

#ifdef HAVE_SCSI_SG_H

MPERS_PRINTER_DECL(int, decode_sg_req_info,
		   struct tcb *const tcp, const kernel_ulong_t arg)
{
	struct_sg_req_info info;

	if (entering(tcp))
		return 0;

	tprints(", ");
	if (!umove_or_printaddr(tcp, arg, &info)) {
		tprintf("{req_state=%hhd"
			", orphan=%hhd"
			", sg_io_owned=%hhd"
			", problem=%hhd"
			", pack_id=%d"
			", usr_ptr=",
			info.req_state,
			info.orphan,
			info.sg_io_owned,
			info.problem,
			info.pack_id);
		printaddr(ptr_to_kulong(info.usr_ptr));
		tprintf(", duration=%u}", info.duration);
	}

	return RVAL_IOCTL_DECODED;
}

#endif /* HAVE_SCSI_SG_H */
