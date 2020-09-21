/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2011-2018 ARM or its affiliates
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2.
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>

#include "acamera_logger.h"

#include "isp-v4l2-common.h"
#include "isp-v4l2.h"
#include "isp-v4l2-ctrl.h"
#include "isp-v4l2-stream.h"
#include "isp-vb2.h"
#include "fw-interface.h"
#include <linux/dma-mapping.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-contiguous.h>
#include "system_am_sc.h"

#define ISP_V4L2_NUM_INPUTS 1


/* isp_v4l2_dev_t to destroy video device */
static isp_v4l2_dev_t *g_isp_v4l2_devs[FIRMWARE_CONTEXT_NUMBER];
static isp_v4l2_dev_t *g_isp_v4l2_dev;
void *isp_kaddr = NULL;
resource_size_t isp_paddr = 0;
#define SIZE_1M (1024 * 1024UL)
#define DEFAULT_TEMPER_BUFFER_SIZE 16
#define DEFAULT_TEMPER_LINE_OFFSET 1920*4
unsigned int temper_line_offset = DEFAULT_TEMPER_LINE_OFFSET;
/* ----------------------------------------------------------------
 * V4L2 file handle structures and functions
 * : implementing multi stream
 */
#define fh_to_private( __fh ) \
    container_of( __fh, struct isp_v4l2_fh, fh )

struct isp_v4l2_fh {
    struct v4l2_fh fh;
    unsigned int stream_id;
    struct vb2_queue vb2_q;
};

static int isp_v4l2_fh_open( struct file *file )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp;
    int i;

    sp = kzalloc( sizeof( struct isp_v4l2_fh ), GFP_KERNEL );
    if ( !sp )
        return -ENOMEM;

    unsigned int stream_opened = atomic_read( &dev->opened );
    /* update open counter */

    if ( stream_opened > V4L2_STREAM_TYPE_MAX ) {
        LOG( LOG_CRIT, "too many open streams." );
        kzfree( sp );
        return -EBUSY;
    }

    file->private_data = &sp->fh;

    if ( mutex_lock_interruptible( &dev->file_lock ) )
        LOG( LOG_CRIT, "mutex_lock_interruptible failed.\n" );
    for ( i = 0; i < V4L2_STREAM_TYPE_MAX; i++ ) {
        if ( ( dev->stream_mask & ( 1 << i ) ) == 0 ) {
            dev->stream_mask |= ( 1 << i );
            sp->stream_id = i;
            break;
        }
    }
    mutex_unlock( &dev->file_lock );

    v4l2_fh_init( &sp->fh, &dev->video_dev );
    v4l2_fh_add( &sp->fh );

    return 0;
}

static int isp_v4l2_fh_release( struct file *file )
{
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );

    if ( sp ) {
        v4l2_fh_del( &sp->fh );
        v4l2_fh_exit( &sp->fh );
    }
    kzfree( sp );

    return 0;
}


/* ----------------------------------------------------------------
 * V4L2 file operations
 */
static int isp_v4l2_fop_open( struct file *file )
{
    int rc = 0;
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp;

    atomic_add( 1, &dev->opened );
    /* open file header */
    rc = isp_v4l2_fh_open( file );
    if ( rc < 0 ) {
        LOG( LOG_ERR, "Error, file handle open fail (rc=%d)", rc );
        goto fh_open_fail;
    }
    sp = fh_to_private( file->private_data );

    LOG( LOG_INFO, "isp_v4l2: %s: called for sid:%d.", __func__, sp->stream_id );
    /* init stream */
    isp_v4l2_stream_init( &dev->pstreams[sp->stream_id], sp->stream_id, dev->ctx_id );
    if ( sp->stream_id == 0 ) {
        // stream_id 0 is a full resolution
        dev->stream_id_index[V4L2_STREAM_TYPE_FR] = sp->stream_id;
        acamera_api_dma_buff_queue_reset(dev->ctx_id, dma_fr);
    }
#if ISP_HAS_DS1
    else if ( sp->stream_id == V4L2_STREAM_TYPE_DS1 ) {
        dev->stream_id_index[V4L2_STREAM_TYPE_DS1] = sp->stream_id;
        acamera_api_dma_buff_queue_reset(dev->ctx_id, dma_ds1);
    }
#endif
#if ISP_HAS_DS2
    else if (sp->stream_id == V4L2_STREAM_TYPE_DS2) {
        dev->stream_id_index[V4L2_STREAM_TYPE_DS2] = sp->stream_id;
    }
#endif

    /* init vb2 queue */

    rc = isp_vb2_queue_init( &sp->vb2_q, &dev->mlock, dev->pstreams[sp->stream_id], dev->v4l2_dev->dev );
    if ( rc < 0 ) {
        LOG( LOG_ERR, "Error, vb2 queue init fail (rc=%d)", rc );
        goto vb2_q_fail;
    }
    if (sp->stream_id == V4L2_STREAM_TYPE_FR ||
        sp->stream_id == V4L2_STREAM_TYPE_DS1) {

        sp->vb2_q.dev = g_isp_v4l2_devs[dev->ctx_id]->pdev;
    }
#if ISP_HAS_DS2
    else if (sp->stream_id == V4L2_STREAM_TYPE_DS2) {
        sp->vb2_q.dev = g_isp_v4l2_devs[dev->ctx_id]->pdev;
    }
#endif

    g_isp_v4l2_dev = g_isp_v4l2_devs[dev->ctx_id];
    fw_intf_isp_set_current_ctx_id(dev->ctx_id);

	dev->pstreams[sp->stream_id]->vb2_q = &sp->vb2_q;
    /* init fh_ptr */
    if ( mutex_lock_interruptible( &dev->notify_lock ) )
        LOG( LOG_CRIT, "mutex_lock_interruptible failed.\n" );
    dev->fh_ptr[sp->stream_id] = &( sp->fh );
    mutex_unlock( &dev->notify_lock );

    return rc;

vb2_q_fail:
    isp_v4l2_stream_deinit( dev->pstreams[sp->stream_id], atomic_read(&dev->stream_on_cnt) );

    //too_many_stream:
    isp_v4l2_fh_release( file );

fh_open_fail:
    atomic_sub( 1, &dev->opened );
    return rc;
}

static int isp_v4l2_fop_close( struct file *file )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
    int open_counter;

    LOG( LOG_INFO, "isp_v4l2: %s: called for sid:%d.", __func__, sp->stream_id );

    /* deinit fh_ptr */
    if ( mutex_lock_interruptible( &dev->notify_lock ) )
        LOG( LOG_CRIT, "mutex_lock_interruptible failed.\n" );
    dev->fh_ptr[sp->stream_id] = NULL;
    mutex_unlock( &dev->notify_lock );

    /* deinit stream */
    if ( pstream ) {
        if ( pstream->stream_type < V4L2_STREAM_TYPE_MAX )
            dev->stream_id_index[pstream->stream_type] = -1;
        if (pstream->stream_started)
        {
            isp_v4l2_stream_deinit( pstream, atomic_read(&dev->stream_on_cnt) );
            atomic_sub_return( 1, &dev->stream_on_cnt );
            dev->pstreams[sp->stream_id] = NULL;
        }
        if ( dev->pstreams[sp->stream_id] ) {
            kzfree( dev->pstreams[sp->stream_id] );
            dev->pstreams[sp->stream_id] = NULL;
        }
    }

    /* release vb2 queue */
    if ( sp->vb2_q.lock )
        mutex_lock( sp->vb2_q.lock );

    isp_vb2_queue_release( &sp->vb2_q );

    if ( sp->vb2_q.lock )
        mutex_unlock( sp->vb2_q.lock );

    if ( mutex_lock_interruptible( &dev->file_lock ) )
        LOG( LOG_CRIT, "mutex_lock_interruptible failed.\n" );

    dev->stream_mask &= ~( 1 << sp->stream_id );
    mutex_unlock( &dev->file_lock );
    open_counter = atomic_sub_return( 1, &dev->opened );

    /* release file handle */
    isp_v4l2_fh_release( file );

    return 0;
}

static ssize_t isp_v4l2_fop_read( struct file *filep,
                                  char __user *buf, size_t count, loff_t *ppos )
{
    struct isp_v4l2_fh *sp = fh_to_private( filep->private_data );
    int rc = 0;

    rc = vb2_read( &sp->vb2_q, buf, count, ppos, filep->f_flags & O_NONBLOCK );

    return rc;
}

static unsigned int isp_v4l2_fop_poll( struct file *filep,
                                       struct poll_table_struct *wait )
{
    struct isp_v4l2_fh *sp = fh_to_private( filep->private_data );
    int rc = 0;

    if ( sp->vb2_q.lock && mutex_lock_interruptible( sp->vb2_q.lock ) )
        return POLLERR;

    rc = vb2_poll( &sp->vb2_q, filep, wait );

    if ( sp->vb2_q.lock )
        mutex_unlock( sp->vb2_q.lock );

    return rc;
}

static int isp_v4l2_fop_mmap( struct file *file, struct vm_area_struct *vma )
{
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    int rc = 0;

    rc = vb2_mmap( &sp->vb2_q, vma );

    return rc;
}

static const struct v4l2_file_operations isp_v4l2_fops = {
    .owner = THIS_MODULE,
    .open = isp_v4l2_fop_open,
    .release = isp_v4l2_fop_close,
    .read = isp_v4l2_fop_read,
    .poll = isp_v4l2_fop_poll,
    .unlocked_ioctl = video_ioctl2,
    .mmap = isp_v4l2_fop_mmap,
};


/* ----------------------------------------------------------------
 * V4L2 ioctl operations
 */
static int isp_v4l2_querycap( struct file *file, void *priv, struct v4l2_capability *cap )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );

    LOG( LOG_DEBUG, "dev: %p, file: %p, priv: %p.\n", dev, file, priv );

    strcpy( cap->driver, "ARM-camera-isp" );
    strcpy( cap->card, "juno R2" );
    snprintf( cap->bus_info, sizeof( cap->bus_info ), "platform:%s", dev->v4l2_dev->name );

    /* V4L2_CAP_VIDEO_CAPTURE_MPLANE */

    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

    return 0;
}


/* format related, will be moved to isp-v4l2-stream.c */
static int isp_v4l2_g_fmt_vid_cap( struct file *file, void *priv, struct v4l2_format *f )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

    LOG( LOG_DEBUG, "isp_v4l2_g_fmt_vid_cap sid:%d", sp->stream_id );

    isp_v4l2_stream_get_format( pstream, f );

    LOG( LOG_DEBUG, "v4l2_format: type: %u, w: %u, h: %u, pixelformat: 0x%x, field: %u, colorspace: %u, sizeimage: %u, bytesperline: %u, flags: %u.\n",
         f->type,
         f->fmt.pix.width,
         f->fmt.pix.height,
         f->fmt.pix.pixelformat,
         f->fmt.pix.field,
         f->fmt.pix.colorspace,
         f->fmt.pix.sizeimage,
         f->fmt.pix.bytesperline,
         f->fmt.pix.flags );

    return 0;
}

static int isp_v4l2_enum_fmt_vid_cap( struct file *file, void *priv, struct v4l2_fmtdesc *f )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

    return isp_v4l2_stream_enum_format( pstream, f );
}

static int isp_v4l2_try_fmt_vid_cap( struct file *file, void *priv, struct v4l2_format *f )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
    LOG( LOG_CRIT, "isp_v4l2_try_fmt_vid_cap" );
    return isp_v4l2_stream_try_format( pstream, f );
}

static int isp_v4l2_s_fmt_vid_cap( struct file *file, void *priv, struct v4l2_format *f )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
    struct vb2_queue *q = &sp->vb2_q;
    int rc = 0;
    LOG( LOG_CRIT, "isp_v4l2_s_fmt_vid_cap sid:%d", sp->stream_id );
    if ( vb2_is_busy( q ) )
        return -EBUSY;

    rc = isp_v4l2_stream_set_format( pstream, f );
    if ( rc < 0 ) {
        LOG( LOG_ERR, "set format failed." );
        return rc;
    }

    /* update stream pointer index */
    dev->stream_id_index[pstream->stream_type] = pstream->stream_id;

    return 0;
}

static int isp_v4l2_enum_framesizes( struct file *file, void *priv, struct v4l2_frmsizeenum *fsize )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

    return isp_v4l2_stream_enum_framesizes( pstream, fsize );
}


/* Per-stream control operations */
static inline bool isp_v4l2_is_q_busy( struct vb2_queue *queue, struct file *file )
{
    return queue->owner && queue->owner != file->private_data;
}

static int isp_v4l2_streamon( struct file *file, void *priv, enum v4l2_buf_type i )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( priv );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
    int rc = 0;

    if ( isp_v4l2_is_q_busy( &sp->vb2_q, file ) )
        return -EBUSY;

    rc = vb2_streamon( &sp->vb2_q, i );
    if ( rc != 0 ) {
        LOG( LOG_ERR, "fail to vb2_streamon. (rc=%d)", rc );
        return rc;
    }

    /* Start hardware */
    rc = isp_v4l2_stream_on( pstream );
    if ( rc != 0 ) {
        LOG( LOG_ERR, "fail to isp_stream_on. (stream_id = %d, rc=%d)", sp->stream_id, rc );
        isp_v4l2_stream_off( pstream, atomic_read(&dev->stream_on_cnt) );
        return rc;
    }

    atomic_add( 1, &dev->stream_on_cnt );

    return rc;
}

static int isp_v4l2_streamoff( struct file *file, void *priv, enum v4l2_buf_type i )
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( priv );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];
    int rc = 0;

    if ( isp_v4l2_is_q_busy( &sp->vb2_q, file ) )
        return -EBUSY;

    /* Stop hardware */
    isp_v4l2_stream_off( pstream, atomic_read(&dev->stream_on_cnt) );

    /* vb streamoff */
    rc = vb2_streamoff( &sp->vb2_q, i );

    atomic_sub_return( 1, &dev->stream_on_cnt );

    return rc;
}


/* input control */
static int isp_v4l2_enum_input( struct file *file, void *fh, struct v4l2_input *input )
{
    /* currently only support general camera input */
    if ( input->index > 0 )
        return -EINVAL;

    strlcpy( input->name, "camera", sizeof( input->name ) );
    input->type = V4L2_INPUT_TYPE_CAMERA;

    return 0;
}

static int isp_v4l2_g_input( struct file *file, void *fh, unsigned int *input )
{
    /* currently only support general camera input */
    *input = 0;

    return 0;
}

static int isp_v4l2_s_input( struct file *file, void *fh, unsigned int input )
{
    /* currently only support general camera input */
    return input == 0 ? 0 : -EINVAL;
}


/* vb2 customization for multi-stream support */
static int isp_v4l2_reqbufs( struct file *file, void *priv,
                             struct v4l2_requestbuffers *p )
{
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    int rc = 0;

    LOG( LOG_DEBUG, "(stream_id = %d, ownermatch=%d)", sp->stream_id, isp_v4l2_is_q_busy( &sp->vb2_q, file ) );
    if ( isp_v4l2_is_q_busy( &sp->vb2_q, file ) )
        return -EBUSY;

    rc = vb2_reqbufs( &sp->vb2_q, p );
    if ( rc == 0 )
        sp->vb2_q.owner = p->count ? file->private_data : NULL;

#if ISP_HAS_DS2
    if (sp->stream_id == V4L2_STREAM_TYPE_DS2) {
        LOG( LOG_INFO, "request buffer count = %d\n", p->count);
        am_sc_set_buf_num(p->count);
        am_sc_system_init();
    }
#endif

    LOG( LOG_DEBUG, "sid:%d reqbuf p->type:%d p->memory %d p->count %d rc %d", sp->stream_id, p->type, p->memory, p->count, rc );
    return rc;
}

static int isp_v4l2_querybuf( struct file *file, void *priv, struct v4l2_buffer *p )
{
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    int rc = 0;

    rc = vb2_querybuf( &sp->vb2_q, p );

    LOG( LOG_DEBUG, "sid:%d querybuf p->type:%d p->index:%d , rc %d",
            sp->stream_id, p->type, p->index, rc );

    return rc;
}

static int isp_v4l2_qbuf( struct file *file, void *priv, struct v4l2_buffer *p )
{
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    int rc = 0;

    LOG( LOG_DEBUG, "(stream_id = %d, ownermatch=%d)", sp->stream_id, isp_v4l2_is_q_busy( &sp->vb2_q, file ) );
    if ( isp_v4l2_is_q_busy( &sp->vb2_q, file ) ) {
        return -EBUSY;
    }

    rc = vb2_qbuf( &sp->vb2_q, p );
    LOG( LOG_DEBUG, "sid:%d qbuf p->type:%d p->index:%d, rc %d", sp->stream_id, p->type, p->index, rc );
    return rc;
}

static int isp_v4l2_dqbuf( struct file *file, void *priv, struct v4l2_buffer *p )
{
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    int rc = 0;
    LOG( LOG_DEBUG, "(stream_id = %d, ownermatch=%d)", sp->stream_id, isp_v4l2_is_q_busy( &sp->vb2_q, file ) );
    if ( isp_v4l2_is_q_busy( &sp->vb2_q, file ) )
        return -EBUSY;

    rc = vb2_dqbuf( &sp->vb2_q, p, file->f_flags & O_NONBLOCK );
    LOG( LOG_DEBUG, "sid:%d qbuf p->type:%d p->index:%d, rc %d", sp->stream_id, p->type, p->index, rc );
    return rc;
}

static int isp_v4l2_cropcap(struct file *file, void *fh,
                                    struct v4l2_cropcap *cap)
{
    int ret = -1;
    isp_v4l2_dev_t *dev = video_drvdata(file);
    struct isp_v4l2_fh *sp = fh_to_private(fh);
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

    if (pstream->stream_type == V4L2_STREAM_TYPE_FR ||
                pstream->stream_type == V4L2_STREAM_TYPE_DS1)
        ret = isp_v4l2_get_cropcap(pstream, cap);
    else
        LOG(LOG_ERR, "Error support this stream type: %d", pstream->stream_type);

    return ret;
}

static int isp_v4l2_g_crop(struct file *file, void *fh,
                                    struct v4l2_crop *crop)
{
    int ret = -1;
    isp_v4l2_dev_t *dev = video_drvdata(file);
    struct isp_v4l2_fh *sp = fh_to_private(fh);
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

    if (pstream->stream_type == V4L2_STREAM_TYPE_FR ||
                pstream->stream_type == V4L2_STREAM_TYPE_DS1)
        ret = isp_v4l2_get_crop(pstream, crop);
    else
        LOG(LOG_ERR, "Error support this stream type: %d", pstream->stream_type);

    return ret;
}

static int isp_v4l2_s_crop(struct file *file, void *fh,
                                const struct v4l2_crop *crop)
{
    int ret = -1;
    isp_v4l2_dev_t *dev = video_drvdata(file);
    struct isp_v4l2_fh *sp = fh_to_private(fh);
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

    if (pstream->stream_type == V4L2_STREAM_TYPE_FR ||
                pstream->stream_type == V4L2_STREAM_TYPE_DS1)
        ret = isp_v4l2_set_crop(pstream, crop);
    else
        LOG(LOG_ERR, "Error support this stream type: %d", pstream->stream_type);

    return ret;
}

static int isp_v4l2_expbuf(struct file *file, void *priv, struct v4l2_exportbuffer *ex_buf)
{
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );

    if (sp == NULL || ex_buf == NULL) {
        LOG(LOG_ERR, "Error invalid input param");
        return -EINVAL;
    }

    return vb2_expbuf(&sp->vb2_q, ex_buf);
}

int isp_v4l2_enum_frameintervals(struct file *file, void *fh,
                                        struct v4l2_frmivalenum *fival)
{
    isp_v4l2_dev_t *dev = video_drvdata( file );
    struct isp_v4l2_fh *sp = fh_to_private( file->private_data );
    isp_v4l2_stream_t *pstream = dev->pstreams[sp->stream_id];

    return isp_v4l2_stream_enum_frameintervals( pstream, fival);
}


static const struct v4l2_ioctl_ops isp_v4l2_ioctl_ops = {
    .vidioc_querycap = isp_v4l2_querycap,

    /* Per-stream config operations */
    .vidioc_g_fmt_vid_cap_mplane = isp_v4l2_g_fmt_vid_cap,
    .vidioc_enum_fmt_vid_cap_mplane = isp_v4l2_enum_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap_mplane = isp_v4l2_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap_mplane = isp_v4l2_s_fmt_vid_cap,

    .vidioc_g_fmt_vid_cap = isp_v4l2_g_fmt_vid_cap,
    .vidioc_enum_fmt_vid_cap = isp_v4l2_enum_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap = isp_v4l2_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap = isp_v4l2_s_fmt_vid_cap,
    .vidioc_enum_framesizes = isp_v4l2_enum_framesizes,

    /* Per-stream control operations */
    .vidioc_streamon = isp_v4l2_streamon,
    .vidioc_streamoff = isp_v4l2_streamoff,

    /* input control */
    .vidioc_enum_input = isp_v4l2_enum_input,
    .vidioc_g_input = isp_v4l2_g_input,
    .vidioc_s_input = isp_v4l2_s_input,

    /* vb2 customization for multi-stream support */
    .vidioc_reqbufs = isp_v4l2_reqbufs,

    .vidioc_querybuf = isp_v4l2_querybuf,
    .vidioc_qbuf = isp_v4l2_qbuf,
    .vidioc_dqbuf = isp_v4l2_dqbuf,

    /* v4l2 event ioctls */
    .vidioc_log_status = v4l2_ctrl_log_status,
    .vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
    .vidioc_unsubscribe_event = v4l2_event_unsubscribe,

    /* crop ioctls */
    .vidioc_cropcap = isp_v4l2_cropcap,
    .vidioc_g_crop = isp_v4l2_g_crop,
    .vidioc_s_crop = isp_v4l2_s_crop,

    .vidioc_expbuf = isp_v4l2_expbuf,
 #ifdef CONFIG_ANDROID_OS
	.vidioc_enum_frameintervals = isp_v4l2_enum_frameintervals,
 #endif
};

static int isp_cma_alloc(struct platform_device *pdev, unsigned long size)
{
    struct page *cma_pages = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
    cma_pages = dma_alloc_from_contiguous(
			&(pdev->dev), size >> PAGE_SHIFT, 0, false);
#else
    cma_pages = dma_alloc_from_contiguous(
			&(pdev->dev), size >> PAGE_SHIFT, 0);
#endif
    if (cma_pages) {
        isp_paddr = page_to_phys(cma_pages);
    } else {
        LOG(LOG_ERR, "Failed alloc cma pages.\n");
        return -1;
    }
    isp_kaddr = (void *)cma_pages;

    LOG( LOG_INFO, "isp_cma_mem : %p, paddr:0x%x\n", isp_kaddr, isp_paddr);

    return 0;
}

static void isp_cma_free(struct platform_device *pdev, void *kaddr, unsigned long size)
{
    struct page *cma_pages = NULL;
    bool rc = false;

    if (pdev == NULL || kaddr == NULL) {
        LOG(LOG_ERR, "Error input param\n");
        return;
    }

    cma_pages = kaddr;

    rc = dma_release_from_contiguous(&(pdev->dev), cma_pages, size >> PAGE_SHIFT);
    if (rc == false) {
        LOG(LOG_ERR, "Failed to release cma buffer\n");
        return;
    }
}

static int isp_v4l2_init_dev( uint32_t ctx_id, struct v4l2_device *v4l2_dev )
{
    isp_v4l2_dev_t *dev;
    struct video_device *vfd;
    v4l2_std_id tvnorms_cap = 0;
    int rc = 0;
    int i;

    if ( ctx_id >= FIRMWARE_CONTEXT_NUMBER ) {
        LOG( LOG_ERR, "Invalid ctx numbr: %d, max: %d.", ctx_id, FIRMWARE_CONTEXT_NUMBER - 1 );
        return -EINVAL;
    }

    /* allocate main isp_v4l2 state structure */
    dev = kzalloc( sizeof( *dev ), GFP_KERNEL );
    if ( !dev )
        return -ENOMEM;

    memset( dev, 0x0, sizeof( isp_v4l2_dev_t ) );

    /* register v4l2_device */

    dev->v4l2_dev = v4l2_dev;
    dev->ctx_id = ctx_id;

    /* init v4l2 controls */
    dev->isp_v4l2_ctrl.v4l2_dev = dev->v4l2_dev;
    dev->isp_v4l2_ctrl.video_dev = &dev->video_dev;
    rc = isp_v4l2_ctrl_init( ctx_id, &dev->isp_v4l2_ctrl );
    if ( rc )
        goto free_dev;

    /* initialize locks */
    mutex_init( &dev->mlock );
    mutex_init( &dev->notify_lock );
	mutex_init( &dev->file_lock);

    /* initialize stream id table */
    for ( i = 0; i < V4L2_STREAM_TYPE_MAX; i++ ) {
        dev->stream_id_index[i] = -1;
    }

    /* initialize open counter */
    atomic_set( &dev->stream_on_cnt, 0 );
    atomic_set( &dev->opened, 0 );

    /* finally start creating the device nodes */
    vfd = &dev->video_dev;
    strlcpy( vfd->name, "isp_v4l2-vid-cap", sizeof( vfd->name ) );
    vfd->fops = &isp_v4l2_fops;
    vfd->ioctl_ops = &isp_v4l2_ioctl_ops;
    vfd->release = video_device_release_empty;
    vfd->v4l2_dev = dev->v4l2_dev;
    vfd->queue = NULL; // queue will be customized in file handle
    vfd->tvnorms = tvnorms_cap;
    vfd->vfl_type = VFL_TYPE_GRABBER;

    /*
     * Provide a mutex to v4l2 core. It will be used to protect
     * all fops and v4l2 ioctls.
     */
    vfd->lock = &dev->mlock;
    video_set_drvdata( vfd, dev );

    /* videoX start number, -1 is autodetect */
    rc = video_register_device( vfd, VFL_TYPE_GRABBER, VIDEO_NODE_NUM );
    if ( rc < 0 )
        goto unreg_dev;

    LOG( LOG_INFO, "V4L2 capture device registered as %s.",
         video_device_node_name( vfd ) );

    /* store dev pointer to destroy later and find stream */
    g_isp_v4l2_devs[ctx_id] = dev;

    return rc;

unreg_dev:
    video_unregister_device( &dev->video_dev );
    isp_v4l2_ctrl_deinit( &dev->isp_v4l2_ctrl );

free_dev:
    kfree( dev );

    return rc;
}

static void isp_v4l2_destroy_dev( int ctx_id )
{
    if ( g_isp_v4l2_devs[ctx_id] ) {
        LOG( LOG_INFO, "unregistering %s.", video_device_node_name( &g_isp_v4l2_devs[ctx_id]->video_dev ) );

        /* unregister video device */
        video_unregister_device( &g_isp_v4l2_devs[ctx_id]->video_dev );

        isp_v4l2_ctrl_deinit( &g_isp_v4l2_devs[ctx_id]->isp_v4l2_ctrl );

        kfree( g_isp_v4l2_devs[ctx_id] );
        g_isp_v4l2_devs[ctx_id] = NULL;
    } else {
        LOG( LOG_INFO, "isp_v4l2_dev ctx_id: %d is NULL, skip.", ctx_id );
    }
}

/* ----------------------------------------------------------------
 * V4L2 external interface for probe
 */
int isp_v4l2_create_instance( struct v4l2_device *v4l2_dev, struct platform_device *pdev, uint32_t hw_isp_addr )
{
	uint32_t ctx_id;
	uint32_t ret_value;
	unsigned int temper_buf_size;
	int rc = 0;
	
	if ( v4l2_dev == NULL ) {
		LOG( LOG_ERR, "Invalid parameter" );
		return -EINVAL;
	}
	
	rc = of_property_read_u32(pdev->dev.of_node, "temper-buf-size",
							   &temper_buf_size);
	LOG(LOG_INFO, "%s:temper buffer size %d, rtn %d\n", __func__, temper_buf_size, rc);

	if (rc != 0) {
		   LOG(LOG_ERR, "failed to get temper-buf-size from dts, use default value\n");
		   temper_buf_size = DEFAULT_TEMPER_BUFFER_SIZE;
	}

	rc = isp_cma_alloc(pdev, temper_buf_size * SIZE_1M);
	if (rc < 0)
		return rc;

	rc = of_property_read_u32(pdev->dev.of_node, "temper-line-offset", &temper_line_offset);
	LOG(LOG_INFO, "%s:temper line offset %d, rtn %d\n", __func__, temper_line_offset, rc);
	if (rc != 0) {
		   LOG(LOG_ERR, "failed to get temper_line_offset from dts, use default value\n");
		   temper_line_offset = DEFAULT_TEMPER_LINE_OFFSET;
	}

	/* initialize isp */
	rc = fw_intf_isp_init(hw_isp_addr);
	if ( rc < 0 )
		goto free_cma;

    /* initialize stream related resources to prepare for streaming.
     * It should be called after sensor initialized.
     */
    for ( ctx_id = 0; ctx_id < FIRMWARE_CONTEXT_NUMBER; ctx_id++ ) {
        rc = isp_v4l2_stream_init_static_resources( pdev, ctx_id );
        if ( rc < 0 )
            goto deinit_fw_intf;
    }

	/* check sensor devices */
	for ( ctx_id = 0; ctx_id < FIRMWARE_CONTEXT_NUMBER; ctx_id++ ) {
		rc = acamera_command(ctx_id, TSENSOR, SENSOR_HWID, 0, COMMAND_GET, &ret_value);
		if ( rc ) {
			LOG( LOG_CRIT, "isp_v4l2_init ctx_id: %d failed.", ctx_id );
			rc = 0;
			goto deinit_fw_intf;
		}
	}
		
	/* initialize v4l2 layer devices */
	for ( ctx_id = 0; ctx_id < FIRMWARE_CONTEXT_NUMBER; ctx_id++ ) {
		rc = isp_v4l2_init_dev( ctx_id, v4l2_dev );
		if ( rc ) {
			LOG( LOG_ERR, "isp_v4l2_init ctx_id: %d failed.", ctx_id );
			goto unreg_dev;
		}
		g_isp_v4l2_devs[ctx_id]->pdev = &pdev->dev;
	}
		
	g_isp_v4l2_dev = g_isp_v4l2_devs[0];
    g_isp_v4l2_dev->temper_buf_size = temper_buf_size;
	
	return 0;
	
unreg_dev:
	for ( ctx_id = 0; ctx_id < FIRMWARE_CONTEXT_NUMBER; ctx_id++ ) {
		isp_v4l2_destroy_dev( ctx_id );
	}
	
deinit_fw_intf:
	fw_intf_isp_deinit();

free_cma:
	isp_cma_free(pdev, isp_kaddr, temper_buf_size * SIZE_1M);
	
	return rc;
}

void isp_v4l2_destroy_instance( struct platform_device *pdev )
{
    LOG( LOG_INFO, "%s: Enter.\n", __func__ );

	if ( g_isp_v4l2_devs[0] ) {
        int ctx_id;

        /* deinitialize firmware & stream resources */
        fw_intf_isp_deinit();
        isp_v4l2_stream_deinit_static_resources(pdev);

        isp_cma_free(pdev, isp_kaddr,
             (g_isp_v4l2_dev->temper_buf_size) * SIZE_1M);

        for ( ctx_id = 0; ctx_id < FIRMWARE_CONTEXT_NUMBER; ctx_id++ ) {
            isp_v4l2_destroy_dev( ctx_id );
        }
    }
}


/* ----------------------------------------------------------------
 * stream finder utility function
 */
int isp_v4l2_find_stream( isp_v4l2_stream_t **ppstream,
                          int ctx_number, isp_v4l2_stream_type_t stream_type )
{
    int stream_id;

    *ppstream = NULL;

    if ( g_isp_v4l2_dev == NULL ) {
        return -EBUSY;
    }

    if ( stream_type >= V4L2_STREAM_TYPE_MAX || stream_type < 0 ) {
        return -EINVAL;
    }

    stream_id = g_isp_v4l2_devs[ctx_number]->stream_id_index[stream_type];
    if ( stream_id < 0 || stream_id >= V4L2_STREAM_TYPE_MAX || g_isp_v4l2_dev->pstreams[stream_id] == NULL ) {
        return -EBUSY;
    }

    *ppstream = g_isp_v4l2_devs[ctx_number]->pstreams[stream_id];

    return 0;
}
						  
isp_v4l2_dev_t *isp_v4l2_get_dev( uint32_t ctx_number )
{
    return g_isp_v4l2_devs[ctx_number];
}

/* ----------------------------------------------------------------
 * event notifier utility function
 */
int isp_v4l2_notify_event( int ctx_id, int stream_id, uint32_t event_type )
{
    struct v4l2_event event;

    if ( g_isp_v4l2_devs[ctx_id] == NULL ) {
        return -EBUSY;
    }

    if ( mutex_lock_interruptible( &g_isp_v4l2_devs[ctx_id]->notify_lock ) )
        LOG( LOG_CRIT, "mutex_lock_interruptible failed.\n" );
    if ( g_isp_v4l2_devs[ctx_id]->fh_ptr[stream_id] == NULL ) {
        LOG( LOG_ERR, "Error, no fh_ptr exists for stream id %d (event_type = %d)", stream_id, event_type );
        mutex_unlock( &g_isp_v4l2_devs[ctx_id]->notify_lock );
        return -EINVAL;
    }

    memset( &event, 0, sizeof( event ) );
    event.type = event_type;

    v4l2_event_queue_fh( g_isp_v4l2_devs[ctx_id]->fh_ptr[stream_id], &event );
    mutex_unlock( &g_isp_v4l2_devs[ctx_id]->notify_lock );

    return 0;
}
