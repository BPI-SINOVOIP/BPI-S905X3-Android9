/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This API creates multiple threads, one for control, and a thread per audio
 * stream. The control thread is used to receive messages and notifications
 * from the audio server, and manage the per-stream threads. API calls below
 * may send messages to the control thread, or directly to the server. It is
 * required that the control thread is running in order to support audio
 * streams and notifications from the server.
 *
 * The API has multiple initialization sequences, but some of those can block
 * while waiting for a response from the server.
 *
 * The following is the non-blocking API initialization sequence:
 *	cras_client_create()
 *      cras_client_set_connection_status_cb()                       (optional)
 *      cras_client_run_thread()
 *      cras_client_connect_async()
 *
 * The connection callback is executed asynchronously from the control thread
 * when the connection has been established. The connection callback should be
 * used to turn on or off interactions with any API call that communicates with
 * the audio server or starts/stops audio streams. The above is implemented by
 * cras_helper_create_connect_async().
 *
 * The following alternative (deprecated) initialization sequence can ensure
 * that the connection is established synchronously.
 *
 * Just connect to the server (no control thread):
 *      cras_client_create()
 *      cras_client_set_server_connection_cb()                       (optional)
 *   one of:
 *      cras_client_connect()                                  (blocks forever)
 *   or
 *      cras_client_connect_timeout()                      (blocks for timeout)
 *
 * For API calls below that require the control thread to be running:
 *      cras_client_run_thread();
 *      cras_client_connected_wait();                     (blocks up to 1 sec.)
 *
 * The above minus setting the connection callback is implemented within
 * cras_helper_create_connect().
 */

#ifndef CRAS_CLIENT_H_
#define CRAS_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <sys/select.h>

#include "cras_iodev_info.h"
#include "cras_types.h"
#include "cras_util.h"

struct cras_client;
struct cras_stream_params;

/* Callback for audio received or transmitted.
 * Args (All pointer will be valid - except user_arg, that's up to the user):
 *    client: The client requesting service.
 *    stream_id - Unique identifier for the stream needing data read/written.
 *    samples - Read or write samples to/form here.
 *    frames - Maximum number of frames to read or write.
 *    sample_time - Playback time for the first sample read/written.
 *    user_arg - Value passed to add_stream;
 * Return:
 *    Returns the number of frames read or written on success, or a negative
 *    number if there is a stream-fatal error. Returns EOF when the end of the
 *    stream is reached.
 */
typedef int (*cras_playback_cb_t)(struct cras_client *client,
				  cras_stream_id_t stream_id,
				  uint8_t *samples,
				  size_t frames,
				  const struct timespec *sample_time,
				  void *user_arg);

/* Callback for audio received and/or transmitted.
 * Args (All pointer will be valid - except user_arg, that's up to the user):
 *    client: The client requesting service.
 *    stream_id - Unique identifier for the stream needing data read/written.
 *    captured_samples - Read samples form here.
 *    playback_samples - Read or write samples to here.
 *    frames - Maximum number of frames to read or write.
 *    captured_time - Time the first sample was read.
 *    playback_time - Playback time for the first sample written.
 *    user_arg - Value passed to add_stream;
 * Return:
 *    Returns the number of frames read or written on success, or a negative
 *    number if there is a stream-fatal error. Returns EOF when the end of the
 *    stream is reached.
 */
typedef int (*cras_unified_cb_t)(struct cras_client *client,
				 cras_stream_id_t stream_id,
				 uint8_t *captured_samples,
				 uint8_t *playback_samples,
				 unsigned int frames,
				 const struct timespec *captured_time,
				 const struct timespec *playback_time,
				 void *user_arg);

/* Callback for handling stream errors.
 * Args:
 *    client - The client created with cras_client_create().
 *    stream_id - The ID for this stream.
 *    error - The error code,
 *    user_arg - The argument defined in cras_client_*_params_create().
 */
typedef int (*cras_error_cb_t)(struct cras_client *client,
			       cras_stream_id_t stream_id,
			       int error,
			       void *user_arg);

/* Callback for handling server error. DEPRECATED
 *
 * Deprecated by cras_server_connection_status_cb_t: use that instead.
 * This is equivalent to CRAS_CONN_STATUS_FAILED.
 *
 * This callback is executed rarely: only when the connection to the server has
 * already been interrupted and could not be re-established due to resource
 * allocation failure (memory or file-descriptors). The caller may attempt
 * to reestablish communication once those resources are available with
 * cras_client_connect_async(), or (blocking) cras_client_connect().
 *
 * Args:
 *    client - The client created with cras_client_create().
 *    user_arg - The argument defined in cras_client_set_server_errro_cb().
 */
typedef void (*cras_server_error_cb_t)(struct cras_client *client,
				       void *user_arg);

/* Server connection status. */
typedef enum cras_connection_status {
	CRAS_CONN_STATUS_FAILED,
		/* Resource allocation problem. Free resources, and retry the
		 * connection with cras_client_connect_async(), or (blocking)
		 * cras_client_connect(). Do not call cras_client_connect(),
		 * cras_client_connect_timeout(), or cras_client_destroy()
		 * from the callback. */
	CRAS_CONN_STATUS_DISCONNECTED,
		/* The control thread is attempting to reconnect to the
		 * server in the background. Any attempt to access the
		 * server will fail or block (see
		 * cras_client_set_server_message_blocking(). */
	CRAS_CONN_STATUS_CONNECTED,
		/* Connection is established. All state change callbacks
		 * have been re-registered, but audio streams must be
		 * restarted, and node state data must be updated. */
} cras_connection_status_t;

/* Callback for handling server connection status.
 *
 * See also cras_client_set_connection_status_cb(). Do not call
 * cras_client_connect(), cras_client_connect_timeout(), or
 * cras_client_destroy() from this callback.
 *
 * Args:
 *    client - The client created with cras_client_create().
 *    status - The status of the connection to the server.
 *    user_arg - The argument defined in
 *               cras_client_set_connection_status_cb().
 */
typedef void (*cras_connection_status_cb_t)(struct cras_client *client,
					    cras_connection_status_t status,
					    void *user_arg);

/* Callback for setting thread priority. */
typedef void (*cras_thread_priority_cb_t)(struct cras_client *client);

/* Callback for handling get hotword models reply. */
typedef void (*get_hotword_models_cb_t)(struct cras_client *client,
					const char *hotword_models);

/*
 * Client handling.
 */

/* Creates a new client.
 * Args:
 *    client - Filled with a pointer to the new client.
 * Returns:
 *    0 on success (*client is filled with a valid cras_client pointer).
 *    Negative error code on failure(*client will be NULL).
 */
int cras_client_create(struct cras_client **client);

/* Destroys a client.
 * Args:
 *    client - returned from "cras_client_create".
 */
void cras_client_destroy(struct cras_client *client);

/* Connects a client to the running server.
 * Waits forever (until interrupted or connected).
 * Args:
 *    client - pointer returned from "cras_client_create".
 * Returns:
 *    0 on success, or a negative error code on failure (from errno.h).
 */
int cras_client_connect(struct cras_client *client);

/* Connects a client to the running server, retries until timeout.
 * Args:
 *    client - pointer returned from "cras_client_create".
 *    timeout_ms - timeout in milliseconds or negative to wait forever.
 * Returns:
 *    0 on success, or a negative error code on failure (from errno.h).
 */
int cras_client_connect_timeout(struct cras_client *client,
				unsigned int timeout_ms);

/* Begins running the client control thread.
 *
 * Required for stream operations and other operations noted below.
 *
 * Args:
 *    client - the client to start (from cras_client_create).
 * Returns:
 *    0 on success or if the thread is already running, -EINVAL if the client
 *    pointer is NULL, or the negative result of pthread_create().
 */
int cras_client_run_thread(struct cras_client *client);

/* Stops running a client.
 * This function is executed automatically by cras_client_destroy().
 * Args:
 *    client - the client to stop (from cras_client_create).
 * Returns:
 *    0 on success or if the thread was already stopped, -EINVAL if the client
 *    isn't valid.
 */
int cras_client_stop(struct cras_client *client);

/* Wait up to 1 second for the client thread to complete the server connection.
 *
 * After cras_client_run_thread() is executed, this function can be used to
 * ensure that the connection has been established with the server and ensure
 * that any information about the server is up to date. If
 * cras_client_run_thread() has not yet been executed, or cras_client_stop()
 * was executed and thread isn't running, then this function returns -EINVAL.
 *
 * Args:
 *    client - pointer returned from "cras_client_create".
 * Returns:
 *    0 on success, or a negative error code on failure (from errno.h).
 */
int cras_client_connected_wait(struct cras_client *client);

/* Ask the client control thread to connect to the audio server.
 *
 * After cras_client_run_thread() is executed, this function can be used
 * to ask the control thread to connect to the audio server asynchronously.
 * The callback set with cras_client_set_connection_status_cb() will be
 * executed when the connection is established.
 *
 * Args:
 *    client - The client from cras_client_create().
 * Returns:
 *    0 on success, or a negative error code on failure (from errno.h).
 *    -EINVAL if the client pointer is invalid or the control thread is
 *    not running.
 */
int cras_client_connect_async(struct cras_client *client);

/* Sets server error callback. DEPRECATED
 *
 * See cras_server_error_cb_t for more information about this callback.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    err_cb - The callback function to register.
 *    user_arg - Pointer that will be passed to the callback.
 */
void cras_client_set_server_error_cb(struct cras_client *client,
				     cras_server_error_cb_t err_cb,
				     void *user_arg);

/* Sets server connection status callback.
 *
 * See cras_connection_status_t for a description of the connection states
 * and appropriate user action.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    connection_cb - The callback function to register.
 *    user_arg - Pointer that will be passed to the callback.
 */
void cras_client_set_connection_status_cb(
		struct cras_client *client,
		cras_connection_status_cb_t connection_cb,
		void *user_arg);

/* Sets callback for setting thread priority.
 * Args:
 *    client - The client from cras_client_create.
 *    cb - The thread priority callback.
 */
void cras_client_set_thread_priority_cb(struct cras_client *client,
					cras_thread_priority_cb_t cb);

/* Returns the current list of output devices.
 *
 * Requires that the connection to the server has been established.
 *
 * Data is copied and thus can become out of date. This call must be
 * re-executed to get updates.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    devs - Array that will be filled with device info.
 *    nodes - Array that will be filled with node info.
 *    *num_devs - Maximum number of devices to put in the array.
 *    *num_nodes - Maximum number of nodes to put in the array.
 * Returns:
 *    0 on success, -EINVAL if the client isn't valid or isn't running.
 *    *num_devs is set to the actual number of devices info filled.
 *    *num_nodes is set to the actual number of nodes info filled.
 */
int cras_client_get_output_devices(const struct cras_client *client,
				   struct cras_iodev_info *devs,
				   struct cras_ionode_info *nodes,
				   size_t *num_devs, size_t *num_nodes);

/* Returns the current list of input devices.
 *
 * Requires that the connection to the server has been established.
 *
 * Data is copied and thus can become out of date. This call must be
 * re-executed to get updates.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    devs - Array that will be filled with device info.
 *    nodes - Array that will be filled with node info.
 *    *num_devs - Maximum number of devices to put in the array.
 *    *num_nodes - Maximum number of nodes to put in the array.
 * Returns:
 *    0 on success, -EINVAL if the client isn't valid or isn't running.
 *    *num_devs is set to the actual number of devices info filled.
 *    *num_nodes is set to the actual number of nodes info filled.
 */
int cras_client_get_input_devices(const struct cras_client *client,
				  struct cras_iodev_info *devs,
				  struct cras_ionode_info *nodes,
				  size_t *num_devs, size_t *num_nodes);

/* Returns the current list of clients attached to the server.
 *
 * Requires that the connection to the server has been established.
 *
 * Data is copied and thus can become out of date. This call must be
 * re-executed to get updates.
 *
 * Args:
 *    client - This client (from cras_client_create).
 *    clients - Array that will be filled with a list of attached clients.
 *    max_clients - Maximum number of clients to put in the array.
 * Returns:
 *    The number of attached clients.  This may be more that max_clients passed
 *    in, this indicates that all of the clients wouldn't fit in the provided
 *    array.
 */
int cras_client_get_attached_clients(const struct cras_client *client,
				     struct cras_attached_client_info *clients,
				     size_t max_clients);

/* Find a node info with the matching node id.
 *
 * Requires that the connection to the server has been established.
 *
 * Data is copied and thus can become out of date. This call must be
 * re-executed to get updates.
 *
 * Args:
 *    client - This client (from cras_client_create).
 *    input - Non-zero for input nodes, zero for output nodes.
 *    node_id - The node id to look for.
 *    node_info - The information about the ionode will be returned here.
 * Returns:
 *    0 if successful, negative on error; -ENOENT if the node cannot be found.
 */
int cras_client_get_node_by_id(const struct cras_client *client,
			       int input,
			       const cras_node_id_t node_id,
			       struct cras_ionode_info* node_info);

/* Checks if the output device with the given name is currently plugged in.
 *
 * For internal devices this checks that jack state, for USB devices this will
 * always be true if they are present. The name parameter can be the complete
 * name or any unique prefix of the name. If the name is not unique the first
 * matching name will be checked.
 *
 * Requires that the connection to the server has been established.
 *
 * Data is copied and thus can become out of date. This call must be
 * re-executed to get updates.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    name - Name of the device to check.
 * Returns:
 *    1 if the device exists and is plugged, 0 otherwise.
 */
int cras_client_output_dev_plugged(const struct cras_client *client,
				   const char *name);

/* Set the value of an attribute of an ionode.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    node_id - The id of the ionode.
 *    attr - the attribute we want to change.
 *    value - the value we want to set.
 * Returns:
 *    Returns 0 for success, negative on error (from errno.h).
 */
int cras_client_set_node_attr(struct cras_client *client,
			      cras_node_id_t node_id,
			      enum ionode_attr attr,
			      int value);

/* Select the preferred node for playback/capture.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    direction - The direction of the ionode.
 *    node_id - The id of the ionode. If node_id is the special value 0, then
 *        the preference is cleared and cras will choose automatically.
 */
int cras_client_select_node(struct cras_client *client,
			    enum CRAS_STREAM_DIRECTION direction,
			    cras_node_id_t node_id);

/* Adds an active node for playback/capture.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    direction - The direction of the ionode.
 *    node_id - The id of the ionode. If there's no node matching given
 *        id, nothing will happen in CRAS.
 */
int cras_client_add_active_node(struct cras_client *client,
				enum CRAS_STREAM_DIRECTION direction,
				cras_node_id_t node_id);

/* Removes an active node for playback/capture.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    direction - The direction of the ionode.
 *    node_id - The id of the ionode. If there's no node matching given
 *        id, nothing will happen in CRAS.
 */
int cras_client_rm_active_node(struct cras_client *client,
			       enum CRAS_STREAM_DIRECTION direction,
			       cras_node_id_t node_id);


/* Asks the server to reload dsp plugin configuration from the ini file.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    0 on success, -EINVAL if the client isn't valid or isn't running.
 */
int cras_client_reload_dsp(struct cras_client *client);

/* Asks the server to dump current dsp information to syslog.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    0 on success, -EINVAL if the client isn't valid or isn't running.
 */
int cras_client_dump_dsp_info(struct cras_client *client);

/* Asks the server to dump current audio thread information.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    cb - A function to call when the data is received.
 * Returns:
 *    0 on success, -EINVAL if the client isn't valid or isn't running.
 */
int cras_client_update_audio_debug_info(
	struct cras_client *client, void (*cb)(struct cras_client *));

/*
 * Stream handling.
 */

/* Setup stream configuration parameters.
 * Args:
 *    direction - playback(CRAS_STREAM_OUTPUT) or capture(CRAS_STREAM_INPUT).
 *    buffer_frames - total number of audio frames to buffer (dictates latency).
 *    cb_threshold - For playback, call back for more data when the buffer
 *        reaches this level. For capture, this is ignored (Audio callback will
 *        be called when buffer_frames have been captured).
 *    unused - No longer used.
 *    stream_type - media or talk (currently only support "default").
 *    flags - None currently used.
 *    user_data - Pointer that will be passed to the callback.
 *    aud_cb - Called when audio is needed(playback) or ready(capture). Allowed
 *        return EOF to indicate that the stream should terminate.
 *    err_cb - Called when there is an error with the stream.
 *    format - The format of the audio stream.  Specifies bits per sample,
 *        number of channels, and sample rate.
 */
struct cras_stream_params *cras_client_stream_params_create(
		enum CRAS_STREAM_DIRECTION direction,
		size_t buffer_frames,
		size_t cb_threshold,
		size_t unused,
		enum CRAS_STREAM_TYPE stream_type,
		uint32_t flags,
		void *user_data,
		cras_playback_cb_t aud_cb,
		cras_error_cb_t err_cb,
		struct cras_audio_format *format);

/* Setup stream configuration parameters.
 * Args:
 *    direction - playback(CRAS_STREAM_OUTPUT) or capture(CRAS_STREAM_INPUT) or
 *        loopback(CRAS_STREAM_POST_MIX_PRE_DSP).
 *    block_size - The number of frames per callback(dictates latency).
 *    stream_type - media or talk (currently only support "default").
 *    flags - None currently used.
 *    user_data - Pointer that will be passed to the callback.
 *    unified_cb - Called for streams that do simultaneous input/output.
 *    err_cb - Called when there is an error with the stream.
 *    format - The format of the audio stream.  Specifies bits per sample,
 *        number of channels, and sample rate.
 */
struct cras_stream_params *cras_client_unified_params_create(
		enum CRAS_STREAM_DIRECTION direction,
		unsigned int block_size,
		enum CRAS_STREAM_TYPE stream_type,
		uint32_t flags,
		void *user_data,
		cras_unified_cb_t unified_cb,
		cras_error_cb_t err_cb,
		struct cras_audio_format *format);

/* Destroy stream params created with cras_client_stream_params_create. */
void cras_client_stream_params_destroy(struct cras_stream_params *params);

/* Creates a new stream and return the stream id or < 0 on error.
 *
 * Requires execution of cras_client_run_thread(), and an active connection
 * to the audio server.
 *
 * Args:
 *    client - The client to add the stream to (from cras_client_create).
 *    stream_id_out - On success will be filled with the new stream id.
 *        Guaranteed to be set before any callbacks are made.
 *    config - The cras_stream_params struct specifying the parameters for the
 *        stream.
 * Returns:
 *    0 on success, negative error code on failure (from errno.h).
 */
int cras_client_add_stream(struct cras_client *client,
			   cras_stream_id_t *stream_id_out,
			   struct cras_stream_params *config);

/* Creates a pinned stream and return the stream id or < 0 on error.
 *
 * Requires execution of cras_client_run_thread(), and an active connection
 * to the audio server.
 *
 * Args:
 *    client - The client to add the stream to (from cras_client_create).
 *    dev_idx - Index of the device to attach the newly created stream.
 *    stream_id_out - On success will be filled with the new stream id.
 *        Guaranteed to be set before any callbacks are made.
 *    config - The cras_stream_params struct specifying the parameters for the
 *        stream.
 * Returns:
 *    0 on success, negative error code on failure (from errno.h).
 */
int cras_client_add_pinned_stream(struct cras_client *client,
				  uint32_t dev_idx,
				  cras_stream_id_t *stream_id_out,
				  struct cras_stream_params *config);

/* Removes a currently playing/capturing stream.
 *
 * Requires execution of cras_client_run_thread().
 *
 * Args:
 *    client - Client to remove the stream (returned from cras_client_create).
 *    stream_id - ID returned from cras_client_add_stream to identify the stream
          to remove.
 * Returns:
 *    0 on success negative error code on failure (from errno.h).
 */
int cras_client_rm_stream(struct cras_client *client,
			  cras_stream_id_t stream_id);

/* Sets the volume scaling factor for the given stream.
 *
 * Requires execution of cras_client_run_thread().
 *
 * Args:
 *    client - Client owning the stream.
 *    stream_id - ID returned from cras_client_add_stream.
 *    volume_scaler - 0.0-1.0 the new value to scale this stream by.
 */
int cras_client_set_stream_volume(struct cras_client *client,
				  cras_stream_id_t stream_id,
				  float volume_scaler);

/*
 * System level functions.
 */

/* Sets the volume of the system.
 *
 * Volume here ranges from 0 to 100, and will be translated to dB based on the
 * output-specific volume curve.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    volume - 0-100 the new volume index.
 * Returns:
 *    0 for success, -EPIPE if there is an I/O error talking to the server, or
 *    -EINVAL if 'client' is invalid.
 */
int cras_client_set_system_volume(struct cras_client *client, size_t volume);

/* Sets the capture gain of the system.
 *
 * Gain is specified in dBFS * 100.  For example 5dB of gain would be specified
 * with an argument of 500, while -10 would be specified with -1000.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    gain - The gain in dBFS * 100.
 * Returns:
 *    0 for success, -EPIPE if there is an I/O error talking to the server, or
 *    -EINVAL if 'client' is invalid.
 */
int cras_client_set_system_capture_gain(struct cras_client *client, long gain);

/* Sets the mute state of the system.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    mute - 0 is un-mute, 1 is muted.
 * Returns:
 *    0 for success, -EPIPE if there is an I/O error talking to the server, or
 *    -EINVAL if 'client' is invalid.
 */
int cras_client_set_system_mute(struct cras_client *client, int mute);

/* Sets the user mute state of the system.
 *
 * This is used for mutes caused by user interaction. Like the mute key.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    mute - 0 is un-mute, 1 is muted.
 * Returns:
 *    0 for success, -EPIPE if there is an I/O error talking to the server, or
 *    -EINVAL if 'client' is invalid.
 */
int cras_client_set_user_mute(struct cras_client *client, int mute);

/* Sets the mute locked state of the system.
 *
 * Changing mute state is impossible when this flag is set to locked.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    locked - 0 is un-locked, 1 is locked.
 * Returns:
 *    0 for success, -EPIPE if there is an I/O error talking to the server, or
 *    -EINVAL if 'client' is invalid.
 */
int cras_client_set_system_mute_locked(struct cras_client *client, int locked);

/* Sets the capture mute state of the system.
 *
 * Recordings will be muted when this is set.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    mute - 0 is un-mute, 1 is muted.
 * Returns:
 *    0 for success, -EPIPE if there is an I/O error talking to the server, or
 *    -EINVAL if 'client' is invalid.
 */
int cras_client_set_system_capture_mute(struct cras_client *client, int mute);

/* Sets the capture mute locked state of the system.
 *
 * Changing mute state is impossible when this flag is set to locked.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    locked - 0 is un-locked, 1 is locked.
 * Returns:
 *    0 for success, -EPIPE if there is an I/O error talking to the server, or
 *    -EINVAL if 'client' is invalid.
 */
int cras_client_set_system_capture_mute_locked(struct cras_client *client,
					       int locked);

/* Gets the current system volume.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    The current system volume between 0 and 100.
 */
size_t cras_client_get_system_volume(const struct cras_client *client);

/* Gets the current system capture gain.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    The current system capture volume in dB * 100.
 */
long cras_client_get_system_capture_gain(const struct cras_client *client);

/* Gets the current system mute state.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    0 if not muted, 1 if it is.
 */
int cras_client_get_system_muted(const struct cras_client *client);

/* Gets the current user mute state.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    0 if not muted, 1 if it is.
 */
int cras_client_get_user_muted(const struct cras_client *client);

/* Gets the current system capture mute state.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    0 if capture is not muted, 1 if it is.
 */
int cras_client_get_system_capture_muted(const struct cras_client *client);

/* Gets the current minimum system volume.
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    The minimum value for the current output device in dBFS * 100.  This is
 *    the level of attenuation at volume == 1.
 */
long cras_client_get_system_min_volume(const struct cras_client *client);

/* Gets the current maximum system volume.
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    The maximum value for the current output device in dBFS * 100.  This is
 *    the level of attenuation at volume == 100.
 */
long cras_client_get_system_max_volume(const struct cras_client *client);

/* Gets the current minimum system capture gain.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    The minimum capture gain for the current input device in dBFS * 100.
 */
long cras_client_get_system_min_capture_gain(const struct cras_client *client);

/* Gets the current maximum system capture gain.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    The maximum capture gain for the current input device in dBFS * 100.
 */
long cras_client_get_system_max_capture_gain(const struct cras_client *client);

/* Gets audio debug info.
 *
 * Requires that the connection to the server has been established.
 * Access to the resulting pointer is not thread-safe.
 *
 * Args:
 *    client - The client from cras_client_create.
 * Returns:
 *    A pointer to the debug info.  This info is only updated when requested by
 *    calling cras_client_update_audio_debug_info.
 */
const struct audio_debug_info *cras_client_get_audio_debug_info(
		const struct cras_client *client);

/* Gets the number of streams currently attached to the server.
 *
 * This is the total number of capture and playback streams. If the ts argument
 * is not null, then it will be filled with the last time audio was played or
 * recorded. ts will be set to the current time if streams are currently
 * active.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    ts - Filled with the timestamp of the last stream.
 * Returns:
 *    The number of active streams.
 */
unsigned cras_client_get_num_active_streams(const struct cras_client *client,
					    struct timespec *ts);


/*
 * Utility functions.
 */

/* Returns the number of bytes in an audio frame for a stream.
 * Args:
 *    format - The format of the audio stream.  Specifies bits per sample,
 *        number of channels, and sample rate.
 * Returns:
 *   Positive number of bytes in a frame, or a negative error code if fmt is
 *   NULL.
 */
int cras_client_format_bytes_per_frame(struct cras_audio_format *fmt);

/* For playback streams, calculates the latency of the next sample written.
 * Only valid when called from the audio callback function for the stream
 * (aud_cb).
 * Args:
 *    sample_time - The sample time stamp passed in to aud_cb.
 *    delay - Out parameter will be filled with the latency.
 * Returns:
 *    0 on success, -EINVAL if delay is NULL.
 */
int cras_client_calc_playback_latency(const struct timespec *sample_time,
				      struct timespec *delay);

/* For capture returns the latency of the next frame to be read from the buffer
 * (based on when it was captured).  Only valid when called from the audio
 * callback function for the stream (aud_cb).
 * Args:
 *    sample_time - The sample time stamp passed in to aud_cb.
 *    delay - Out parameter will be filled with the latency.
 * Returns:
 *    0 on success, -EINVAL if delay is NULL.
 */
int cras_client_calc_capture_latency(const struct timespec *sample_time,
				     struct timespec *delay);

/* Set the volume of the given output node. Only for output nodes.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    node_id - ID of the node.
 *    volume - New value for node volume.
 */
int cras_client_set_node_volume(struct cras_client *client,
				cras_node_id_t node_id,
				uint8_t volume);

/* Swap the left and right channel of the given node.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    node_id - ID of the node.
 *    enable - 1 to enable swap mode, 0 to disable.
 */
int cras_client_swap_node_left_right(struct cras_client *client,
					cras_node_id_t node_id, int enable);

/* Set the capture gain of the given input node.  Only for input nodes.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    node_id - ID of the node.
 *    gain - New capture gain for the node.
 */
int cras_client_set_node_capture_gain(struct cras_client *client,
				      cras_node_id_t node_id,
				      long gain);

/* Add a test iodev to the iodev list.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    type - The type of test iodev, see cras_types.h
 */
int cras_client_add_test_iodev(struct cras_client *client,
			       enum TEST_IODEV_TYPE type);

/* Send a test command to a test iodev.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    iodev_idx - The index of the test iodev.
 *    command - The command to send.
 *    data_len - Length of command data.
 *    data - Command data.
 */
int cras_client_test_iodev_command(struct cras_client *client,
				   unsigned int iodev_idx,
				   enum CRAS_TEST_IODEV_CMD command,
				   unsigned int data_len,
				   const uint8_t *data);

/* Finds the first device that contains a node of the given type.
 *
 * This is used for finding a special hotword device.
 *
 * Requires that the connection to the server has been established.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    type - The type of device to find.
 *    direction - Search input or output devices.
 * Returns the device index of a negative error on failure.
 */
int cras_client_get_first_dev_type_idx(const struct cras_client *client,
				       enum CRAS_NODE_TYPE type,
				       enum CRAS_STREAM_DIRECTION direction);

/* Sets the suspend state of audio playback and capture.
 *
 * Set this before putting the system into suspend.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    suspend - Suspend the system if non-zero, otherwise resume.
 */
int cras_client_set_suspend(struct cras_client *client, int suspend);

/* Configures the global converter for output remixing.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    num_channels - Number of output channels.
 *    coefficient - Float array representing |num_channels| * |num_channels|
 *        matrix. Channels of mixed PCM output will be remixed by
 *        multiplying this matrix.
 */
int cras_client_config_global_remix(struct cras_client *client,
				    unsigned num_channels,
				    float *coefficient);

/* Gets the set of supported hotword language models on a node. The supported
 * models may differ on different nodes.
 *
 * Args:
 *    client - The client from cras_client_create.
 *    node_id - ID of a hotword input node (CRAS_NODE_TYPE_HOTWORD).
 *    cb - The function to be called when hotword models are ready.
 * Returns:
 *    0 on success.
 */
int cras_client_get_hotword_models(struct cras_client *client,
				     cras_node_id_t node_id,
				     get_hotword_models_cb_t cb);

/* Sets the hotword language model on a node. If there are existing streams on
 * the hotword input node when this function is called, they need to be closed
 * then re-opend for the model change to take effect.
 * Args:
 *    client - The client from cras_client_create.
 *    node_id - ID of a hotword input node (CRAS_NODE_TYPE_HOTWORD).
 *    model_name - Name of the model to use, e.g. "en_us".
 * Returns:
 *    0 on success.
 *    -EINVAL if client or node_id is invalid.
 *    -ENOENT if the specified model is not found.
 */
int cras_client_set_hotword_model(struct cras_client *client,
				  cras_node_id_t node_id,
				  const char *model_name);

/* Set the context pointer for system state change callbacks.
 * Args:
 *    client - The client from cras_client_create.
 *    context - The context pointer passed to all callbacks.
 */
void cras_client_set_state_change_callback_context(
		struct cras_client *client, void *context);

/* Output volume change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    volume - The system output volume, ranging from 0 to 100.
 */
typedef void (*cras_client_output_volume_changed_callback)(
		void* context, int32_t volume);

/* Output mute change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    muted - Non-zero when the audio is muted, zero otherwise.
 *    user_muted - Non-zero when the audio has been muted by the
 *                 user, zero otherwise.
 *    mute_locked - Non-zero when the mute funcion is locked,
 *                  zero otherwise.
 */
typedef void (*cras_client_output_mute_changed_callback)(
		void* context, int muted, int user_muted, int mute_locked);

/* Capture gain change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    gain - The system capture gain, in centi-decibels.
 */
typedef void (*cras_client_capture_gain_changed_callback)(
		void* context, int32_t gain);

/* Capture mute change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    muted - Non-zero when the audio is muted, zero otherwise.
 *    mute_locked - Non-zero when the mute funcion is locked,
 *                  zero otherwise.
 */
typedef void (*cras_client_capture_mute_changed_callback)(
		void* context, int muted, int mute_locked);

/* Nodes change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 */
typedef void (*cras_client_nodes_changed_callback)(void* context);

/* Active node change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    direction - Indicates the direction of the selected node.
 *    node_id - The ID of the selected node. Special device ID values
 *              defined by CRAS_SPECIAL_DEVICE will be used when no other
 *              device or node is selected or between selections.
 */
typedef void (*cras_client_active_node_changed_callback)(
    void* context, enum CRAS_STREAM_DIRECTION direction,
    cras_node_id_t node_id);

/* Output node volume change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    node_id - The ID of the output node.
 *    volume - The volume for this node with range 0 to 100.
 */
typedef void (*cras_client_output_node_volume_changed_callback)(
		void* context, cras_node_id_t node_id, int32_t volume);

/* Node left right swapped change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    node_id - The ID of the node.
 *    swapped - Non-zero if the node is left-right swapped, zero otherwise.
 */
typedef void (*cras_client_node_left_right_swapped_changed_callback)(
		void* context, cras_node_id_t node_id, int swapped);

/* Input node gain change callback.
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    node_id - The ID of the input node.
 *    gain - The gain for this node in centi-decibels.
 */
typedef void (*cras_client_input_node_gain_changed_callback)(
		void* context, cras_node_id_t node_id, int32_t gain);

/* Number of active streams change callback.
 *
 * Args:
 *    context - Context pointer set with
 *              cras_client_set_state_change_callback_context().
 *    direction - Indicates the direction of the stream's node.
 *    num_active_streams - The number of active streams.
 */
typedef void (*cras_client_num_active_streams_changed_callback)(
		void* context, enum CRAS_STREAM_DIRECTION direction,
		uint32_t num_active_streams);

/* Set system state information callbacks.
 * NOTE: These callbacks are executed from the client control thread.
 * Each state change callback is given the context pointer set with
 * cras_client_set_state_change_callback_context(). The context pointer is
 * NULL by default.
 * Args:
 *    client - The client from cras_client_create.
 *    cb - The callback, or NULL to disable the call-back.
 * Returns:
 *    0 for success or negative errno error code on error.
 */
int cras_client_set_output_volume_changed_callback(
		struct cras_client *client,
		cras_client_output_volume_changed_callback cb);
int cras_client_set_output_mute_changed_callback(
		struct cras_client *client,
		cras_client_output_mute_changed_callback cb);
int cras_client_set_capture_gain_changed_callback(
		struct cras_client *client,
		cras_client_capture_gain_changed_callback cb);
int cras_client_set_capture_mute_changed_callback(
		struct cras_client *client,
		cras_client_capture_mute_changed_callback cb);
int cras_client_set_nodes_changed_callback(
		struct cras_client *client,
		cras_client_nodes_changed_callback cb);
int cras_client_set_active_node_changed_callback(
		struct cras_client *client,
		cras_client_active_node_changed_callback cb);
int cras_client_set_output_node_volume_changed_callback(
		struct cras_client *client,
		cras_client_output_node_volume_changed_callback cb);
int cras_client_set_node_left_right_swapped_changed_callback(
		struct cras_client *client,
		cras_client_node_left_right_swapped_changed_callback cb);
int cras_client_set_input_node_gain_changed_callback(
		struct cras_client *client,
		cras_client_input_node_gain_changed_callback cb);
int cras_client_set_num_active_streams_changed_callback(
		struct cras_client *client,
		cras_client_num_active_streams_changed_callback cb);

#ifdef __cplusplus
}
#endif

#endif /* CRAS_CLIENT_H_ */
