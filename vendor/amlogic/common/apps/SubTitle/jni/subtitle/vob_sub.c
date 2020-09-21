/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: c file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include "log_print.h"

#include "vob_sub.h"
#include "sub_vob_sub.h"

#include <cutils/properties.h>
#include <android/log.h>

#define MAX_EXTNAME_LEN 8

#define  LOG_TAG    "sub_jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
static int dbg_level = 0;

//#define //AF_VOBSUB_LOG_ERR(level, args...) AVOS_LOG(level, DBGHLP_LOG_AFRAME, "<SubtitleVOBSub> ", ##args)
//#define AF_VOBSUB_LOG_INF(level, args...) AVOS_LOG(level, DBGHLP_LOG_APPTASK, "<SubtitleVOBSub> ", ##args)

subtitlevobsub_t *vobsubdata = NULL;

/* access Handle/Array prop by these Macros*/
/*Macros defines end*/

//static void subtitlevobsub_release(control_t* cntl)
//{
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t *)(cntl->private_data);
//    if(subtitlevobsub)
//    {
//        free(subtitlevobsub);
//    }
//
//}

//static subtitlevobsub_t* subtitlevobsub_init(control_t* cntl)
//{
//    int i=0;
//    cond_item_t* temp_ptr=NULL;
//    subtitlevobsub_t* subtitlevobsub= calloc(sizeof(subtitlevobsub_t),1);
//    if(subtitlevobsub)
//    {
//        subtitlevobsub->cntl=cntl;
//    }
//    return subtitlevobsub;
//}

/* Handle Prop' method functions, #define HANDLE_METHOD_FUN to enable below code, remove functions that never been used */
//#define HANDLE_METHOD_FUN
#ifdef HANDLE_METHOD_FUN
#endif              /*HANDLE_METHOD_FUN */

/* message send functions, #define MSG_SEND_FUN to enable below code, remove functions that never been used */
#define MSG_SEND_FUN
#ifdef MSG_SEND_FUN
//static int subtitlevobsub_send_msg_bplay_show_subtitle(control_t* cntl, handle_t handle, INT32U type, INT32U info)
//{
//    int ret=0;
//    message_t* pMsg;
//    cond_item_type_t param_type[2] = {CONST_INT,CONST_INT};
//    cond_item_t msgval[2];
//    msgval[0] = (cond_item_t)(type);
//    msgval[1] = (cond_item_t)(info);
//    AF_MSG_LOG(dbg_level,"SubtitleVOBSub=>AFMSG_BPLAY_SHOW_SUBTITLE(0x%x,0x%x)\n",msgval[0],msgval[1]);
//    pMsg = af_create_cntl_message(cntl, AFMSG_BPLAY_SHOW_SUBTITLE, handle, 2, param_type, msgval);
//    af_send_message(pMsg);
//    return ret;
//}
#endif              /*MSG_SEND_FUN */

/* Above code is created by AfControlWizard */

/* Add User Code here */
//#include "vobsub.h"
//#include "spudec.h"
#ifdef USE_UNRARLIB
#include "unrarlib.h"
#endif

#define MIN(a, b)    ((a)<(b)?(a):(b))
#define MAX(a, b)    ((a)>(b)?(a):(b))
#define UINT_MAX 0xFFFFFFFFFFFFFFFLL
int vobsub_id = -1;

//int identify=0;
/**********************************************************************
 * RAR stream handling
 * The RAR file must have the same basename as the file to open
 * See <URL:http://www.unrarlib.org/>
 **********************************************************************/
static rar_stream_t *rar_open(const char *const filename,
                              const char *const mode)
{
    rar_stream_t *stream;
#if 0
    /* unrarlib can only read */
    if (strcmp("r", mode) && strcmp("rb", mode))
    {
        //  errno = EINVAL;
        return NULL;
    }
#endif
    stream = malloc(sizeof(rar_stream_t));
    if (stream == NULL)
        return NULL;
    /* first try normal access */
#if 1
    stream->fd = open(filename, O_RDONLY);
    if (stream->fd >= 0)
    {
        stream->pos = 0;
        stream->size = lseek(stream->fd, 0, SEEK_END);
        lseek(stream->fd, 0, SEEK_SET);
    }
    else
    {
        free(stream);
        stream = NULL;
    }
#else
    stream->file = fopen(filename, mode);
    if (stream->file == NULL)
    {
        char *rar_filename;
        const char *p;
        int rc;
        /* Guess the RAR archive filename */
        rar_filename = NULL;
        p = strrchr(filename, '.');
        if (p)
        {
            ptrdiff_t l = p - filename;
            rar_filename = malloc(l + 5);
            if (rar_filename == NULL)
            {
                free(stream);
                return NULL;
            }
            strncpy(rar_filename, filename, l);
            strcpy(rar_filename + l, ".rar");
        }
        else
        {
            rar_filename = malloc(strlen(filename) + 5);
            if (rar_filename == NULL)
            {
                free(stream);
                return NULL;
            }
            strcpy(rar_filename, filename);
            strcat(rar_filename, ".rar");
        }
        /* get rid of the path if there is any */
        if ((p = strrchr(filename, '/')) == NULL)
        {
            p = filename;
        }
        else
        {
            p++;
        }
        rc = urarlib_get(&stream->data, &stream->size, (char *)p,
                         rar_filename, "");
        if (!rc)
        {
            /* There is no matching filename in the archive. However, sometimes
             * the files we are looking for have been given arbitrary names in the archive.
             * Let's look for a file with an exact match in the extension only. */
            int i, num_files, name_len;
            ArchiveList_struct *list, *lp;
            /* the cast in the next line is a hack to overcome a design flaw (IMHO) in unrarlib */
            num_files =
                urarlib_list(rar_filename,
                             (ArchiveList_struct *) & list);
            if (num_files > 0)
            {
                char *demanded_ext;
                demanded_ext = strrchr(p, '.');
                if (demanded_ext)
                {
                    int demanded_ext_len =
                        strlen(demanded_ext);
                    for (i = 0, lp = list; i < num_files;
                            i++, lp = lp->next)
                    {
                        name_len =
                            strlen(lp->item.Name);
                        if (name_len >= demanded_ext_len
                                && !strcasecmp(lp->item.
                                               Name +
                                               name_len -
                                               demanded_ext_len,
                                               demanded_ext))
                        {
                            if ((rc =
                                        urarlib_get
                                        (&stream->data,
                                         &stream->size,
                                         lp->item.Name,
                                         rar_filename,
                                         "")))
                            {
                                break;
                            }
                        }
                    }
                }
                urarlib_freelist(list);
            }
            if (!rc)
            {
                free(rar_filename);
                free(stream);
                return NULL;
            }
        }
        free(rar_filename);
        stream->pos = 0;
    }
#endif
    return stream;
}

static int rar_close(rar_stream_t *stream)
{
    if (stream)
    {
        if (stream->fd >= 0)
            close(stream->fd);
        free(stream);
    }
    return 0;
}

static int rar_eof(rar_stream_t *stream)
{
    return stream->pos >= stream->size;
}

static long rar_tell(rar_stream_t *stream)
{
    return stream->pos;
}

static int rar_seek(rar_stream_t *stream, long offset, int whence)
{
    switch (whence)
    {
        case SEEK_SET:
            if (offset < 0)
            {
                //        errno = EINVAL;
                return -1;
            }
            stream->pos = offset;
            break;
        case SEEK_CUR:
            if (offset < 0 && stream->pos < (unsigned long) - offset)
            {
                //        errno = EINVAL;
                return -1;
            }
            stream->pos += offset;
            break;
        case SEEK_END:
            if (offset < 0 && stream->size < (unsigned long) - offset)
            {
                //        errno = EINVAL;
                return -1;
            }
            stream->pos = stream->size + offset;
            break;
        default:
            //  errno = EINVAL;
            return -1;
    }
    if (stream->fd >= 0)
        lseek(stream->fd, stream->pos, SEEK_SET);
    return 0;
}

static int rar_getc(rar_stream_t *stream)
{
#if 1
    unsigned char ch;
    int ret_ch = EOF;
    if (stream->fd >= 0)
    {
        if (read(stream->fd, &ch, 1) > 0)
        {
            stream->pos++;
            ret_ch = ch;
        }
    }
    return ret_ch;
#else
    if (stream->file)
        return getc(stream->file);
    if (rar_eof(stream))
        return EOF;
    return stream->data[stream->pos++];
#endif
}

static size_t
rar_read(void *ptr, size_t size, size_t nmemb, rar_stream_t *stream)
{
    size_t res;
    int remain;
#if 1
    //when play agin maybe it can: stream->fd=0;
    if (stream->fd >= 0)
    {
        if (rar_eof(stream))
            return 0;
        res = size * nmemb;
        remain = stream->size - stream->pos;
        if (res > remain)
            res = remain / size * size;
        read(stream->fd, ptr, res);
        stream->pos += res;
        res /= size;
        return res;
    }
    else
        return 0;
#else
    if (stream->file)
        return fread(ptr, size, nmemb, stream->file);
    if (rar_eof(stream))
        return 0;
    res = size * nmemb;
    remain = stream->size - stream->pos;
    if (res > remain)
        res = remain / size * size;
    memcpy(ptr, stream->data + stream->pos, res);
    stream->pos += res;
    res /= size;
    return res;
#endif
}

/**********************************************************************/

static ssize_t getSubLine(char **lineptr, size_t *n, rar_stream_t *stream)
{
    size_t res = 0;
    int c;
    if (*lineptr == NULL)
    {
        *lineptr = malloc(4096);
        if (*lineptr)
            *n = 4096;
    }
    else if (*n == 0)
    {
        char *tmp = realloc(*lineptr, 4096);
        if (tmp)
        {
            *lineptr = tmp;
            *n = 4096;
        }
    }
    if (*lineptr == NULL || *n == 0)
        return -1;
    for (c = rar_getc(stream); c != EOF; c = rar_getc(stream))
    {
        if (res + 1 >= *n)
        {
            char *tmp = realloc(*lineptr, *n * 2);
            if (tmp == NULL)
                return -1;
            *lineptr = tmp;
            *n *= 2;
        }
        (*lineptr)[res++] = c;
        if (c == '\n')
        {
            (*lineptr)[res] = 0;
            return res;
        }
    }
    if (res == 0)
        return -1;
    (*lineptr)[res] = 0;
    return res;
}

/**********************************************************************
 * MPEG parsing
 **********************************************************************/

static mpeg_t *mpeg_open(const char *filename)
{
    mpeg_t *res = calloc(sizeof(mpeg_t), 1);
    int err = res == NULL;
    if (!err)
    {
        res->pts = 0;
        res->aid = -1;
        res->packet = NULL;
        res->packet_size = 0;
        //res->packet_reserve = 0;
        res->stream = rar_open(filename, "rb");
        err = res->stream == NULL;
        if (err)
            LOGI("fopen Vobsub file failed");
        if (err)
            free(res);
    }
    return err ? NULL : res;
}

static void mpeg_free(mpeg_t *mpeg)
{
    if (mpeg->packet)
        free(mpeg->packet);
    if (mpeg->stream)
        rar_close(mpeg->stream);
    free(mpeg);
}

static int mpeg_eof(mpeg_t *mpeg)
{
    return rar_eof(mpeg->stream);
}

static off_t mpeg_tell(mpeg_t *mpeg)
{
    return rar_tell(mpeg->stream);
}

static int mpeg_run(mpeg_t *mpeg, char read_flag)
{
    unsigned int len, idx, version;
    int c;
    /* Goto start of a packet, it starts with 0x000001?? */
    const unsigned char wanted[] = { 0, 0, 1 };
    unsigned char buf[5];
    mpeg->aid = -1;
    mpeg->packet_size = 0;
    if (rar_read(buf, 4, 1, mpeg->stream) != 1)
        return -1;
    while (memcmp(buf, wanted, sizeof(wanted)) != 0)
    {
        c = rar_getc(mpeg->stream);
        if (c < 0)
            return -1;
        memmove(buf, buf + 1, 3);
        buf[3] = c;
    }
    switch (buf[3])
    {
        case 0xb9:      /* System End Code */
            break;
        case 0xba:      /* Packet start code */
            c = rar_getc(mpeg->stream);
            if (c < 0)
                return -1;
            if ((c & 0xc0) == 0x40)
                version = 4;
            else if ((c & 0xf0) == 0x20)
                version = 2;
            else
            {
                //                //AF_VOBSUB_LOG_ERR(dbg_level, "VobSub: Unsupported MPEG version: 0x%02x\n", c);
                //return -1;
                //if c not 0x44. can parse , fix OTT-3544
                if (rar_seek(mpeg->stream, 9, SEEK_CUR))
                    return -1;
            }
            /*if (version == 4)
            {
                if (rar_seek(mpeg->stream, 9, SEEK_CUR))
                    return -1;
            }
            else if (version == 2)
            {
                if (rar_seek(mpeg->stream, 7, SEEK_CUR))
                    return -1;
            }
            else
                abort();*/
            break;
        case 0xbd:      /* packet */
            if (rar_read(buf, 2, 1, mpeg->stream) != 1)
                return -1;
            len = buf[0] << 8 | buf[1];
            idx = mpeg_tell(mpeg);
            c = rar_getc(mpeg->stream);
            if (c < 0)
                return -1;
            if ((c & 0xC0) == 0x40)     /* skip STD scale & size */
            {
                if (rar_getc(mpeg->stream) < 0)
                    return -1;
                c = rar_getc(mpeg->stream);
                if (c < 0)
                    return -1;
            }
            if ((c & 0xf0) == 0x20)     /* System-1 stream timestamp */
            {
                /* Do we need this? */
                abort();
            }
            else if ((c & 0xf0) == 0x30)
            {
                /* Do we need this? */
                abort();
            }
            else if ((c & 0xc0) == 0x80)        /* System-2 (.VOB) stream */
            {
                unsigned int pts_flags, hdrlen, dataidx;
                c = rar_getc(mpeg->stream);
                if (c < 0)
                    return -1;
                pts_flags = c;
                c = rar_getc(mpeg->stream);
                if (c < 0)
                    return -1;
                hdrlen = c;
                dataidx = mpeg_tell(mpeg) + hdrlen;
                if (dataidx > idx + len)
                {
                    //                    ////AF_VOBSUB_LOG_ERR(dbg_level, "Invalid header length: %d (total length: %d, idx: %d, dataidx: %d)\n",
                    //                    hdrlen, len, idx, dataidx);
                    return -1;
                }
                if ((pts_flags & 0xc0) == 0x80)
                {
                    if (rar_read(buf, 5, 1, mpeg->stream) != 1)
                        return -1;
                    if (!
                            (((buf[0] & 0xf0) == 0x20) && (buf[0] & 1)
                             && (buf[2] & 1) && (buf[4] & 1)))
                    {
                        ////AF_VOBSUB_LOG_ERR(dbg_level, "vobsub PTS error: 0x%02x %02x%02x %02x%02x \n",
                        //                            buf[0], buf[1], buf[2], buf[3], buf[4]);
                        mpeg->pts = 0;
                    }
                    else
                        mpeg->pts =
                            ((buf[0] & 0x0e) << 29 | buf[1] <<
                             22 | (buf[2] & 0xfe) << 14 | buf[3]
                             << 7 | (buf[4] >> 1));
                }
                else        /* if ((pts_flags & 0xc0) == 0xc0) */
                {
                    /* what's this? */
                    /* abort(); */
                }
                rar_seek(mpeg->stream, dataidx, SEEK_SET);
                mpeg->aid = rar_getc(mpeg->stream);
                if (mpeg->aid < 0)
                {
                    ////AF_VOBSUB_LOG_ERR(dbg_level, "Bogus aid %d\n", mpeg->aid);
                    return -1;
                }
                mpeg->packet_size =
                    len - ((unsigned int)mpeg_tell(mpeg) - idx);
                if (read_flag)
                {
                    //if (mpeg->packet_reserve < mpeg->packet_size) {
                    if (mpeg->packet)
                        free(mpeg->packet);
                    mpeg->packet = malloc(mpeg->packet_size);
                    //if (mpeg->packet)
                    //    mpeg->packet_reserve = mpeg->packet_size;
                    //}
                    if (mpeg->packet == NULL)
                    {
                        ////AF_VOBSUB_LOG_ERR(dbg_level,"malloc failure");
                        //mpeg->packet_reserve = 0;
                        mpeg->packet_size = 0;
                        return -1;
                    }
                    if (rar_read
                            (mpeg->packet, mpeg->packet_size, 1,
                             mpeg->stream) != 1)
                    {
                        ////AF_VOBSUB_LOG_ERR(dbg_level,"fread failure");
                        mpeg->packet_size = 0;
                        return -1;
                    }
                }
                else
                {
                    rar_seek(mpeg->stream, mpeg->packet_size,
                             SEEK_CUR);
                }
                idx = len;
            }
            break;
        case 0xbe:      /* Padding */
            if (rar_read(buf, 2, 1, mpeg->stream) != 1)
                return -1;
            len = buf[0] << 8 | buf[1];
            if (len > 0 && rar_seek(mpeg->stream, len, SEEK_CUR))
                return -1;
            break;
        default:
            if (0xc0 <= buf[3] && buf[3] < 0xf0)
            {
                /* MPEG audio or video */
                if (rar_read(buf, 2, 1, mpeg->stream) != 1)
                    return -1;
                len = buf[0] << 8 | buf[1];
                if (len > 0 && rar_seek(mpeg->stream, len, SEEK_CUR))
                    return -1;
            }
            else
            {
                ////AF_VOBSUB_LOG_ERR(dbg_level,"unknown header 0x%02X%02X%02X%02X\n",
                //                buf[0], buf[1], buf[2], buf[3]);
                return -1;
            }
    }
    return 0;
}

/**********************************************************************
 * Packet queue
 **********************************************************************/

static void packet_construct(packet_t *pkt)
{
    pkt->pts100 = 0;
    pkt->filepos = 0;
    //pkt->size = 0;
    //pkt->data = NULL;
}

static void packet_destroy(packet_t *pkt)
{
    //if (pkt->data)
    //free(pkt->data);
}

static void packet_queue_construct(packet_queue_t *queue)
{
    queue->id = NULL;
    queue->packets = NULL;
    queue->packets_reserve = 0;
    queue->packets_size = 0;
    queue->current_index = 0;
}

static void packet_queue_destroy(packet_queue_t *queue)
{
    if (queue->id)
    {
        free(queue->id);
        queue->id = NULL;
    }
    if (queue->packets)
    {
        while (queue->packets_size--)
            packet_destroy(queue->packets + queue->packets_size);
        free(queue->packets);
    }
    return;
}

/* Make sure there is enough room for needed_size packets in the
   packet queue. */
static int packet_queue_ensure(packet_queue_t *queue, unsigned int needed_size)
{
    if (queue->packets_reserve < needed_size)
    {
        if (queue->packets)
        {
            packet_t *tmp =
                realloc(queue->packets,
                        2 * queue->packets_reserve *
                        sizeof(packet_t));
            if (tmp == NULL)
            {
                ////AF_VOBSUB_LOG_ERR(dbg_level,"realloc failure");
                return -1;
            }
            queue->packets = tmp;
            queue->packets_reserve *= 2;
        }
        else
        {
            queue->packets = malloc(sizeof(packet_t));
            if (queue->packets == NULL)
            {
                ////AF_VOBSUB_LOG_ERR(dbg_level,"malloc failure");
                return -1;
            }
            queue->packets_reserve = 1;
        }
    }
    return 0;
}

/* add one more packet */
static int packet_queue_grow(packet_queue_t *queue)
{
    if (packet_queue_ensure(queue, queue->packets_size + 1) < 0)
        return -1;
    packet_construct(queue->packets + queue->packets_size);
    ++queue->packets_size;
    return 0;
}

/* insert a new packet, duplicating pts from the current one */
static int packet_queue_insert(packet_queue_t *queue)
{
    packet_t *pkts;
    if (packet_queue_ensure(queue, queue->packets_size + 1) < 0)
        return -1;
    /* XXX packet_size does not reflect the real thing here, it will be updated a bit later */
    memmove(queue->packets + queue->current_index + 2,
            queue->packets + queue->current_index + 1,
            sizeof(packet_t) * (queue->packets_size - queue->current_index -
                                1));
    pkts = queue->packets + queue->current_index;
    ++queue->packets_size;
    ++queue->current_index;
    packet_construct(pkts + 1);
    pkts[1].pts100 = pkts[0].pts100;
    pkts[1].filepos = pkts[0].filepos;
    return 0;
}

/**********************************************************************
 * Vobsub
 **********************************************************************/

/* Make sure that the spu stream idx exists. */
static int vobsub_ensure_spu_stream(vobsub_t *vob, unsigned int index)
{
    if (index >= vob->spu_streams_size)
    {
        /* This is a new stream */
        if (vob->spu_streams)
        {
            packet_queue_t *tmp =
                realloc(vob->spu_streams,
                        (index + 1) * sizeof(packet_queue_t));
            if (tmp == NULL)
            {
                ////AF_VOBSUB_LOG_ERR(dbg_level,"vobsub_ensure_spu_stream: realloc failure");
                return -1;
            }
            vob->spu_streams = tmp;
        }
        else
        {
            vob->spu_streams =
                malloc((index + 1) * sizeof(packet_queue_t));
            if (vob->spu_streams == NULL)
            {
                ////AF_VOBSUB_LOG_ERR(dbg_level,"vobsub_ensure_spu_stream: malloc failure");
                return -1;
            }
        }
        while (vob->spu_streams_size <= index)
        {
            packet_queue_construct(vob->spu_streams +
                                   vob->spu_streams_size);
            ++vob->spu_streams_size;
        }
    }
    return 0;
}

static int
vobsub_add_id(vobsub_t *vob, const char *id, size_t idlen,
              const unsigned int index)
{
    if (vobsub_ensure_spu_stream(vob, index) < 0)
        return -1;
    if (id && idlen)
    {
        if (vob->spu_streams[index].id)
            free(vob->spu_streams[index].id);
        vob->spu_streams[index].id = malloc(idlen + 1);
        if (vob->spu_streams[index].id == NULL)
        {
            ////AF_VOBSUB_LOG_ERR(dbg_level,"vobsub_add_id: malloc failure");
            return -1;
        }
        vob->spu_streams[index].id[idlen] = 0;
        memcpy(vob->spu_streams[index].id, id, idlen);
    }
    vob->spu_streams_current = index;
    return 0;
}

static int vobsub_add_timestamp(vobsub_t *vob, off_t filepos, int ms)
{
    packet_queue_t *queue;
    packet_t *pkt;
    if (vob->spu_streams == 0)
    {
        ////AF_VOBSUB_LOG_ERR(dbg_level,"[vobsub] warning, binning some index entries.  Check your index file\n");
        return -1;
    }
    queue = vob->spu_streams + vob->spu_streams_current;
    if (packet_queue_grow(queue) >= 0)
    {
        pkt = queue->packets + (queue->packets_size - 1);
        pkt->filepos = filepos;
        pkt->pts100 = ms < 0 ? UINT_MAX : (unsigned int)ms * 90;
        return 0;
    }
    return -1;
}

static int vobsub_parse_id(vobsub_t *vob, const char *line)
{
    // id: xx, index: n
    size_t idlen;
    const char *p, *q;
    p = line;
    while (isspace(*p))
        ++p;
    q = p;
    while (isalpha(*q) || (*q == '-'))    //if id: --, index: 0 ,can parse, fix OTT-3544
        ++q;
    idlen = q - p;
    if (idlen == 0)
        return -1;
    ++q;
    while (isspace(*q))
        ++q;
    if (strncmp("index:", q, 6))
        return -1;
    q += 6;
    while (isspace(*q))
        ++q;
    if (!isdigit(*q))
        return -1;
    return vobsub_add_id(vob, p, idlen, atoi(q));
}

static int vobsub_parse_timestamp(vobsub_t *vob, const char *line)
{
    // timestamp: HH:MM:SS.mmm, filepos: 0nnnnnnnnn
    const char *p;
    int h, m, s, ms;
    off_t filepos;
    while (isspace(*line))
        ++line;
    p = line;
    while (isdigit(*p))
        ++p;
    if (p - line != 2)
        return -1;
    h = atoi(line);
    if (*p != ':')
        return -1;
    line = ++p;
    while (isdigit(*p))
        ++p;
    if (p - line != 2)
        return -1;
    m = atoi(line);
    if (*p != ':')
        return -1;
    line = ++p;
    while (isdigit(*p))
        ++p;
    if (p - line != 2)
        return -1;
    s = atoi(line);
    if (*p != ':')
        return -1;
    line = ++p;
    while (isdigit(*p))
        ++p;
    if (p - line != 3)
        return -1;
    ms = atoi(line);
    if (*p != ',')
        return -1;
    line = p + 1;
    while (isspace(*line))
        ++line;
    if (strncmp("filepos:", line, 8))
        return -1;
    line += 8;
    while (isspace(*line))
        ++line;
    if (!isxdigit(*line))
        return -1;
    filepos = strtol(line, NULL, 16);
    return vobsub_add_timestamp(vob, filepos,
                                vob->delay + ms + 1000 * (s +
                                        60 * (m +
                                                60 * h)));
}

static int vobsub_parse_size(vobsub_t *vob, const char *line)
{
    // size: WWWxHHH
    char *p;
    while (isspace(*line))
        ++line;
    if (!isdigit(*line))
        return -1;
    vob->orig_frame_width = strtoul(line, &p, 10);
    if (*p != 'x')
        return -1;
    ++p;
    vob->orig_frame_height = strtoul(p, NULL, 10);
    return 0;
}

static int vobsub_parse_origin(vobsub_t *vob, const char *line)
{
    // org: X,Y
    char *p;
    while (isspace(*line))
        ++line;
    if (!isdigit(*line))
        return -1;
    vob->origin_x = strtoul(line, &p, 10);
    if (*p != ',')
        return -1;
    ++p;
    vob->origin_y = strtoul(p, NULL, 10);
    return 0;
}

static int vobsub_parse_palette(vobsub_t *vob, const char *line)
{
    // palette: XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX
    unsigned int n;
    n = 0;
    while (1)
    {
        const char *p;
        int r, g, b, y, u, v, tmp;
        while (isspace(*line))
            ++line;
        p = line;
        while (isxdigit(*p))
            ++p;
        if (p - line != 6)
            return -1;
        tmp = strtoul(line, NULL, 16);
        r = tmp >> 16 & 0xff;
        g = tmp >> 8 & 0xff;
        b = tmp & 0xff;
        y = MIN(MAX((int)(0.1494 * r + 0.6061 * g + 0.2445 * b), 0),
                0xff);
        u = MIN(MAX
                ((int)(0.6066 * r - 0.4322 * g - 0.1744 * b) + 128, 0),
                0xff);
        v = MIN(MAX
                ((int)(-0.08435 * r - 0.3422 * g + 0.4266 * b) + 128,
                 0), 0xff);
        vob->palette[n++] = y << 16 | u << 8 | v;
        //vob->palette[n++]=tmp;
        if (n == 16)
            break;
        if (*p == ',')
            ++p;
        line = p;
    }
    vob->have_palette = 1;
    return 0;
}

static int vobsub_parse_custom(vobsub_t *vob, const char *line)
{
    //custom colors: OFF/ON(0/1)
    if ((strncmp("ON", line + 15, 2) == 0)
            || strncmp("1", line + 15, 1) == 0)
        vob->custom = 1;
    else if ((strncmp("OFF", line + 15, 3) == 0)
             || strncmp("0", line + 15, 1) == 0)
        vob->custom = 0;
    else
        return -1;
    return 0;
}

static int vobsub_parse_cuspal(vobsub_t *vob, const char *line)
{
    //colors: XXXXXX, XXXXXX, XXXXXX, XXXXXX
    unsigned int n;
    n = 0;
    line += 40;
    while (1)
    {
        const char *p;
        while (isspace(*line))
            ++line;
        p = line;
        while (isxdigit(*p))
            ++p;
        if (p - line != 6)
            return -1;
        vob->cuspal[n++] = strtoul(line, NULL, 16);
        if (n == 4)
            break;
        if (*p == ',')
            ++p;
        line = p;
    }
    return 0;
}

/* don't know how to use tridx */
static int vobsub_parse_tridx(const char *line)
{
    //tridx: XXXX
    int tridx;
    tridx = strtoul((line + 26), NULL, 16);
    tridx =
        ((tridx & 0x1000) >> 12) | ((tridx & 0x100) >> 7) | ((tridx & 0x10)
                >> 2) | ((tridx
                          & 1)
                         << 3);
    return tridx;
}

static int vobsub_parse_delay(vobsub_t *vob, const char *line)
{
    int h, m, s, ms;
    int forward = 1;
    if (*(line + 7) == '+')
    {
        forward = 1;
        line++;
    }
    else if (*(line + 7) == '-')
    {
        forward = -1;
        line++;
    }
    //    AF_VOBSUB_LOG_INF(dbg_level, "forward=%d", forward);
    h = atoi(line + 7);
    //    AF_VOBSUB_LOG_INF(dbg_level, "h=%d," ,h);
    m = atoi(line + 10);
    //    AF_VOBSUB_LOG_INF(dbg_level, "m=%d,", m);
    s = atoi(line + 13);
    //    AF_VOBSUB_LOG_INF(dbg_level, "s=%d,", s);
    ms = atoi(line + 16);
    //    AF_VOBSUB_LOG_INF(dbg_level, "ms=%d", ms);
    vob->delay = (ms + 1000 * (s + 60 * (m + 60 * h))) * forward;
    return 0;
}

static int vobsub_set_lang(const char *line)
{
    if (vobsub_id == -1)
        vobsub_id = atoi(line + 8);
    return 0;
}

static int vobsub_parse_forced_subs(vobsub_t *vob, const char *line)
{
    const char *p;
    p = line;
    while (isspace(*p))
        ++p;
    if (strncasecmp("on", p, 2) == 0)
    {
        vob->forced_subs = ~0;
        return 0;
    }
    else if (strncasecmp("off", p, 3) == 0)
    {
        vob->forced_subs = 0;
        return 0;
    }
    return -1;
}

static int vobsub_parse_one_line(vobsub_t *vob, rar_stream_t *fd)
{
    ssize_t line_size;
    int res = -1;
    size_t line_reserve = 0;
    char *line = NULL;
    do
    {
        line_size = getSubLine(&line, &line_reserve, fd);
        if (line_size < 0)
        {
            break;
        }
        if (*line == 0 || *line == '\r' || *line == '\n'
                || *line == '#')
            continue;
        else if (strncmp("langidx:", line, 8) == 0)
            res = vobsub_set_lang(line);
        else if (strncmp("delay:", line, 6) == 0)
            res = vobsub_parse_delay(vob, line);
        else if (strncmp("id:", line, 3) == 0)
            res = vobsub_parse_id(vob, line + 3);
        else if (strncmp("palette:", line, 8) == 0)
            res = vobsub_parse_palette(vob, line + 8);
        else if (strncmp("size:", line, 5) == 0)
            res = vobsub_parse_size(vob, line + 5);
        else if (strncmp("org:", line, 4) == 0)
            res = vobsub_parse_origin(vob, line + 4);
        else if (strncmp("timestamp:", line, 10) == 0)
            res = vobsub_parse_timestamp(vob, line + 10);
        else if (strncmp("custom colors:", line, 14) == 0)
            //custom colors: ON/OFF, tridx: XXXX, colors: XXXXXX, XXXXXX, XXXXXX,XXXXXX
            res =
                vobsub_parse_cuspal(vob,
                                    line) +
                vobsub_parse_tridx(line) + vobsub_parse_custom(vob,
                        line);
        else if (strncmp("forced subs:", line, 12) == 0)
            res = vobsub_parse_forced_subs(vob, line + 12);
        else
        {
            //        AF_VOBSUB_LOG_INF(dbg_level, "vobsub: ignoring %s", line);
            continue;
        }
        //if (res < 0)
            ////AF_VOBSUB_LOG_ERR(dbg_level,  "ERROR in %s", line);
            //break;
    }
    while (1);
    if (line)
        free(line);
    return res;
}

int
vobsub_parse_ifo(void *this, const char *const name, unsigned int *palette,
                 unsigned int *width, unsigned int *height, int force, int sid,
                 char *langid)
{
    vobsub_t *vob = (vobsub_t *) this;
    int res = -1;
    rar_stream_t *fd = rar_open(name, "rb");
    if (fd == NULL)
    {
        if (force)
        {
            //AF_VOBSUB_LOG_ERR(dbg_level, "VobSub: Can't open IFO file\n");
        }
    }
    else
    {
        // parse IFO header
        unsigned char block[0x800];
        const char *const ifo_magic = "DVDVIDEO-VTS";
        if (rar_read(block, sizeof(block), 1, fd) != 1)
        {
            if (force)
            {
                //AF_VOBSUB_LOG_ERR(dbg_level, "VobSub: Can't read IFO header\n");
            }
        }
        else if (memcmp(block, ifo_magic, strlen(ifo_magic) + 1))
        {
            //AF_VOBSUB_LOG_ERR(dbg_level, "VobSub: Bad magic in IFO header\n");
        }
        else
        {
            unsigned long pgci_sector =
                block[0xcc] << 24 | block[0xcd] << 16 | block[0xce]
                << 8 | block[0xcf];
            int standard = (block[0x200] & 0x30) >> 4;
            int resolution = (block[0x201] & 0x0c) >> 2;
            *height = standard ? 576 : 480;
            *width = 0;
            switch (resolution)
            {
                case 0x0:
                    *width = 720;
                    break;
                case 0x1:
                    *width = 704;
                    break;
                case 0x2:
                    *width = 352;
                    break;
                case 0x3:
                    *width = 352;
                    *height /= 2;
                    break;
                default:
                    //AF_VOBSUB_LOG_ERR(dbg_level,"Vobsub: Unknown resolution %d \n", resolution);
                    break;
            }
            if (langid && 0 <= sid && sid < 32)
            {
                unsigned char *tmp =
                    block + 0x256 + sid * 6 + 2;
                langid[0] = tmp[0];
                langid[1] = tmp[1];
                langid[2] = 0;
            }
            if (rar_seek(fd, pgci_sector * sizeof(block), SEEK_SET)
                    || rar_read(block, sizeof(block), 1, fd) != 1)
            {
                //AF_VOBSUB_LOG_ERR(dbg_level, "VobSub: Can't read IFO PGCI\n");
            }
            else
            {
                unsigned long idx;
                unsigned long pgc_offset =
                    block[0xc] << 24 | block[0xd] << 16 |
                    block[0xe] << 8 | block[0xf];
                for (idx = 0; idx < 16; ++idx)
                {
                    unsigned char *p =
                        block + pgc_offset + 0xa4 + 4 * idx;
                    palette[idx] =
                        p[0] << 24 | p[1] << 16 | p[2] << 8
                        | p[3];
                }
                if (vob)
                    vob->have_palette = 1;
                res = 0;
            }
        }
        rar_close(fd);
    }
    return res;
}

void vobsub_close(void *this)
{
    vobsub_t *vob = (vobsub_t *) this;
    if (vob->spu_streams)
    {
        while (vob->spu_streams_size--)
            packet_queue_destroy(vob->spu_streams +
                                 vob->spu_streams_size);
        free(vob->spu_streams);
    }
    free(vob);
}

static void close_subtitle(subtitlevobsub_t *subtitlevobsub)
{
    if (subtitlevobsub->vobsub)
    {
        vobsub_close(subtitlevobsub->vobsub);
        subtitlevobsub->vobsub = NULL;
    }
    if (subtitlevobsub->mpeg)
    {
        mpeg_free(subtitlevobsub->mpeg);
        subtitlevobsub->mpeg = NULL;
    }
    if (subtitlevobsub->cur_idx_url)
    {
        free(subtitlevobsub->cur_idx_url);
        subtitlevobsub->cur_idx_url = NULL;
    }
    if (subtitlevobsub->vob_pixData)
    {
        free(subtitlevobsub->vob_pixData);
        subtitlevobsub->vob_pixData = NULL;
    }
    subtitlevobsub->cur_track_id = 0;
    subtitlevobsub->cur_pts100 = 0;
    subtitlevobsub->cur_endpts100 = 0;
    subtitlevobsub->next_pts100 = 0;
    subtitlevobsub->next_filepos = 0;
}

//this function can be used to call java to show ui. form the data subdata
static void show_vob_subtitle(subtitlevobsub_t *subtitlevobsub)
{
    subtitlevobsub->vob_subtitle_config.left =
        subtitlevobsub->VobSPU.spu_start_x;
    subtitlevobsub->vob_subtitle_config.top =
        subtitlevobsub->VobSPU.spu_start_y;
    subtitlevobsub->vob_subtitle_config.width =
        (((subtitlevobsub->VobSPU.spu_width + 7) >> 3) << 3);
    subtitlevobsub->vob_subtitle_config.height =
        subtitlevobsub->VobSPU.spu_height;
    subtitlevobsub->vob_subtitle_config.contrast =
        subtitlevobsub->VobSPU.spu_alpha;
    subtitlevobsub->vob_subtitle_config.prtData =
        (unsigned long)subtitlevobsub->vob_pixData;
    if (subtitlevobsub->VobSPU.display_pending == 0
            || subtitlevobsub->VobSPU.displaying)
    {
        subtitlevobsub->vob_subtitle_config.cls = 1;
    }
    else
    {
        subtitlevobsub->vob_subtitle_config.cls = 0;
    }
}

/* code is copied from vob_sub.c */

static unsigned short DecodeRL(unsigned short RLData, unsigned short *pixelnum,
                               unsigned short *pixeldata)
{
    unsigned short nData = RLData;
    unsigned short nShiftNum;
    unsigned short nDecodedBits;
    if (nData & 0xc000)
        nDecodedBits = 4;
    else if (nData & 0x3000)
        nDecodedBits = 8;
    else if (nData & 0x0c00)
        nDecodedBits = 12;
    else
        nDecodedBits = 16;
    nShiftNum = 16 - nDecodedBits;
    *pixeldata = (nData >> nShiftNum) & 0x0003;
    *pixelnum = nData >> (nShiftNum + 2);
    return nDecodedBits;
}

static unsigned short GetWordFromPixBuffer(subtitlevobsub_t *subtitlevobsub,
        unsigned short bitpos)
{
    unsigned char hi = 0, lo = 0, hi_ = 0, lo_ = 0;
    char *tmp = (char *)subtitlevobsub->vob_ptrPXDRead;
    hi = *(tmp + 0);
    lo = *(tmp + 1);
    hi_ = *(tmp + 2);
    lo_ = *(tmp + 3);
    if (bitpos == 0)
    {
        return (hi << 0x8 | lo);
    }
    else
    {
        return (((hi << 0x8 | lo) << bitpos) |
                ((hi_ << 0x8 | lo_) >> (16 - bitpos)));
    }
}

unsigned char vob_fill_pixel(subtitlevobsub_t *subtitlevobsub, int n)
{
    unsigned short nPixelNum = 0, nPixelData = 0;
    unsigned short nRLData, nBits;
    unsigned short nDecodedPixNum = 0;
    unsigned short i, j;
    unsigned short rownum = subtitlevobsub->VobSPU.spu_width;
    unsigned short _alpha = subtitlevobsub->VobSPU.spu_alpha;
    unsigned short PXDBufferBitPos = 0, WrOffset = 16;
    //  unsigned short totalBits = 0;
    unsigned short change_data = 0;
    unsigned short PixelDatas[4] = { 0, 1, 2, 3 };
    unsigned short *vob_ptrPXDWrite;
    // 4 buffer for pix data
    if (n == 1)     // 1 for odd
    {
        memset(subtitlevobsub->vob_pixData, 0, OSD_HALF_SIZE);
        vob_ptrPXDWrite = (unsigned short *)subtitlevobsub->vob_pixData;
    }
    else if (n == 2)    // 2 for even
    {
        memset(subtitlevobsub->vob_pixData + OSD_HALF_SIZE, 0,
               OSD_HALF_SIZE);
        vob_ptrPXDWrite =
            (unsigned short *)(subtitlevobsub->vob_pixData +
                               OSD_HALF_SIZE);
    }
    else
    {
        return -1;
    }
    if (_alpha & 0xF)
    {
        _alpha = _alpha >> 4;
        change_data++;
        while (_alpha & 0xF)
        {
            change_data++;
            _alpha = _alpha >> 4;
        }
        PixelDatas[0] = change_data;
        PixelDatas[change_data] = 0;
        if (n == 2)
            subtitlevobsub->VobSPU.spu_alpha =
                (subtitlevobsub->VobSPU.
                 spu_alpha & 0xFFF0) | (0x000F << (change_data <<
                                                   2));
    }
    for (j = 0; j < subtitlevobsub->VobSPU.spu_height / 2; j++)
    {
        while (nDecodedPixNum < rownum)
        {
            nRLData =
                GetWordFromPixBuffer(subtitlevobsub,
                                     PXDBufferBitPos);
            nBits = DecodeRL(nRLData, &nPixelNum, &nPixelData);
            PXDBufferBitPos += nBits;
            if (PXDBufferBitPos >= 16)
            {
                PXDBufferBitPos -= 16;
                subtitlevobsub->vob_ptrPXDRead++;
            }
            if (nPixelNum == 0)
            {
                nPixelNum = rownum - nDecodedPixNum % rownum;
            }
            if (change_data)
            {
                nPixelData = PixelDatas[nPixelData];
            }
            for (i = 0; i < nPixelNum; i++)
            {
                WrOffset -= 2;
                *vob_ptrPXDWrite |= nPixelData << WrOffset;
                if (WrOffset == 0)
                {
                    WrOffset = 16;
                    vob_ptrPXDWrite++;
                }
            }
            //          totalBits += nBits;
            nDecodedPixNum += nPixelNum;
        }
        if (PXDBufferBitPos == 4)   //Rule 6
        {
            PXDBufferBitPos = 8;
        }
        else if (PXDBufferBitPos == 12)
        {
            PXDBufferBitPos = 0;
            subtitlevobsub->vob_ptrPXDRead++;
        }
        if (WrOffset != 16)
        {
            WrOffset = 16;
            vob_ptrPXDWrite++;
        }
        nDecodedPixNum -= rownum;
    }
    /*if (totalBits == subpichdr->field_offset){
       return 1;
       }
     */
    return 0;
}

extern unsigned short doDCSQC(unsigned char *pdata,unsigned char *pend);

static int do_vob_sub_cmd(subtitlevobsub_t *subtitlevobsub,
                          unsigned char *packet)
{
    unsigned short m_OffsetToCmd;
    unsigned short m_SubPicSize;
    unsigned short temp;
    unsigned char *pCmdData;
    unsigned char *pCmdEnd;
    unsigned char data_byte0, data_byte1;
    unsigned char spu_cmd;
    m_SubPicSize = (packet[0] << 8) | (packet[1]);
    m_OffsetToCmd = (packet[2] << 8) | (packet[3]);
    if (m_OffsetToCmd >= m_SubPicSize)
        return FAIL;
    pCmdData = packet;
    pCmdEnd = pCmdData + m_SubPicSize;
    pCmdData += m_OffsetToCmd;
    pCmdData += 4;
    while (pCmdData < pCmdEnd)
    {
        spu_cmd = *pCmdData++;
        switch (spu_cmd)
        {
            case FSTA_DSP:
                subtitlevobsub->VobSPU.display_pending = 2;
                break;
            case STA_DSP:
                subtitlevobsub->VobSPU.display_pending = 1;
                break;
            case STP_DSP:
                subtitlevobsub->VobSPU.display_pending = 0;
                break;
            case SET_COLOR:
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.spu_color = temp << 8;
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.spu_color += temp;
                break;
            case SET_CONTR:
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.spu_alpha = temp << 8;
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.spu_alpha += temp;
                break;
            case SET_DAREA:
                data_byte0 = *pCmdData++;
                data_byte1 = *pCmdData++;
                subtitlevobsub->VobSPU.spu_start_x =
                    ((data_byte0 & 0x3f) << 4) | (data_byte1 >> 4);
                data_byte0 = *pCmdData++;
                subtitlevobsub->VobSPU.spu_width =
                    ((data_byte1 & 0x07) << 8) | (data_byte0);
                subtitlevobsub->VobSPU.spu_width =
                    subtitlevobsub->VobSPU.spu_width -
                    subtitlevobsub->VobSPU.spu_start_x + 1;
                data_byte0 = *pCmdData++;
                data_byte1 = *pCmdData++;
                subtitlevobsub->VobSPU.spu_start_y =
                    ((data_byte0 & 0x3f) << 4) | (data_byte1 >> 4);
                data_byte0 = *pCmdData++;
                subtitlevobsub->VobSPU.spu_height =
                    ((data_byte1 & 0x07) << 8) | (data_byte0);
                subtitlevobsub->VobSPU.spu_height =
                    subtitlevobsub->VobSPU.spu_height -
                    subtitlevobsub->VobSPU.spu_start_y + 1;
#if 0
                if ((subtitlevobsub->VobSPU.spu_width > 720) ||
                        (subtitlevobsub->VobSPU.spu_height > 576)
                   )
                {
                    subtitlevobsub->VobSPU.spu_width = 720;
                    subtitlevobsub->VobSPU.spu_height = 576;
                }
#endif
                break;
            case SET_DSPXA:
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.top_pxd_addr = temp << 8;
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.top_pxd_addr += temp;
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.bottom_pxd_addr = temp << 8;
                temp = *pCmdData++;
                subtitlevobsub->VobSPU.bottom_pxd_addr += temp;
                break;
            case CHG_COLCON:
                temp = *pCmdData++;
                temp = temp << 8;
                temp += *pCmdData++;
                pCmdData += temp;
                /*
                   subtitlevobsub->VobSPU.disp_colcon_addr = subtitlevobsub->VobSPU.point + subtitlevobsub->VobSPU.point_offset;
                   subtitlevobsub->VobSPU.colcon_addr_valid = 1;
                   temp = subtitlevobsub->VobSPU.disp_colcon_addr + temp - 2;

                   uSPU.point = temp & 0x1fffc;
                   uSPU.point_offset = temp & 3;
                 */
                break;
            case CMD_END:
                if (pCmdData <= (pCmdEnd - 6))
                {
                    subtitlevobsub->duration =
                        doDCSQC(pCmdData, pCmdEnd - 6);
                    if (subtitlevobsub->duration > 0)
                        subtitlevobsub->duration *= 1024;
                }
                else
                {
                    subtitlevobsub->duration = 0;
                }
                return SUCCESS;
                break;
            default:
                return FAIL;
        }
    }
    return FAIL;
}

/*end*/

/*****************************************************************
**                                                              **
**    Method  functions                                         **
**                                                              **
**                                                              **
*****************************************************************/
//static int SubtitleVOBSub_Init(control_t* cntl, cond_item_t* param)
//{
//    int ret=-1;
//    subtitlevobsub_t* subtitlevobsub= subtitlevobsub_init(cntl);
//    if(subtitlevobsub)
//    {
//        ret=0;
//        /*  Add Initialize Code here , set ret to be -1 if fail*/
//
//        /* end */
//        cntl->private_data=subtitlevobsub;
//    }
//    if(ret<0)
//        subtitlevobsub_release(cntl);
//
//    return ret;
//}

//static int SubtitleVOBSub_UnInit(control_t* cntl, cond_item_t* param)
//{
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t*)(cntl->private_data);
//    /* Add UnInitialize code here */
//    close_subtitle(subtitlevobsub);
//
//    /* end */
//    subtitlevobsub_release(cntl);
//    return 0;
//}

//static int SubtitleVOBSub_MsgProcess(control_t* cntl, cond_item_t* param)
//{
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t*)(cntl->private_data);
//    message_t* msg = (message_t *)param;
//    switch(msg->id){
//        case AFMSG_BPLAY_SHOW_SUBTITLE:
//            if(msg->param[0]==SUBTITLE_TYPE_VOB_RAW){
//                unsigned char* subdata=(unsigned char *)(msg->param[1]);
//                if(subdata){
//                    if(do_vob_sub_cmd(subtitlevobsub,subdata)==SUCCESS) {
//                        if(subtitlevobsub->vob_pixData==NULL)
//                            subtitlevobsub->vob_pixData = malloc(OSD_HALF_SIZE*2);
//
//                        if (subtitlevobsub->vob_pixData) {
//                            subtitlevobsub->vob_ptrPXDRead = (unsigned short *)((unsigned)(subdata)+subtitlevobsub->VobSPU.top_pxd_addr);
//                            vob_fill_pixel(subtitlevobsub,1);        // 1 for odd, 2 for even
//                            subtitlevobsub->vob_ptrPXDRead = (unsigned short *)((unsigned)(subdata)+subtitlevobsub->VobSPU.bottom_pxd_addr);
//                            vob_fill_pixel(subtitlevobsub,2);        // 1 for odd, 2 for even
//                            show_vob_subtitle(subtitlevobsub);
//                        }
//                    }
//                }
//                else{
//                    subtitlevobsub_send_msg_bplay_show_subtitle(subtitlevobsub->cntl, BROADCAST_ALL, SUBTITLE_TYPE_VOB, 0);
//                }
//            }
//            break;
//        default:
//            break;
//    }
//    return 0;
//}

//static int SubtitleVOBSub_SetAppInfo(control_t* cntl, cond_item_t* param)
//{
//    int ret=0;
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t*)(cntl->private_data);
//    int data0 = (int)(param[0]);
//    int data1 = (int)(param[1]);
//    int data2 = (int)(param[2]);
//    /* Add Method code here, Set ret to be -1 if fail */
//
//    /* end */
//    return ret;
//}

//static int SubtitleVOBSub_SetExtSubtitle(control_t* cntl, cond_item_t* param)
static int SubtitleVOBSub_SetExtSubtitle(subtitlevobsub_t *subtitlevobsub,
        char *fileurl, int flag1)
{
    int ret = 0;
    mpeg_t *mpeg;
    int address = 0;
    char *url = fileurl;
    int flag = flag1;
    unsigned int track_id = 1;
    /* Add Method code here, Set ret to be -1 if fail */
    char *subname;
    char extname[MAX_EXTNAME_LEN];
    unsigned int i, count;
    int namelen = strlen(url);
    strcpy(extname, url + namelen - 3);
    if ((strncasecmp(extname, "idx", 3))
            && (strncasecmp(extname, "IDX", 3)))
    {
        return 0;
    }
    if (subtitlevobsub->vobsub && subtitlevobsub->mpeg
            && subtitlevobsub->cur_idx_url
            && (strcmp(url, subtitlevobsub->cur_idx_url) == 0))
    {
        count = 0;
        for (i = 0; i < subtitlevobsub->vobsub->spu_streams_size; i++)
        {
            if (subtitlevobsub->vobsub->spu_streams[i].id)
                count++;
        }
        ret = count;
    }
    else
    {
        close_subtitle(subtitlevobsub);
        subname = strdup(url);
        subname[namelen - 3] = 's';
        subname[namelen - 2] = 'u';
        subname[namelen - 1] = 'b';
        mpeg = mpeg_open(subname);
        if (mpeg)
        {
            subtitlevobsub->mpeg = mpeg;
            vobsub_t *vob = calloc(sizeof(vobsub_t), 1);
            if (vob)
            {
                rar_stream_t *file;
                file = rar_open(url, "rb");
                if (file)
                {
                    while (vobsub_parse_one_line(vob, file)
                            >= 0)
                        /* NOOP */ ;
                    rar_close(file);
                    if ((vob->custom == 0)
                            && (vob->have_palette != 1))
                        vob->custom = 1;
                    subtitlevobsub->vobsub = vob;
                    subtitlevobsub->cur_idx_url =
                        strdup(url);
                    count = 0;
                    for (i = 0; i < vob->spu_streams_size;
                            i++)
                    {
                        if (vob->spu_streams[i].id)
                            count++;
                    }
                    ret = count;
                }
                else
                {
                    free(vob);
                    goto exit;
                }
            }
        }
exit:
        free(subname);
    }
    if (ret > 0)
    {
        if (flag == 1)
        {
            count = 0;
            for (i = 0;
                    (i < subtitlevobsub->vobsub->spu_streams_size);
                    i++)
            {
                if (subtitlevobsub->vobsub->spu_streams[i].id)
                {
                    if (count == track_id)
                        break;
                    count++;
                }
            }
            if (i < subtitlevobsub->vobsub->spu_streams_size)
                subtitlevobsub->cur_track_id = i;
            else
                subtitlevobsub->cur_track_id = 0;
        }
        subtitlevobsub->cur_pts100 = 0;
        subtitlevobsub->cur_endpts100 = 0;
        subtitlevobsub->next_pts100 = 0;
        subtitlevobsub->next_filepos = 0;
    }
    else
    {
        close_subtitle(subtitlevobsub);
    }
    /* end */
    return ret;
}

//static int SubtitleVOBSub_ChangeSubtitle(control_t* cntl, cond_item_t* param)
//{
//    int ret=0;
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t*)(cntl->private_data);
//    int id = (int)(param[0]);
//    /* Add Method code here, Set ret to be -1 if fail */
//
//    /* end */
//    return ret;
//}

//static int SubtitleVOBSub_RemoveExtSub(control_t* cntl, cond_item_t* param)
//{
//    int ret=0;
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t*)(cntl->private_data);
//    int index = (int)(param[0]);
//    /* Add Method code here, Set ret to be -1 if fail */
//
//    /* end */
//    return ret;
//}

static int SubtitleVOBSub_ShowSubtitle(subtitlevobsub_t *subtitlevobsub,
                                       int pts)
{
    int ret = 0;
    /* Add Method code here, Set ret to be -1 if fail */
    unsigned int time = pts;
    if (subtitlevobsub->vobsub)
    {
        int seek_pts100 = time;
        if ((seek_pts100 < subtitlevobsub->cur_pts100)
                || (seek_pts100 >= subtitlevobsub->cur_endpts100))
        {
            subtitlevobsub->cur_endpts100 = 0x7fffffff;
            subtitlevobsub->next_filepos = 0;
        }
        if (seek_pts100 >= subtitlevobsub->next_pts100
                || seek_pts100 < subtitlevobsub->cur_pts100)
        {
            unsigned int i;
            vobsub_t *vob = subtitlevobsub->vobsub;
            packet_queue_t *queue;
            mpeg_t *mpg = subtitlevobsub->mpeg;
            queue = vob->spu_streams + subtitlevobsub->cur_track_id;
            if (queue->packets_size == 0)
                return 0;
            for (i = 0; i < queue->packets_size; i++)
            {
                if (queue->packets[i].pts100 >= seek_pts100)
                {
                    break;
                }
            }
            if (i > 0)
                i--;
            if (seek_pts100 >= queue->packets[i].pts100)
            {
                if (rar_seek
                        (mpg->stream, queue->packets[i].filepos,
                         SEEK_SET) == 0)
                {
                    while (!mpeg_eof(mpg))
                    {
                        if (mpeg_run(mpg, 1) < 0)
                        {
                            if (!mpeg_eof(mpg))
                                //AF_VOBSUB_LOG_ERR(dbg_level,"VobSub: mpeg_run error\n");
                                break;
                        }
                        if (mpg->packet_size)
                        {
                            subtitlevobsub->
                            duration = 0;
                            if ((mpg->packet)
                                    &&
                                    ((mpg->
                                      aid & 0xe0) ==
                                     0x20)
                                    &&
                                    ((mpg->
                                      aid & 0x1f) ==
                                     subtitlevobsub->
                                     cur_track_id))
                            {
                                unsigned char
                                *rawsubdata,
                                *subdata_ptr;
                                int sublen, len;
                                sublen =
                                    (mpg->
                                     packet[0]
                                     << 8) |
                                    (mpg->
                                     packet[1]);
                                rawsubdata =
                                    malloc
                                    (sublen);
                                subdata_ptr =
                                    rawsubdata;
                                if (rawsubdata)
                                {
                                    len =
                                        mpg->
                                        packet_size;
                                    if (len
                                            >
                                            sublen)
                                        len = sublen;
                                    memcpy
                                    (rawsubdata,
                                     mpg->
                                     packet,
                                     len);
                                    free(mpg->packet);
                                    mpg->
                                    packet
                                        =
                                            NULL;
                                    sublen
                                    -=
                                        len;
                                    subdata_ptr
                                    +=
                                        len;
                                    while ((!mpeg_eof(mpg)) && (sublen > 0))
                                    {
                                        if (mpeg_run(mpg, 1) < 0)
                                        {
                                            if (!mpeg_eof(mpg))
                                            {
                                                //AF_VOBSUB_LOG_ERR(dbg_level,"VobSub: mpeg_run error\n");
                                                break;
                                            }
                                        }
                                        if (mpg->packet_size)
                                        {
                                            if ((mpg->packet) && ((mpg->aid & 0xe0) == 0x20) && ((mpg->aid & 0x1f) == subtitlevobsub->cur_track_id))
                                            {
                                                len = mpg->packet_size;
                                                if (len > sublen)
                                                    len = sublen;
                                                memcpy
                                                (subdata_ptr,
                                                 mpg->
                                                 packet,
                                                 len);
                                                sublen
                                                -=
                                                    len;
                                                subdata_ptr
                                                +=
                                                    len;
                                                free(mpg->packet);
                                                mpg->
                                                packet
                                                    =
                                                        NULL;
                                            }
                                            else
                                            {
                                                free(mpg->packet);
                                                mpg->
                                                packet
                                                    =
                                                        NULL;
                                                break;
                                            }
                                        }
                                    }
                                    if (do_vob_sub_cmd(subtitlevobsub, rawsubdata) == SUCCESS)
                                    {
                                        if (subtitlevobsub->vob_pixData == NULL)
                                            subtitlevobsub->
                                            vob_pixData
                                                =
                                                    malloc
                                                    (OSD_HALF_SIZE
                                                     *
                                                     2);
                                        if (subtitlevobsub->vob_pixData)
                                        {
                                            subtitlevobsub->
                                            vob_ptrPXDRead
                                                = (unsigned long *)((unsigned long)(rawsubdata) + subtitlevobsub->VobSPU.top_pxd_addr);
                                            vob_fill_pixel(subtitlevobsub, 1);  // 1 for odd, 2 for even
                                            subtitlevobsub->
                                            vob_ptrPXDRead
                                                =
                                                    (unsigned
                                                     long
                                                     *)
                                                    ((unsigned long)(rawsubdata) + subtitlevobsub->VobSPU.bottom_pxd_addr);
                                            vob_fill_pixel(subtitlevobsub, 2);  // 1 for odd, 2 for even
                                            show_vob_subtitle
                                            (subtitlevobsub);
                                            LOGI("get subtitle ----------------yes\n");
                                            ret = 1;
                                        }
                                    }
                                    free(rawsubdata);
                                }
                            }
                            subtitlevobsub->
                            cur_pts100 =
                                queue->packets[i].
                                pts100;
                            if ((i + 1) <
                                    queue->
                                    packets_size)
                            {
                                subtitlevobsub->
                                next_pts100
                                    =
                                        queue->
                                        packets[i +
                                                1].
                                        pts100;
                            }
                            else
                            {
                                subtitlevobsub->
                                next_pts100
                                    =
                                        0x7fffffff;
                            }
                            if (subtitlevobsub->
                                    duration > 0)
                                subtitlevobsub->
                                cur_endpts100
                                    =
                                        subtitlevobsub->
                                        cur_pts100 +
                                        subtitlevobsub->
                                        duration;
                            else if
                            (subtitlevobsub->
                                    next_pts100 >=
                                    subtitlevobsub->
                                    cur_pts100)
                                subtitlevobsub->
                                cur_endpts100
                                    =
                                        subtitlevobsub->
                                        next_pts100
                                        + 1;
                            break;
                        }
                    }
                }
            }
        }
    }
    /* end */
    return ret;
}

//static int SubtitleVOBSub_HideSubtitle(control_t* cntl, cond_item_t* param)
//{
//    int ret=0;
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t*)(cntl->private_data);
//    /* Add Method code here, Set ret to be -1 if fail */
//    subtitlevobsub_send_msg_bplay_show_subtitle(subtitlevobsub->cntl, BROADCAST_ALL, SUBTITLE_TYPE_VOB, 0);
//
//    /* end */
//    return ret;
//}
//
//static int SubtitleVOBSub_QuitSubtitle(control_t* cntl, cond_item_t* param)
//{
//    int ret=0;
//    subtitlevobsub_t* subtitlevobsub = (subtitlevobsub_t*)(cntl->private_data);
//    /* Add Method code here, Set ret to be -1 if fail */
//    close_subtitle(subtitlevobsub);
//    subtitlevobsub_send_msg_bplay_show_subtitle(subtitlevobsub->cntl, BROADCAST_ALL, SUBTITLE_TYPE_VOB, 0);
//    /* end */
//    return ret;
//}

unsigned int totalsubnum = 0;

subtitlevobsub_t *getIdxSubData(int ptms)
{
    if (SubtitleVOBSub_ShowSubtitle(vobsubdata, ptms * 90) == 1)
        return vobsubdata;
    else
        return NULL;
}

void switch_sub(int index)
{
    if (totalsubnum > index || vobsubdata != NULL)
    {
        vobsubdata->cur_track_id = index;
    }
}

static void ini_subdata(subtitlevobsub_t *subtitlevobsub)
{
    subtitlevobsub->vobsub = NULL;
    subtitlevobsub->mpeg = NULL;
    subtitlevobsub->cur_idx_url = NULL;
    subtitlevobsub->vob_pixData = NULL;
    subtitlevobsub->cur_track_id = 0;
    subtitlevobsub->cur_pts100 = 0;
    subtitlevobsub->cur_endpts100 = 0;
    subtitlevobsub->next_pts100 = 0;
    subtitlevobsub->next_filepos = 0;
}

//when change to other idxsub file ,should call close_subtitle() to free memory first;
int idxsub_init_subtitle(char *fileurl, int index)
{
    if (vobsubdata == NULL)
    {
        vobsubdata =
            (subtitlevobsub_t *) malloc(sizeof(subtitlevobsub_t));
        memset(vobsubdata, 0x0, sizeof(subtitlevobsub_t));
        ini_subdata(vobsubdata);
    }
    totalsubnum = SubtitleVOBSub_SetExtSubtitle(vobsubdata, fileurl, 1);
    switch_sub(index);
    return totalsubnum;
}

void idxsub_close_subtitle()
{
    if (vobsubdata != NULL)
    {
        close_subtitle(vobsubdata);
    }
}

void covert2bto32b(const unsigned char *source, long length, int bytesPerLine,
                   unsigned int *dist, int subtitle_alpha)
{
    if (dist == NULL)
    {
        return;
    }
    char value[PROPERTY_VALUE_MAX] = { 0 };
    unsigned int RGBA_Pal[4];
    RGBA_Pal[0] = RGBA_Pal[1] = RGBA_Pal[2] = RGBA_Pal[3] = 0;
    int aAlpha[4];
    int aPalette[4];
    int rgb0 = 0;
    int rgb1 = 0xffffff;
    int rgb2 = 0;
    int rgb3 = 0;
    int set_rgb = 0;
    /*  update Alpha */
    aAlpha[1] = ((subtitle_alpha >> 8) >> 4) & 0xf;
    aAlpha[0] = (subtitle_alpha >> 8) & 0xf;
    aAlpha[3] = (subtitle_alpha >> 4) & 0xf;
    aAlpha[2] = subtitle_alpha & 0xf;
    /* update Palette */
    aPalette[0] = ((vobsubdata->VobSPU.spu_color >> 8) >> 4) & 0xf;
    aPalette[1] = (vobsubdata->VobSPU.spu_color >> 8) & 0xf;
    aPalette[2] = (vobsubdata->VobSPU.spu_color >> 4) & 0xf;
    aPalette[3] = vobsubdata->VobSPU.spu_color & 0xf;
    if (property_get("media.vobsub.setrgb.enable", value, NULL) > 0)
    {
        if (!strcmp(value, "1") || !strcmp(value, "true"))
        {
            set_rgb = 1;
            LOGI("aAlpha[0] = 0x%x \n", aAlpha[0]);
            LOGI("aAlpha[1] = 0x%x \n", aAlpha[1]);
            LOGI("aAlpha[2] = 0x%x \n", aAlpha[2]);
            LOGI("aAlpha[3] = 0x%x \n", aAlpha[3]);
            LOGI("aPalette[0] = 0x%x \n", aPalette[0]);
            LOGI("aPalette[1] = 0x%x \n", aPalette[1]);
            LOGI("aPalette[2] = 0x%x \n", aPalette[2]);
            LOGI("aPalette[3] = 0x%x \n", aPalette[3]);
            LOGI("vobsubdata->vobsub->palette[aPalette[0]] = 0x%x \n", vobsubdata->vobsub->palette[aPalette[0]]);
            LOGI("vobsubdata->vobsub->palette[aPalette[1]] = 0x%x \n", vobsubdata->vobsub->palette[aPalette[1]]);
            LOGI("vobsubdata->vobsub->palette[aPalette[2]] = 0x%x \n", vobsubdata->vobsub->palette[aPalette[2]]);
            LOGI("vobsubdata->vobsub->palette[aPalette[3]] = 0x%x \n", vobsubdata->vobsub->palette[aPalette[3]]);
        }
    }
    aAlpha[1] = 0;
    if (set_rgb)
    {
        if (property_get("media.vobsub.rgb0", value, NULL) > 0)
        {
            rgb0 = atoi(value);
            LOGI("rgb0 = 0x%x \n", rgb0);
        }
        if (property_get("media.vobsub.rgb1", value, NULL) > 0)
        {
            rgb1 = atoi(value);
            LOGI("rgb1 = 0x%x \n", rgb1);
        }
        if (property_get("media.vobsub.rgb2", value, NULL) > 0)
        {
            rgb2 = atoi(value);
            LOGI("rgb2 = 0x%x \n", rgb2);
        }
        if (property_get("media.vobsub.rgb3", value, NULL) > 0)
        {
            rgb3 = atoi(value);
            LOGI("rgb3 = 0x%x \n", rgb3);
        }
        RGBA_Pal[0] = ((aAlpha[0] == 0) ? 0xff000000 : 0x0) + rgb0;
        RGBA_Pal[1] = ((aAlpha[1] == 0) ? 0xff000000 : 0x0) + rgb1;
        RGBA_Pal[2] = ((aAlpha[2] == 0) ? 0xff000000 : 0x0) + rgb2;
        RGBA_Pal[3] = ((aAlpha[3] == 0) ? 0xff000000 : 0x0) + rgb3;
    }
    else
    {
        RGBA_Pal[0] =
            ((aAlpha[0] ==
              0) ? 0xff000000 : 0x0) +
            vobsubdata->vobsub->palette[aPalette[0]];
        RGBA_Pal[1] =
            ((aAlpha[1] ==
              0) ? 0xff000000 : 0x0) +
            vobsubdata->vobsub->palette[aPalette[1]];
        RGBA_Pal[2] =
            ((aAlpha[2] ==
              0) ? 0xff000000 : 0x0) +
            vobsubdata->vobsub->palette[aPalette[2]];
        RGBA_Pal[3] =
            ((aAlpha[3] ==
              0) ? 0xff000000 : 0x0) +
            vobsubdata->vobsub->palette[aPalette[3]];
    }
    int i, j;
    unsigned char a, b;
    unsigned char *sourcemodify;
    //    int fd =open("/sdcard/subfrom", O_WRONLY|O_CREAT);
    //    if (fd==NULL)
    //    {
    //      LOGE("fd ================NULL");
    //    }
    //    long bytes = write(fd, source,length );
    //    close(fd);
    //
    //  LOGE("write bytes %d  / %d ",bytes, length);
    //
    for (i = 0; i < length; i += 2)
    {
        int linenumber = i / bytesPerLine;
        if (linenumber & 1)
        {
            //sourcemodify=source+(720*576/8);
            sourcemodify = source + OSD_HALF_SIZE;
        }
        else
        {
            sourcemodify = source;
        }
        a = sourcemodify[(linenumber / 2) * bytesPerLine +
                         i % bytesPerLine];
        b = sourcemodify[(linenumber / 2) * bytesPerLine +
                         i % bytesPerLine + 1];
        j = i * 4;
        dist[j] = RGBA_Pal[(b & 0xc0) >> 6];
        dist[j + 1] = RGBA_Pal[(b & 0x30) >> 4];
        dist[j + 2] = RGBA_Pal[(b & 0x0c) >> 2];
        dist[j + 3] = RGBA_Pal[(b & 0x03)];
        dist[j + 4] = RGBA_Pal[(a & 0xc0) >> 6];
        dist[j + 5] = RGBA_Pal[(a & 0x30) >> 4];
        dist[j + 6] = RGBA_Pal[(a & 0x0c) >> 2];
        dist[j + 7] = RGBA_Pal[(a & 0x03)];
    }
    //    int fdto =open("/sdcard/subto", O_WRONLY|O_CREAT);
    //    if (fdto==NULL)
    //    {
    //      LOGE("fd ================NULL");
    //    }
    //    bytes = write(fdto, dist,length*16 );
    //    close(fdto);
    //
    //  LOGE("write bytes %d  / %d ",bytes, length*16);
}

//change data from 2bit to 32bit
void idxsub_parser_data(const unsigned char *source, long length, int linewidth,
                        unsigned int *dist, int subtitle_alpha)
{
    covert2bto32b(source, length, linewidth, dist, subtitle_alpha);
    return ;
}
