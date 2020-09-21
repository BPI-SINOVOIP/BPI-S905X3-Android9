/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef CRAS_FILE_WAIT_H_
#define CRAS_FILE_WAIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Structure used to track the current progress of a file wait. */
struct cras_file_wait;

/* Flags type for file wait. */
typedef unsigned int cras_file_wait_flag_t;

/* No flags. */
#define CRAS_FILE_WAIT_FLAG_NONE           ((cras_file_wait_flag_t)0)

/* File wait events. */
typedef enum cras_file_wait_event {
	CRAS_FILE_WAIT_EVENT_NONE,
	CRAS_FILE_WAIT_EVENT_CREATED,
	CRAS_FILE_WAIT_EVENT_DELETED,
} cras_file_wait_event_t;

/*
 * File wait callback function.
 *
 * Called for cras_file_wait events. Do not call cras_file_wait_destroy()
 * from this function.
 *
 * Args:
 *    context - Context pointer passed to cras_file_wait_start().
 *    event - Event that has occurred related to this file wait.
 *    filename - Filename associated with the event.
 */
typedef void (*cras_file_wait_callback_t)(void *context,
					  cras_file_wait_event_t event,
					  const char *filename);

/*
 * Wait for the existence of a file.
 *
 * Setup a watch with the aim of determining if the given file path exists. If
 * any parent directory is missing, then the appropriate watch is created to
 * watch for the parent (or it's parent). Watches are created or renewed while
 * this file wait structure exists.
 *
 * The callback function will be called with event CRAS_FILE_WAIT_EVENT_CREATED
 * when the file is created, moved into the directory, or if it already exists
 * when this function is called.
 *
 * After the file is found future deletion and creation events for the file can
 * be observed using the same file_wait structure and callback. When the file
 * is deleted or moved out of it's parent, the callback is called with event
 * CRAS_FILE_WAIT_EVENT_DELETED.
 *
 * Call cras_file_wait_destroy() to cancel the wait anytime and cleanup
 * resources.
 *
 * Args:
 *    file_path - Path of the file or directory that must exist.
 *    flags - CRAS_FILE_WAIT_FLAG_* values bit-wise orred together. Set to
 *            CRAS_FILE_WAIT_FLAG_NONE for now.
 *    callback - Callback function to execute to notify of file existence.
 *    callback_context - Context pointer passed to the callback function.
 *    file_wait_out - Pointer to file wait structure that is initialized.
 *
 * Returns:
 *    - 0 for success, or negative on error.
 *    - On error cras_file_wait_destroy() need not be called.
 */
int cras_file_wait_create(const char *file_path,
			  cras_file_wait_flag_t flags,
			  cras_file_wait_callback_t callback,
			  void *callback_context,
                          struct cras_file_wait **file_wait_out);

/* Returns the file-descriptor to poll for a file wait.
 *
 * Poll for POLLIN on this file decriptor. When there is data available, call
 * cras_file_wait_continue() on the associated file_wait structure.
 *
 * Args:
 *    file_wait - The associated cras_file_wait structure initialized by
 *                cras_file_wait_start().
 *
 * Returns:
 *    Non-negative file descriptor number, or -EINVAL if the file_wait
 *    structure is NULL or otherwise invalid.
 */
int cras_file_wait_get_fd(struct cras_file_wait *file_wait);

/* Dispatch a file wait event.
 *
 * Call this function when the file descriptor from cras_file_wait_fd() has
 * data ready (POLLIN). This function will call the callback provided to
 * cras_file_wait_start when there is a relevant event.
 *
 * Args:
 *    file_wait - The associated cras_file_wait structure initialized by
 *                cras_file_wait_start().
 *
 * Returns:
 *    - 0 for success, non-zero on error.
 *    - -EAGAIN or -EWOULDBLOCK when this function would have blocked.
 */
int cras_file_wait_dispatch(struct cras_file_wait *file_wait);

/* Destroy a file wait structure.
 *
 * This function can be called to cleanup a cras_file_wait structure, and it
 * will interrupt any wait that is in progress; the pointer is subsequently
 * invalid.
 *
 * Args:
 *    file_wait - The cras_file_wait structure initialized by
 *                cras_file_wait_start();
 */
void cras_file_wait_destroy(struct cras_file_wait *file_wait);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CRAS_FILE_WAIT_H_ */
