
#ifndef __RINGBUFFER_H_
#define __RINGBUFFER_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 * TYPES
 ************************************************/
typedef enum {
       RBUF_MODE_BLOCK = 0,
       RBUF_MODE_NBLOCK
 } rbuf_mode_t;

typedef void * rbuf_handle_t;

/************************************************
 * FUNCTIONS
 ************************************************/

rbuf_handle_t ringbuffer_create(int size, const char*name);
void ringbuffer_free(rbuf_handle_t handle);

int ringbuffer_read(rbuf_handle_t handle, char *dst, int count, rbuf_mode_t mode);
int ringbuffer_write(rbuf_handle_t handle, const char *src, int count, rbuf_mode_t mode);
int ringbuffer_peek(rbuf_handle_t handle, char *dst, int count, rbuf_mode_t mode);

int ringbuffer_cleanreset(rbuf_handle_t handle);

int ringbuffer_read_avail(rbuf_handle_t handle);
int ringbuffer_write_avail(rbuf_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* BUFFER_H_ */
