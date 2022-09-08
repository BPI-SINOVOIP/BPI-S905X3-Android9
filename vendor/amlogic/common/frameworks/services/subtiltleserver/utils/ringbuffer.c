#define LOG_TAG "ringbuffer"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <utils/Log.h>

#include "ringbuffer.h"

//#define DEBUG
//#define DUMP_FILE

#ifdef DEBUG
#define DBG_printf(...) ALOGD(__VA_ARGS__)
#else
#define DBG_printf(...) ((void)0)

#endif

// Handle to a buffer structure
// Create with buffer_create(), destroy with buffer_free()
typedef struct {
    char *buffer;
    int buffer_size;
    int read_pos;
    int write_pos;
    int read_avail;
    int write_avail;
    unsigned long magic;
    char* name;

} ringbuffer_t;


#ifdef  DUMP_FILE
static void dumpbuffer(FILE *fp,  char *buf, int count) {
    int i =0;
    fprintf(fp, "add: %p %d \n", buf, count);

    for (i=0; i<count; i++) {
        fprintf(fp, "%02x ", buf[i]);
        if (i%16 == 15)  fprintf(fp, "\n");
    }

    fprintf(fp, "\n");
    fflush(fp);
}
#endif


/************************************************
 * CONSTANTS
 ************************************************/

// Magic number contained in the buffer handles to check if
// they are valid (debug)
#define MAGIC_NUMBER    0xA234C678UL

// TODO: better implementation
#define SLEEP_TIME  1000


/************************************************
 * PUBLIC API
 ************************************************/

int ringbuffer_read(rbuf_handle_t handle, char *dst, int count, rbuf_mode_t mode)
{
    ringbuffer_t *rbuf = (ringbuffer_t *)handle;

    assert(rbuf);
    assert(rbuf->magic == MAGIC_NUMBER);
    assert(rbuf->buffer);
    assert(rbuf);
    assert(count < rbuf->buffer_size);

    if (count <= 0)
        return 0;

    // Cannot read more than what is available
    count = (count < rbuf->read_avail) ? count : rbuf->read_avail;
    int over = rbuf->read_pos + count - rbuf->buffer_size;


    if (over > 0) {
        memcpy(dst, rbuf->buffer + rbuf->read_pos, count - over);
        // Roll back from the beginning of the buffer
        memcpy(dst + count - over, rbuf->buffer, over);
        rbuf->read_pos = over;
    } else {
        memcpy(dst, rbuf->buffer + rbuf->read_pos, count);
        rbuf->read_pos += count;
    }
#ifdef  DUMP_FILE
    dumpbuffer(rbuf->r_fp, dst, count);
#endif

    rbuf->write_avail += count;
    rbuf->read_avail -= count;
    DBG_printf("after read[%s]: readavail:%d write_avail:%d size:%d r_pos:%d w_pos:%d\n", rbuf->name,
        rbuf->read_avail,rbuf->write_avail, rbuf->buffer_size, rbuf->read_pos, rbuf->write_pos);


    return count;
}

int ringbuffer_peek(rbuf_handle_t handle, char *dst, int count, rbuf_mode_t mode)
{
    ringbuffer_t *rbuf = (ringbuffer_t *)handle;

    assert(rbuf);
    assert(rbuf->magic == MAGIC_NUMBER);
    assert(rbuf->buffer);
    assert(rbuf);
    assert(count < rbuf->buffer_size);

    if (count <= 0)
        return 0;

    // Cannot read more than what is available
    count = (count < rbuf->read_avail) ? count : rbuf->read_avail;
    int over = rbuf->read_pos + count - rbuf->buffer_size;

    if (over > 0) {
        memcpy(dst, rbuf->buffer + rbuf->read_pos, count - over);
        // Roll back from the beginning of the buffer
        memcpy(dst + count - over, rbuf->buffer, over);
    } else {
        memcpy(dst, rbuf->buffer + rbuf->read_pos, count);
    }
#ifdef  DUMP_FILE
    dumpbuffer(rbuf->r_fp, dst, count);
#endif

    DBG_printf("after read[%s]: readavail:%d write_avail:%d size:%d r_pos:%d w_pos:%d\n", rbuf->name,
        rbuf->read_avail,rbuf->write_avail, rbuf->buffer_size, rbuf->read_pos, rbuf->write_pos);

    return count;
}


int ringbuffer_write(rbuf_handle_t handle, const char *src, int count, rbuf_mode_t mode)
{
    ringbuffer_t *rbuf = (ringbuffer_t *)handle;
    assert(rbuf);
    assert(rbuf->magic == MAGIC_NUMBER);
    assert(rbuf->buffer);
    assert(src);

    if (count <= 0)
        return 0;

    count = (count < rbuf->write_avail) ? count : rbuf->write_avail;

    int over = rbuf->write_pos + count - rbuf->buffer_size;

#ifdef  DUMP_FILE
    dumpbuffer(rbuf->w_fp, src, count);
#endif
    if (over > 0) {
        memcpy(rbuf->buffer + rbuf->write_pos, src, count - over);
        // Roll back from the beginning of the buffer
        memcpy(rbuf->buffer, (char *)src + count - over, over);
        rbuf->write_pos = over;
    } else {
        memcpy(rbuf->buffer + rbuf->write_pos, src, count);
        rbuf->write_pos += count;
    }

    rbuf->write_avail -= count;
    rbuf->read_avail += count;
    DBG_printf("after write[%s]: readavail:%d write_avail:%d size:%d r_pos:%d w_pos:%d\n", rbuf->name,
        rbuf->read_avail,rbuf->write_avail, rbuf->buffer_size, rbuf->read_pos, rbuf->write_pos);

    return count;
}

int ringbuffer_read_avail(rbuf_handle_t handle)
{
    ringbuffer_t *rbuf = (ringbuffer_t *)handle;
    assert(rbuf);

    int val = rbuf->read_avail;

    return val;
}

int ringbuffer_write_avail(rbuf_handle_t handle)
{
    ringbuffer_t *rbuf = (ringbuffer_t *)handle;
    assert(rbuf);

    int val = rbuf->write_avail;

    return val;
}


int ringbuffer_cleanreset(rbuf_handle_t handle)
{
    ringbuffer_t *rbuf = (ringbuffer_t *)handle;
    assert(rbuf);

    rbuf->read_pos  = 0;
    rbuf->write_pos = 0;
    rbuf->read_avail= 0;
    rbuf->write_avail   = rbuf->buffer_size;
    return 0;
}

rbuf_handle_t ringbuffer_create(int size, const char*name)
{
    if (size <= 0) {
        return NULL;
    }

    // Allocate handle
    ringbuffer_t *rbuf = (ringbuffer_t *)malloc(sizeof(ringbuffer_t));
    if (!rbuf) {
        ALOGD("ERROR: failed to allocate handle\n");
        goto error;
    }

    // Initialize members
    rbuf->magic     = MAGIC_NUMBER;
    rbuf->buffer_size   = size;
    rbuf->read_pos  = 0;
    rbuf->write_pos = 0;
    rbuf->read_avail    = 0;
    rbuf->write_avail   = size;
    rbuf->name = strdup(name);


#ifdef  DUMP_FILE
    char filename[256];
    sprintf(filename, "/data/ringbuffer_%s_write.dump", name);
    rbuf->w_fp = fopen(filename, "w+");
    sprintf(filename, "/data/ringbuffer_%s_read.dump", name);
    rbuf->r_fp = fopen(filename, "w+");
#endif
    // Allocate buffer
    rbuf->buffer = (char *)malloc(size);
    if (!(rbuf->buffer)) {
        ALOGD("ERROR: failed to allocate buffer\n");
        goto error;
    }

    return rbuf;

error:
    if (rbuf)
        free(rbuf);

    return NULL;
}

void ringbuffer_free(rbuf_handle_t handle)
{
    ringbuffer_t *rbuf = (ringbuffer_t *)handle;
    if (!rbuf)
        return;
    assert(rbuf->magic == MAGIC_NUMBER);

#ifdef  DUMP_FILE
    fclose(rbuf->w_fp);
    fclose(rbuf->r_fp);
#endif

    if (rbuf->buffer) {
        free(rbuf->buffer);
    }
    free(rbuf->name);

    free(rbuf);
}
