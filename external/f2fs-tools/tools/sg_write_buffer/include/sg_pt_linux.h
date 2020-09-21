#ifndef SG_PT_LINUX_H
#define SG_PT_LINUX_H

/*
 * Copyright (c) 2017 Douglas Gilbert.
 * All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the BSD_LICENSE file.
 */

#include <stdint.h>
#include <stdbool.h>

#include <linux/types.h>

#include "sg_pt_nvme.h"

/* This header is for internal use by the sg3_utils library (libsgutils)
 * and is Linux specific. Best not to include it directly in code that
 * is meant to be OS independent. */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_LINUX_BSG_H

#define BSG_PROTOCOL_SCSI               0

#define BSG_SUB_PROTOCOL_SCSI_CMD       0
#define BSG_SUB_PROTOCOL_SCSI_TMF       1
#define BSG_SUB_PROTOCOL_SCSI_TRANSPORT 2

/*
 * For flag constants below:
 * sg.h sg_io_hdr also has bits defined for it's flags member. These
 * two flag values (0x10 and 0x20) have the same meaning in sg.h . For
 * bsg the BSG_FLAG_Q_AT_HEAD flag is ignored since it is the default.
 */
#define BSG_FLAG_Q_AT_TAIL 0x10 /* default is Q_AT_HEAD */
#define BSG_FLAG_Q_AT_HEAD 0x20

struct sg_io_v4 {
        __s32 guard;            /* [i] 'Q' to differentiate from v3 */
        __u32 protocol;         /* [i] 0 -> SCSI , .... */
        __u32 subprotocol;      /* [i] 0 -> SCSI command, 1 -> SCSI task
                                   management function, .... */

        __u32 request_len;      /* [i] in bytes */
        __u64 request;          /* [i], [*i] {SCSI: cdb} */
        __u64 request_tag;      /* [i] {SCSI: task tag (only if flagged)} */
        __u32 request_attr;     /* [i] {SCSI: task attribute} */
        __u32 request_priority; /* [i] {SCSI: task priority} */
        __u32 request_extra;    /* [i] {spare, for padding} */
        __u32 max_response_len; /* [i] in bytes */
        __u64 response;         /* [i], [*o] {SCSI: (auto)sense data} */

        /* "dout_": data out (to device); "din_": data in (from device) */
        __u32 dout_iovec_count; /* [i] 0 -> "flat" dout transfer else
                                   dout_xfer points to array of iovec */
        __u32 dout_xfer_len;    /* [i] bytes to be transferred to device */
        __u32 din_iovec_count;  /* [i] 0 -> "flat" din transfer */
        __u32 din_xfer_len;     /* [i] bytes to be transferred from device */
        __u64 dout_xferp;       /* [i], [*i] */
        __u64 din_xferp;        /* [i], [*o] */

        __u32 timeout;          /* [i] units: millisecond */
        __u32 flags;            /* [i] bit mask */
        __u64 usr_ptr;          /* [i->o] unused internally */
        __u32 spare_in;         /* [i] */

        __u32 driver_status;    /* [o] 0 -> ok */
        __u32 transport_status; /* [o] 0 -> ok */
        __u32 device_status;    /* [o] {SCSI: command completion status} */
        __u32 retry_delay;      /* [o] {SCSI: status auxiliary information} */
        __u32 info;             /* [o] additional information */
        __u32 duration;         /* [o] time to complete, in milliseconds */
        __u32 response_len;     /* [o] bytes of response actually written */
        __s32 din_resid;        /* [o] din_xfer_len - actual_din_xfer_len */
        __s32 dout_resid;       /* [o] dout_xfer_len - actual_dout_xfer_len */
        __u64 generated_tag;    /* [o] {SCSI: transport generated task tag} */
        __u32 spare_out;        /* [o] */

        __u32 padding;
};

#else

#include <linux/bsg.h>

#endif


struct sg_pt_linux_scsi {
    struct sg_io_v4 io_hdr;     /* use v4 header as it is more general */
    /* Leave io_hdr in first place of this structure */
    bool is_sg;
    bool is_bsg;
    bool is_nvme;	/* OS device type, if false ignore nvme_direct */
    bool nvme_direct;	/* false: our SNTL; true: received NVMe command */
    bool mdxfer_out;    /* direction of metadata xfer, true->data-out */
    bool scsi_dsense;   /* SCSI descriptor sense active when true */
    int dev_fd;                 /* -1 if not given (yet) */
    int in_err;
    int os_err;
    uint32_t nvme_nsid;         /* 1 to 0xfffffffe are possibly valid, 0
                                 * implies dev_fd is not a NVMe device
                                 * (is_nvme=false) or it is a NVMe char
                                 * device (e.g. /dev/nvme0 ) */
    uint32_t nvme_result;       /* DW0 from completion queue */
    uint32_t nvme_status;       /* SCT|SC: DW3 27:17 from completion queue,
                                 * note: the DNR+More bit are not there.
                                 * The whole 16 byte completion q entry is
                                 * sent back as sense data */
    uint32_t mdxfer_len;
    void * mdxferp;
    uint8_t * nvme_id_ctlp;     /* cached response to controller IDENTIFY */
    uint8_t * free_nvme_id_ctlp;
    unsigned char tmf_request[4];
};

struct sg_pt_base {
    struct sg_pt_linux_scsi impl;
};


#ifndef sg_nvme_admin_cmd
#define sg_nvme_admin_cmd sg_nvme_passthru_cmd
#endif

/* Linux NVMe related ioctls */
#ifndef NVME_IOCTL_ID
#define NVME_IOCTL_ID           _IO('N', 0x40)
#endif
#ifndef NVME_IOCTL_ADMIN_CMD
#define NVME_IOCTL_ADMIN_CMD    _IOWR('N', 0x41, struct sg_nvme_admin_cmd)
#endif
#ifndef NVME_IOCTL_SUBMIT_IO
#define NVME_IOCTL_SUBMIT_IO    _IOW('N', 0x42, struct sg_nvme_user_io)
#endif
#ifndef NVME_IOCTL_IO_CMD
#define NVME_IOCTL_IO_CMD       _IOWR('N', 0x43, struct sg_nvme_passthru_cmd)
#endif
#ifndef NVME_IOCTL_RESET
#define NVME_IOCTL_RESET        _IO('N', 0x44)
#endif
#ifndef NVME_IOCTL_SUBSYS_RESET
#define NVME_IOCTL_SUBSYS_RESET _IO('N', 0x45)
#endif

extern bool sg_bsg_nvme_char_major_checked;
extern int sg_bsg_major;
extern volatile int sg_nvme_char_major;
extern long sg_lin_page_size;

void sg_find_bsg_nvme_char_major(int verbose);
int sg_do_nvme_pt(struct sg_pt_base * vp, int fd, int time_secs, int vb);

/* This trims given NVMe block device name in Linux (e.g. /dev/nvme0n1p5)
 * to the name of its associated char device (e.g. /dev/nvme0). If this
 * occurs true is returned and the char device name is placed in 'b' (as
 * long as b_len is sufficient). Otherwise false is returned. */
bool sg_get_nvme_char_devname(const char * nvme_block_devname, uint32_t b_len,
                              char * b);


#ifdef __cplusplus
}
#endif

#endif          /* end of SG_PT_LINUX_H */
