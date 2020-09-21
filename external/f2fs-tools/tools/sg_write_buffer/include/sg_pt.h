#ifndef SG_PT_H
#define SG_PT_H

/*
 * Copyright (c) 2005-2018 Douglas Gilbert.
 * All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the BSD_LICENSE file.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This declaration hides the fact that each implementation has its own
 * structure "derived" (using a C++ term) from this one. It compiles
 * because 'struct sg_pt_base' is only referenced (by pointer: 'objp')
 * in this interface. An instance of this structure represents the
 * context of one SCSI command. */
struct sg_pt_base;


/* The format of the version string is like this: "2.01 20090201".
 * The leading digit will be incremented if this interface changes
 * in a way that may impact backward compatibility. */
const char * scsi_pt_version();


/* Returns >= 0 if successful. If error in Unix returns negated errno. */
int scsi_pt_open_device(const char * device_name, bool read_only, int verbose);

/* Similar to scsi_pt_open_device() but takes Unix style open flags OR-ed
 * together. Returns valid file descriptor( >= 0 ) if successful, otherwise
 * returns -1 or a negated errno.
 * In Win32 O_EXCL translated to equivalent. */
int scsi_pt_open_flags(const char * device_name, int flags, int verbose);

/* Returns 0 if successful. If error in Unix returns negated errno. */
int scsi_pt_close_device(int device_fd);

/* Assumes dev_fd is an "open" file handle associated with device_name. If
 * the implementation (possibly for one OS) cannot determine from dev_fd if
 * a SCSI or NVMe pass-through is referenced, then it might guess based on
 * device_name. Returns 1 if SCSI generic pass-though device, returns 2 if
 * secondary SCSI pass-through device (in Linux a bsg device); returns 3 is
 * char NVMe device (i.e. no NSID); returns 4 if block NVMe device (includes
 * NSID), or 0 if something else (e.g. ATA block device) or dev_fd < 0.
 * If error, returns negated errno (operating system) value. */
int check_pt_file_handle(int dev_fd, const char * device_name, int verbose);


/* Creates an object that can be used to issue one or more SCSI commands
 * (or task management functions). Returns NULL if problem.
 * Once this object has been created it should be destroyed with
 * destruct_scsi_pt_obj() when it is no longer needed. */
struct sg_pt_base * construct_scsi_pt_obj(void);

/* An alternate way to create an object that can be used to issue one or
 * more SCSI commands (or task management functions). This variant
 * associate a device file descriptor (handle) with the object and a
 * verbose argument that causes error messages if errors occur. The
 * reason for this is to optionally allow the detection of NVMe devices
 * that will cause pt_device_is_nvme() to return true. Set dev_fd to
 * -1 if no open device file descriptor is available. Caller should
 *  additionally call get_scsi_pt_os_err() after this call. */
struct sg_pt_base *
        construct_scsi_pt_obj_with_fd(int dev_fd, int verbose);

/* Forget any previous dev_fd and install the one given. May attempt to
 * find file type (e.g. if pass-though) from OS so there could be an error.
 * Returns 0 for success or the same value as get_scsi_pt_os_err()
 * will return. dev_fd should be >= 0 for a valid file handle or -1 . */
int set_pt_file_handle(struct sg_pt_base * objp, int dev_fd, int verbose);

/* Valid file handles (which is the return value) are >= 0 . Returns -1
 * if there is no valid file handle. */
int get_pt_file_handle(const struct sg_pt_base * objp);

/* Clear state information held in *objp . This allows this object to be
 * used to issue more than one SCSI command. The dev_fd is remembered.
 * Use set_pt_file_handle() to change dev_fd. */
void clear_scsi_pt_obj(struct sg_pt_base * objp);

/* Set the CDB (command descriptor block) */
void set_scsi_pt_cdb(struct sg_pt_base * objp, const unsigned char * cdb,
                     int cdb_len);
/* Set the sense buffer and the maximum length that it can handle */
void set_scsi_pt_sense(struct sg_pt_base * objp, unsigned char * sense,
                       int max_sense_len);
/* Set a pointer and length to be used for data transferred from device */
void set_scsi_pt_data_in(struct sg_pt_base * objp,   /* from device */
                         unsigned char * dxferp, int dxfer_ilen);
/* Set a pointer and length to be used for data transferred to device */
void set_scsi_pt_data_out(struct sg_pt_base * objp,    /* to device */
                          const unsigned char * dxferp, int dxfer_olen);
/* Set a pointer and length to be used for metadata transferred to
 * (out_true=true) or from (out_true-false) device */
void set_pt_metadata_xfer(struct sg_pt_base * objp, unsigned char * mdxferp,
                          uint32_t mdxfer_len, bool out_true);
/* The following "set_"s implementations may be dummies */
void set_scsi_pt_packet_id(struct sg_pt_base * objp, int pack_id);
void set_scsi_pt_tag(struct sg_pt_base * objp, uint64_t tag);
void set_scsi_pt_task_management(struct sg_pt_base * objp, int tmf_code);
void set_scsi_pt_task_attr(struct sg_pt_base * objp, int attribute,
                           int priority);

/* Following is a guard which is defined when set_scsi_pt_flags() is
 * present. Older versions of this library may not have this function. */
#define SCSI_PT_FLAGS_FUNCTION 1
/* If neither QUEUE_AT_HEAD nor QUEUE_AT_TAIL are given, or both
 * are given, use the pass-through default. */
#define SCSI_PT_FLAGS_QUEUE_AT_TAIL 0x10
#define SCSI_PT_FLAGS_QUEUE_AT_HEAD 0x20
/* Set (potentially OS dependent) flags for pass-through mechanism.
 * Apart from contradictions, flags can be OR-ed together. */
void set_scsi_pt_flags(struct sg_pt_base * objp, int flags);

#define SCSI_PT_DO_START_OK 0
#define SCSI_PT_DO_BAD_PARAMS 1
#define SCSI_PT_DO_TIMEOUT 2
#define SCSI_PT_DO_NVME_STATUS 48       /* == SG_LIB_NVME_STATUS */
/* If OS error prior to or during command submission then returns negated
 * error value (e.g. Unix '-errno'). This includes interrupted system calls
 * (e.g. by a signal) in which case -EINTR would be returned. Note that
 * system call errors also can be fetched with get_scsi_pt_os_err().
 * Return 0 if okay (i.e. at the very least: command sent). Positive
 * return values are errors (see SCSI_PT_DO_* defines). If a file descriptor
 * has already been provided by construct_scsi_pt_obj_with_fd() then the
 * given 'fd' can be -1 or the same value as given to the constructor. */
int do_scsi_pt(struct sg_pt_base * objp, int fd, int timeout_secs,
               int verbose);

#define SCSI_PT_RESULT_GOOD 0
#define SCSI_PT_RESULT_STATUS 1 /* other than GOOD and CHECK CONDITION */
#define SCSI_PT_RESULT_SENSE 2
#define SCSI_PT_RESULT_TRANSPORT_ERR 3
#define SCSI_PT_RESULT_OS_ERR 4
/* highest numbered applicable category returned */
int get_scsi_pt_result_category(const struct sg_pt_base * objp);

/* If not available return 0 which implies there is no residual
 * value. If supported the number of bytes actually sent back by
 * the device is 'dxfer_ilen - get_scsi_pt_len()' bytes.  */
int get_scsi_pt_resid(const struct sg_pt_base * objp);

/* Returns SCSI status value (from device that received the command). If an
 * NVMe command was issued directly (i.e. through do_scsi_pt() then return
 * NVMe status (i.e. ((SCT << 8) | SC)) */
int get_scsi_pt_status_response(const struct sg_pt_base * objp);

/* Returns SCSI status value or, if NVMe command given to do_scsi_pt(),
 * then returns NVMe result (i.e. DWord(0) from completion queue). If
 * 'objp' is NULL then returns 0xffffffff. */
uint32_t get_pt_result(const struct sg_pt_base * objp);

/* Actual sense length returned. If sense data is present but
   actual sense length is not known, return 'max_sense_len' */
int get_scsi_pt_sense_len(const struct sg_pt_base * objp);

/* If not available return 0 (for success). */
int get_scsi_pt_os_err(const struct sg_pt_base * objp);
char * get_scsi_pt_os_err_str(const struct sg_pt_base * objp, int max_b_len,
                              char * b);

/* If not available return 0 (for success) */
int get_scsi_pt_transport_err(const struct sg_pt_base * objp);
void set_scsi_pt_transport_err(struct sg_pt_base * objp, int err);
char * get_scsi_pt_transport_err_str(const struct sg_pt_base * objp,
                                     int max_b_len, char * b);

/* If not available return -1 */
int get_scsi_pt_duration_ms(const struct sg_pt_base * objp);

/* Return true if device associated with 'objp' uses NVMe command set. To
 * be useful (in modifying the type of command sent (SCSI or NVMe) then
 * construct_scsi_pt_obj_with_fd() should be used followed by an invocation
 * of this function. */
bool pt_device_is_nvme(const struct sg_pt_base * objp);

/* If a NVMe block device (which includes the NSID) handle is associated
 * with 'objp', then its NSID is returned (values range from 0x1 to
 * 0xffffffe). Otherwise 0 is returned. */
uint32_t get_pt_nvme_nsid(const struct sg_pt_base * objp);


/* Should be invoked once per objp after other processing is complete in
 * order to clean up resources. For ever successful construct_scsi_pt_obj()
 * call there should be one destruct_scsi_pt_obj(). If the
 * construct_scsi_pt_obj_with_fd() function was used to create this object
 * then the dev_fd provided to that constructor is not altered by this
 * destructor. So the user should still close dev_fd (perhaps with
 * scsi_pt_close_device() ).  */
void destruct_scsi_pt_obj(struct sg_pt_base * objp);

#ifdef SG_LIB_WIN32
#define SG_LIB_WIN32_DIRECT 1

/* Request SPT direct interface when state_direct is 1, state_direct set
 * to 0 for the SPT indirect interface. Default setting selected by build
 * (i.e. library compile time) and is usually indirect. */
void scsi_pt_win32_direct(int state_direct);

/* Returns current SPT interface state, 1 for direct, 0 for indirect */
int scsi_pt_win32_spt_state(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
