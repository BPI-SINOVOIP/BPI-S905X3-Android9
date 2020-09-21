
/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


/**************************************************
* example based on amcodec
**************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include "vcodec.h"

//#define DEBUG_WITH_BLOCK

#define READ_SIZE (64 * 1024)
#define EXTERNAL_PTS    (1)
#define SYNC_OUTSIDE    (2)
#define UNIT_FREQ       96000
#define PTS_FREQ        90000
#define AV_SYNC_THRESH    PTS_FREQ*30

static vcodec_para_t v_codec_para;
static vcodec_para_t *pcodec, *vpcodec;
static char *filename;
FILE* fp = NULL;
static int axis[8] = {0};

int osd_blank(char *path, int cmd)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);

    if (fd >= 0) {
        sprintf(bcmd, "%d", cmd);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int set_tsync_enable(int enable)
{
    int fd;
    char *path = "/sys/class/tsync/enable";
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", enable);
        write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

int parse_para(const char *para, int para_num, int *result)
{
    char *endp;
    const char *startp = para;
    int *out = result;
    int len = 0, count = 0;

    if (!startp) {
        return 0;
    }

    len = strlen(startp);

    do {
        //filter space out
        while (startp && (isspace(*startp) || !isgraph(*startp)) && len) {
            startp++;
            len--;
        }

        if (len == 0) {
            break;
        }

        *out++ = strtol(startp, &endp, 0);

        len -= endp - startp;
        startp = endp;
        count++;

    } while ((endp) && (count < para_num) && (len > 0));

    return count;
}

int set_display_axis(int recovery)
{
    int fd;
    char *path = "/sys/class/display/axis";
    char str[128];
    int count;

    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        if (!recovery) {
            read(fd, str, 128);
            printf("read axis %s, length %zu\n", str, strlen(str));
            count = parse_para(str, 8, axis);
        }
        if (recovery) {
            sprintf(str, "%d %d %d %d %d %d %d %d",
                    axis[0], axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        } else {
            sprintf(str, "2048 %d %d %d %d %d %d %d",
                    axis[1], axis[2], axis[3], axis[4], axis[5], axis[6], axis[7]);
        }
        write(fd, str, strlen(str));
        close(fd);
        return 0;
    }

    return -1;
}

static void signal_handler(int signum)
{
    printf("Get signum=%x\n", signum);
    vcodec_close(vpcodec);
    fclose(fp);
    set_display_axis(1);
    osd_blank("/sys/class/graphics/fb0/blank", 0);
    osd_blank("/sys/class/graphics/fb0/osd_display_debug", 0);
    signal(signum, SIG_DFL);
    raise(signum);
}



int send_buffer_to_device(char *buffer, int Readlen)
{
    int isize = 0;
    int ret;

    do {
        ret = vcodec_write(pcodec, (buffer + isize), (Readlen - isize));
        if (ret < 0) {
            if (errno != EAGAIN) {
                printf("write data failed, errno %d\n", errno);
                return -1;
            } else {
                continue;
            }
        } else {
            isize += ret;
            //printf("write %d, cur isize %d\n", ret, isize);
        }
        //printf("ret %d, isize %d\n", ret, isize);
    } while (isize < Readlen);

    return 0;
}

/***************** ivf parser *******************/
#define MAX_SIZE 0x200000

/*!\brief OBU types. */
typedef enum ATTRIBUTE_PACKED {
    OBU_SEQUENCE_HEADER = 1,
    OBU_TEMPORAL_DELIMITER = 2,
    OBU_FRAME_HEADER = 3,
    OBU_TILE_GROUP = 4,
    OBU_METADATA = 5,
    OBU_FRAME = 6,
    OBU_REDUNDANT_FRAME_HEADER = 7,
    OBU_TILE_LIST = 8,
    OBU_PADDING = 15,
} OBU_TYPE;

/*!\brief OBU metadata types. */
typedef enum {
    OBU_METADATA_TYPE_RESERVED_0 = 0,
    OBU_METADATA_TYPE_HDR_CLL = 1,
    OBU_METADATA_TYPE_HDR_MDCV = 2,
    OBU_METADATA_TYPE_SCALABILITY = 3,
    OBU_METADATA_TYPE_ITUT_T35 = 4,
    OBU_METADATA_TYPE_TIMECODE = 5,
} OBU_METADATA_TYPE;

typedef struct {
    size_t size;  // Size (1 or 2 bytes) of the OBU header (including the
                  // optional OBU extension header) in the bitstream.
    OBU_TYPE type;
    int has_size_field;
    int has_extension;
    // The following fields come from the OBU extension header and therefore are
    // only used if has_extension is true.
    int temporal_layer_id;
    int spatial_layer_id;
} ObuHeader;

static const size_t kMaximumLeb128Size = 8;
static const uint8_t kLeb128ByteMask = 0x7f;  // Binary: 01111111

// Disallow values larger than 32-bits to ensure consistent behavior on 32 and
// 64 bit targets: value is typically used to determine buffer allocation size
// when decoded.
static const uint64_t kMaximumLeb128Value = UINT32_MAX;

size_t uleb_size_in_bytes(uint64_t value) {
    size_t size = 0;
    do {
        ++size;
    } while ((value >>= 7) != 0);
    return size;
}

int uleb_decode(const uint8_t *buffer, size_t available, uint64_t *value,
    size_t *length) {
    if (buffer && value) {
        *value = 0;
        for (size_t i = 0; i < kMaximumLeb128Size && i < available; ++i) {
            const uint8_t decoded_byte = *(buffer + i) & kLeb128ByteMask;
            *value |= ((uint64_t)decoded_byte) << (i * 7);
            if ((*(buffer + i) >> 7) == 0) {
                if (length) {
                    *length = i + 1;
                }

                // Fail on values larger than 32-bits to ensure consistent behavior on
                // 32 and 64 bit targets: value is typically used to determine buffer
                // allocation size.
                if (*value > UINT32_MAX) return -1;

                return 0;
            }
        }
    }

    // If we get here, either the buffer/value pointers were invalid,
    // or we ran over the available space
    return -1;
}

int uleb_encode(uint64_t value, size_t available, uint8_t *coded_value,
    size_t *coded_size) {
    const size_t leb_size = uleb_size_in_bytes(value);
    if (value > kMaximumLeb128Value || leb_size > kMaximumLeb128Size ||
        leb_size > available || !coded_value || !coded_size) {
        return -1;
    }

    for (size_t i = 0; i < leb_size; ++i) {
        uint8_t byte = value & 0x7f;
        value >>= 7;

        if (value != 0) byte |= 0x80;  // Signal that more bytes follow.

        *(coded_value + i) = byte;
    }

    *coded_size = leb_size;
    return 0;
}

int uleb_encode_fixed_size(uint64_t value, size_t available,
    size_t pad_to_size, uint8_t *coded_value,
    size_t *coded_size) {
    if (value > kMaximumLeb128Value || !coded_value || !coded_size ||
        available < pad_to_size || pad_to_size > kMaximumLeb128Size) {
        return -1;
    }
    const uint64_t limit = 1ULL << (7 * pad_to_size);
    if (value >= limit) {
        // Can't encode 'value' within 'pad_to_size' bytes
        return -1;
    }

    for (size_t i = 0; i < pad_to_size; ++i) {
        uint8_t byte = value & 0x7f;
        value >>= 7;

        if (i < pad_to_size - 1) byte |= 0x80;  // Signal that more bytes follow.

        *(coded_value + i) = byte;
    }

    *coded_size = pad_to_size;
    return 0;
}

// Returns 1 when OBU type is valid, and 0 otherwise.
static int valid_obu_type(int obu_type) {
  int valid_type = 0;
  switch (obu_type) {
    case OBU_SEQUENCE_HEADER:
    case OBU_TEMPORAL_DELIMITER:
    case OBU_FRAME_HEADER:
    case OBU_TILE_GROUP:
    case OBU_METADATA:
    case OBU_FRAME:
    case OBU_REDUNDANT_FRAME_HEADER:
    case OBU_TILE_LIST:
    case OBU_PADDING: valid_type = 1; break;
    default: break;
  }
  return valid_type;
}

char obu_type_name[16][32] = {
    "UNKNOWN",
    "OBU_SEQUENCE_HEADER",
    "OBU_TEMPORAL_DELIMITER",
    "OBU_FRAME_HEADER",
    "OBU_TILE_GROUP",
    "OBU_METADATA",
    "OBU_FRAME",
    "OBU_REDUNDANT_FRAME_HEADER",
    "OBU_TILE_LIST",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "OBU_PADDING"
};

char meta_type_name[6][32] = {
    "OBU_METADATA_TYPE_RESERVED_0",
    "OBU_METADATA_TYPE_HDR_CLL",
    "OBU_METADATA_TYPE_HDR_MDCV",
    "OBU_METADATA_TYPE_SCALABILITY",
    "OBU_METADATA_TYPE_ITUT_T35",
    "OBU_METADATA_TYPE_TIMECODE"
};

struct read_bit_buffer {
    const uint8_t *bit_buffer;
    const uint8_t *bit_buffer_end;
    uint32_t bit_offset;
};

typedef struct DataBuffer {
    const uint8_t *data;
    size_t size;
} DataBuffer;

static int rb_read_bit(struct read_bit_buffer *rb) {
    const uint32_t off = rb->bit_offset;
    const uint32_t p = off >> 3;
    const int q = 7 - (int)(off & 0x7);
    if (rb->bit_buffer + p < rb->bit_buffer_end) {
        const int bit = (rb->bit_buffer[p] >> q) & 1;
        rb->bit_offset = off + 1;
        return bit;
    }
    else {
        return 0;
    }
}

static int rb_read_literal(struct read_bit_buffer *rb, int bits) {
    int value = 0, bit;
    for (bit = bits - 1; bit >= 0; bit--) value |= rb_read_bit(rb) << bit;
    return value;
}

static int read_obu_size(const uint8_t *data,
                                     size_t bytes_available,
                                     size_t *const obu_size,
                                     size_t *const length_field_size) {
  uint64_t u_obu_size = 0;
  if (uleb_decode(data, bytes_available, &u_obu_size, length_field_size) !=
      0) {
    return -1;
  }

  if (u_obu_size > UINT32_MAX) return -1;
  *obu_size = (size_t)u_obu_size;
  return 0;
}

// Parses OBU header and stores values in 'header'.
static int read_obu_header(struct read_bit_buffer *rb,
                                       int is_annexb, ObuHeader *header) {
  if (!rb || !header) return -1;

  const ptrdiff_t bit_buffer_byte_length = rb->bit_buffer_end - rb->bit_buffer;
  if (bit_buffer_byte_length < 1) return -1;

  header->size = 1;

  if (rb_read_bit(rb) != 0) {
    // Forbidden bit. Must not be set.
    return -1;
  }

  header->type = (OBU_TYPE)rb_read_literal(rb, 4);

  if (!valid_obu_type(header->type))
    return -1;

  header->has_extension = rb_read_bit(rb);
  header->has_size_field = rb_read_bit(rb);

  if (!header->has_size_field && !is_annexb) {
    // section 5 obu streams must have obu_size field set.
    return -1;
  }

  if (rb_read_bit(rb) != 0) {
    // obu_reserved_1bit must be set to 0.
    return -1;
  }

  if (header->has_extension) {
    if (bit_buffer_byte_length == 1) return -1;

    header->size += 1;
    header->temporal_layer_id = rb_read_literal(rb, 3);
    header->spatial_layer_id = rb_read_literal(rb, 2);
    if (rb_read_literal(rb, 3) != 0) {
      // extension_header_reserved_3bits must be set to 0.
      return -1;
    }
  }

  return 0;
}

int read_obu_header_and_size(const uint8_t *data,
                                             size_t bytes_available,
                                             int is_annexb,
                                             ObuHeader *obu_header,
                                             size_t *const payload_size,
                                             size_t *const bytes_read) {
  size_t length_field_size_obu = 0;
  size_t length_field_size_payload = 0;
  size_t obu_size = 0;
  int status = 0;

  if (is_annexb) {
    // Size field comes before the OBU header, and includes the OBU header
    status =
        read_obu_size(data, bytes_available, &obu_size, &length_field_size_obu);

    if (status != 0) return status;
  }

  struct read_bit_buffer rb = { data + length_field_size_obu,
                                    data + bytes_available, 0};

  status = read_obu_header(&rb, is_annexb, obu_header);
  if (status != 0) return status;

  if (!obu_header->has_size_field) {
    // Derive the payload size from the data we've already read
    if (obu_size < obu_header->size) return -1;

    *payload_size = obu_size - obu_header->size;
  } else {
    // Size field comes after the OBU header, and is just the payload size
    status = read_obu_size(
        data + length_field_size_obu + obu_header->size,
        bytes_available - length_field_size_obu - obu_header->size,
        payload_size, &length_field_size_payload);
    if (status != 0) return status;
  }

  *bytes_read =
      length_field_size_obu + obu_header->size + length_field_size_payload;
  return 0;
}

int parser_frame(
    int is_annexb,
    uint8_t *data,
    const uint8_t *data_end,
    uint8_t *dst_data,
    uint32_t *frame_len,
    uint8_t *meta_buf,
    uint32_t *meta_len) {
    int frame_decoding_finished = 0;
    uint32_t obu_size = 0;
    ObuHeader obu_header;
    memset(&obu_header, 0, sizeof(obu_header));
    int seen_frame_header = 0;
    int next_start_tile = 0;
    DataBuffer obu_size_hdr;
    uint8_t header[20] = {
        0x00, 0x00, 0x01, 0x54,
        0xFF, 0xFF, 0xFE, 0xAB,
        0x00, 0x00, 0x00, 0x01,
        0x41, 0x4D, 0x4C, 0x56,
        0xD0, 0x82, 0x80, 0x00
    };
    uint8_t *p = NULL;
    uint32_t rpu_size = 0;

    // decode frame as a series of OBUs
    while (!frame_decoding_finished) {
        //      struct read_bit_buffer rb;
        size_t payload_size = 0;
        size_t header_size = 0;
        size_t bytes_read = 0;
        size_t bytes_written = 0;
        const size_t bytes_available = data_end - data;
        unsigned int i;
        OBU_METADATA_TYPE meta_type;
        uint64_t type;

        if (bytes_available == 0 && !seen_frame_header) {
            break;
        }

        int status =
            read_obu_header_and_size(data, bytes_available, is_annexb,
                &obu_header, &payload_size, &bytes_read);

        if (status != 0) {
            return -1;
        }

        // Record obu size header information.
        obu_size_hdr.data = data + obu_header.size;
        obu_size_hdr.size = bytes_read - obu_header.size;

        // Note: read_obu_header_and_size() takes care of checking that this
        // doesn't cause 'data' to advance past 'data_end'.

        if ((size_t)(data_end - data - bytes_read) < payload_size) {
            return -1;
        }

        printf("\tobu %s len %d+%d\n", obu_type_name[obu_header.type], bytes_read, payload_size);

        obu_size = bytes_read + payload_size + 4;

        if (!is_annexb) {
            obu_size = bytes_read + payload_size + 4;
            header_size = 20;
            uleb_encode_fixed_size(obu_size, 4, 4, header + 16, &bytes_written);
        }
        else {
            obu_size = bytes_read + payload_size;
            header_size = 16;
        }
        header[0] = ((obu_size + 4) >> 24) & 0xff;
        header[1] = ((obu_size + 4) >> 16) & 0xff;
        header[2] = ((obu_size + 4) >> 8) & 0xff;
        header[3] = ((obu_size + 4) >> 0) & 0xff;
        header[4] = header[0] ^ 0xff;
        header[5] = header[1] ^ 0xff;
        header[6] = header[2] ^ 0xff;
        header[7] = header[3] ^ 0xff;
        memcpy(dst_data, header, header_size);
        dst_data += header_size;
        memcpy(dst_data, data, bytes_read + payload_size);
        dst_data += bytes_read + payload_size;

        data += bytes_read;
        *frame_len += 20 + bytes_read + payload_size;

        switch (obu_header.type) {
        case OBU_TEMPORAL_DELIMITER:
            seen_frame_header = 0;
            next_start_tile = 0;
            break;
        case OBU_SEQUENCE_HEADER:
            // The sequence header should not change in the middle of a frame.
            if (seen_frame_header) {
                return -1;
            }
            break;
        case OBU_FRAME_HEADER:
            if (data_end == data + payload_size) {
                frame_decoding_finished = 1;
            }
            else {
                seen_frame_header = 1;
            }
            break;
        case OBU_REDUNDANT_FRAME_HEADER:
        case OBU_FRAME:
            if (obu_header.type == OBU_REDUNDANT_FRAME_HEADER) {
                if (!seen_frame_header) {
                    return -1;
                }
            }
            else {
                // OBU_FRAME_HEADER or OBU_FRAME.
                if (seen_frame_header) {
                    return -1;
                }
            }
            if (obu_header.type == OBU_FRAME) {
                if (data_end == data + payload_size) {
                    frame_decoding_finished = 1;
                    seen_frame_header = 0;
                }
            }
            break;

        case OBU_TILE_GROUP:
            if (!seen_frame_header) {
                return -1;
            }
            if (data + payload_size == data_end)
                frame_decoding_finished = 1;
            if (frame_decoding_finished)
                seen_frame_header = 0;
            break;
        case OBU_METADATA:
            uleb_decode(data, 8, &type, &bytes_read);
            if (type < 6)
                meta_type = type;
            else
                meta_type = 0;
            p = data + bytes_read;
            printf("\tmeta type %s %d+%d\n", meta_type_name[type], bytes_read, payload_size - bytes_read);

            if (meta_type == OBU_METADATA_TYPE_ITUT_T35) {
#if 0 /* for dumping original obu payload */
                for (i = 0; i < payload_size - bytes_read; i++) {
                    printf("%02x ", p[i]);
                    if (i % 16 == 15) printf("\n");
                }
                if (i % 16 != 0) printf("\n");
#endif
                if ((p[0] == 0xb5) /* country code */
                    && ((p[1] == 0x00) && (p[2] == 0x3b)) /* terminal_provider_code */
                    && ((p[3] == 0x00) && (p[4] == 0x00) && (p[5] == 0x08) && (p[6] == 0x00))) { /* terminal_provider_oriented_code */
                    printf("\t\tdolbyvison rpu\n");
                    meta_buf[0] = meta_buf[1] = meta_buf[2] = 0;
                    meta_buf[3] = 0x01;    meta_buf[4] = 0x19;

                    if (p[11] & 0x10) {
                        rpu_size = 0x100;
                        rpu_size |= (p[11] & 0x0f) << 4;
                        rpu_size |= (p[12] >> 4) & 0x0f;
                        if (p[12] & 0x08) {
                            printf("\tmeta rpu in obu exceed 512 bytes\n");
                            break;
                        }
                        for (i = 0; i < rpu_size; i++) {
                            meta_buf[5 + i] = (p[12 + i] & 0x07) << 5;
                            meta_buf[5 + i] |= (p[13 + i] >> 3) & 0x1f;
                        }
                        rpu_size += 5;
                    }
                    else {
                        rpu_size = (p[10] & 0x1f) << 3;
                        rpu_size |= (p[11] >> 5) & 0x07;
                        for (i = 0; i < rpu_size; i++) {
                            meta_buf[5 + i] = (p[11 + i] & 0x0f) << 4;
                            meta_buf[5 + i] |= (p[12 + i] >> 4) & 0x0f;
                        }
                        rpu_size += 5;
                    }
                    *meta_len = rpu_size;
                }
            }
            else if (meta_type == OBU_METADATA_TYPE_HDR_CLL) {
                printf("\t\thdr10 cll:\n");
                printf("\t\tmax_cll = %x\n", (p[0] << 8) | p[1]);
                printf("\t\tmax_fall = %x\n", (p[2] << 8) | p[3]);
            }
            else if (meta_type == OBU_METADATA_TYPE_HDR_MDCV) {
                printf("\t\thdr10 primaries[r,g,b] = \n");
                for (i = 0; i < 3; i++) {
                    printf("\t\t %x, %x\n",
                        (p[i * 4] << 8) | p[i * 4 + 1],
                        (p[i * 4 + 2] << 8) | p[i * 4 + 3]);
                }
                printf("\t\twhite point = %x, %x\n", (p[12] << 8) | p[13], (p[14] << 8) | p[15]);
                printf("\t\tmaxl = %x\n", (p[16] << 24) | (p[17] << 16) | (p[18] << 8) | p[19]);
                printf("\t\tminl = %x\n", (p[20] << 24) | (p[21] << 16) | (p[22] << 8) | p[23]);
            }
                break;
        case OBU_TILE_LIST:
            break;
        case OBU_PADDING:
            break;
        default:
            // Skip unrecognized OBUs
            break;
        }

        data += payload_size;
    }

    return 0;
}


bool is_video_file_type_ivf(FILE *fp, int video_type, char *buffer)
{
    if (fp && video_type == VFORMAT_AV1) {
        fread(buffer, 1, 4, fp);
        fseek(fp, 0, SEEK_SET);
        if ((buffer[0] == 0x44) &&
            (buffer[1] == 0x4B) &&
            (buffer[2] == 0x49) &&
            (buffer[3] == 0x46))
            return true;
    }
    return false;
}

int ivf_write_dat(FILE *src_fp, uint8_t *src_buffer)
{
    int frame_count = 0;
    unsigned int src_frame_size = 0;
    unsigned int dst_frame_size = 0;
    unsigned int meta_size = 0;
    uint8_t *dst_buffer = NULL;
    uint8_t *meta_buffer = NULL;
    int process_count = -1;
    unsigned int *p_size;

    meta_buffer = calloc(1, 1024);
    if (!meta_buffer) {
        printf("fail to alloc meta buf\n");
        return -1;
    }
    if (fread(src_buffer, 1, 32, src_fp) != 32) {
        printf("read input file error!\n");
        return -1;
    }
    p_size = (unsigned int *)(src_buffer + 24);
    process_count = *p_size;
    printf("frame number = %d\n", process_count);
    while (frame_count < process_count) {
        if (fread(src_buffer, 1, 12, src_fp) != 12) {
            printf("end of file!\n");
            break;
        }
        p_size = (unsigned int *)src_buffer;
        src_frame_size = *p_size;
        printf("frame %d, size %d\n", frame_count, src_frame_size);

        if (fread(src_buffer, 1, src_frame_size, src_fp) != src_frame_size) {
            printf("read input file error %d!\n", src_frame_size);
            break;
        }

        dst_buffer = calloc(1, src_frame_size + 4096);
        if (!dst_buffer) {
            printf("failed to alloc frame buf\n");
            break;
        }
        dst_frame_size = 0;
        meta_size = 0;

        parser_frame(0, src_buffer, src_buffer + src_frame_size, dst_buffer, &dst_frame_size, meta_buffer, &meta_size);
        if (dst_frame_size) {
            printf("\toutput len=%d\n", dst_frame_size);
            if (send_buffer_to_device((char *)dst_buffer, dst_frame_size) < 0) {
                free(dst_buffer);
                break;
            }
        }
        if (meta_size) {
            printf("\tmeta len=%d\n", meta_size);
            /* dump meta here */
        }
        free(dst_buffer);
        frame_count++;
    }
    printf("Process %d frame\n", frame_count);
    free(meta_buffer);
    return 0;
}

/****************** end of ivf parer *************/

#if 1
//AV1
static int obu_frame_frame_head_come_after_tile = 0;
//static int frame_decoded = 0;
static int decoding_data_flag = 0;
#endif

unsigned char is_picture_start(int video_type, int nal_unit_type)
{
    unsigned char ret = 0;
    if (video_type == VFORMAT_AVS) {
        if (nal_unit_type == 0xB3 || //I_PICTURE_START_CODE
                nal_unit_type == 0xB6  //PB_PICTURE_START_CODE
           )
            ret = 1;
    } else if (video_type == VFORMAT_VP9) {
        /*to do*/
        goto check_av1;
    } else if (video_type == VFORMAT_AV1) {
check_av1:
        if (nal_unit_type == OBU_FRAME_HEADER ||
                nal_unit_type == OBU_FRAME) {
            ret = 1;
            obu_frame_frame_head_come_after_tile = 1;
            if (nal_unit_type == OBU_FRAME) { /*have tile group in this OBU*/
                obu_frame_frame_head_come_after_tile = 0;
                decoding_data_flag = 1;
            }

        }
    }
    else
        ret = 1;
    return ret;
}

unsigned char is_picture_end(int video_type, int nal_unit_type)
{
    unsigned char ret = 0;
    if (video_type == VFORMAT_AVS) {
        if (nal_unit_type == 0xB3 || //I_PICTURE_START_CODE
                nal_unit_type == 0xB6 || //PB_PICTURE_START_CODE
                nal_unit_type == 0xB0 || //SEQUENCE_HEADER_CODE
                nal_unit_type == 0xB1  //SEQUENCE_END_CODE
           )
            ret = 1;
    } else if (video_type == VFORMAT_VP9) {
        /*to do*/
        goto check_av1;
    } else if (video_type == VFORMAT_AV1) {
check_av1:
        if (nal_unit_type == OBU_TILE_GROUP) {
            obu_frame_frame_head_come_after_tile = 0;
            decoding_data_flag = 1;
        }
        else if (nal_unit_type == OBU_FRAME_HEADER ||
                nal_unit_type == OBU_FRAME) {
            if (decoding_data_flag)
                ret = 1;
            decoding_data_flag = 0;
            obu_frame_frame_head_come_after_tile = 1;
            if (nal_unit_type == OBU_FRAME) { /*have tile group in this OBU*/
                obu_frame_frame_head_come_after_tile = 0;
                decoding_data_flag = 1;
            }
        } else if (nal_unit_type == OBU_REDUNDANT_FRAME_HEADER &&
                obu_frame_frame_head_come_after_tile == 0) {
            decoding_data_flag = 0;
            printf("Warning, OBU_REDUNDANT_FRAME_HEADER come without OBU_FRAME or OBU_FRAME_HEAD\n");
        }
    }

    else
        ret = 1;
    return ret;
}

typedef struct
{
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    unsigned max_size;            //! Nal Unit Buffer size
    int forbidden_bit;            //! should be always FALSE
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx
    int nal_unit_type;            //! NALU_TYPE_xxxx
    unsigned char *buf;                    //! contains the first byte followed by the EBSP
    unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;

NALU_t *AllocNALU(int buffersize)
{
    NALU_t *n;

    if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)
    {
        printf("AllocNALU: n");
        exit(0);
    }

    n->max_size=buffersize;

    if ((n->buf = (unsigned char*)calloc (buffersize, sizeof (char))) == NULL)
    {
        free (n);
        printf ("AllocNALU: n->buf");
        exit(0);
    }

    return n;
}

void FreeNALU(NALU_t *n)
{
    if (n)
    {
        if (n->buf)
        {
            free(n->buf);
            n->buf=NULL;
        }
        free (n);
    }
}

static int FindStartCode2 (unsigned char *Buf)
{
    if ((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 1)) return 0; //0x000001
    else return 1;
}

static int FindStartCode3 (unsigned char *Buf)
{
    if ((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 0) || (Buf[3] != 1)) return 0;//0x00000001
    else return 1;
}

static int FindAmlStartCode(unsigned char *Buf)
{
    unsigned int len;
    unsigned int len2;
    len = (Buf[0] << 24) | (Buf[1] << 16) | (Buf[2] << 8) | (Buf[3] << 0);
    len2 = (Buf[4] << 24) | (Buf[5] << 16) | (Buf[6] << 8) | (Buf[7] << 0);
    if (((len + len2) == 0xffffffff) &&
            (Buf[8] == 0 && Buf[9] == 0 && Buf[10] == 0 && Buf[11] == 1) &&
            (Buf[12] == 0x41 && Buf[13] == 0x4d && Buf[14] == 0x4c && Buf[15] == 0x56)) {
        return 1;
    }
    return 0;
}

int GetAnnexbNALU (FILE* fe, NALU_t *nalu, int format)
{
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;
    int info2 = 0, info3 = 0;
    int i;
    int prefix_len = 4;
    if (format == VFORMAT_VP9 || format == VFORMAT_AV1)
        prefix_len = 16;
    if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL)
        printf ("GetAnnexbNALU: Could not allocate Buf memory\n");

    nalu->startcodeprefix_len=0;
    while (!feof(fe)) {
        if (nalu->startcodeprefix_len<prefix_len) {
            Buf[nalu->startcodeprefix_len++] = fgetc(fe);
        }
        else{
            if (prefix_len == 4) {
                for (i = 0; i < 3; i++)
                    Buf[i] = Buf[i+1];
            }
            else {
                for (i = 0; i < prefix_len; i++)
                    Buf[i] = Buf[i+1];
            }
            Buf[nalu->startcodeprefix_len - 1] = fgetc(fe);
        }
        if (prefix_len == 4) {
            if (nalu->startcodeprefix_len >= 3) {
                if (FindStartCode2(Buf)) {
                    pos = nalu->startcodeprefix_len;
                    nalu->startcodeprefix_len = 3;
                    break;
                }
            }
            if (nalu->startcodeprefix_len == 4) {
                if (FindStartCode3(Buf)) {
                    pos = nalu->startcodeprefix_len;
                    break;
                }
            }
        } else {
            if (nalu->startcodeprefix_len == prefix_len) {
                if (FindAmlStartCode(Buf)) {
                    pos = nalu->startcodeprefix_len;
                    break;
                }
            }
        }
    }
    StartCodeFound = 0;
    info2 = 0;
    info3 = 0;

    while (!StartCodeFound)
    {
        if (feof (fe))
        {
            rewind = -1;
            goto fill_data;
        }
        Buf[pos++] = fgetc (fe);
        if (prefix_len == 4) {
            info3 = FindStartCode3(&Buf[pos - 4]);
            if (info3 != 1)
                info2 = FindStartCode2(&Buf[pos - 3]);
            StartCodeFound = (info2 == 1 || info3 == 1);
        } else {
            StartCodeFound = FindAmlStartCode(&Buf[pos-prefix_len]);
        }
    }

    // Here, we have found another start code (and read length of startcode bytes more than we should
    // have.  Hence, go back in the file
    if (prefix_len == 4)
        rewind = (info3 == 1) ? -4 : -3;
    else
        rewind = -prefix_len;

    if (0 != fseek (fe, rewind, SEEK_CUR))
    {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }

    // Here the Start code, the complete NALU, and the next start code is in the Buf.
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code
fill_data:
    nalu->len = (pos + rewind);
    if (nalu->len > MAX_SIZE) {
        printf("%d: Error: to many data to copy %d\n", __LINE__, nalu->len);
        exit(0);
    }
    memcpy (nalu->buf, &Buf[0], nalu->len);//copy nalu, include 0x000001 or 0x00000001
    if (format == VFORMAT_VP9 || format == VFORMAT_AV1) {
        unsigned char* p = &nalu->buf[nalu->startcodeprefix_len];
        while ((*p++)&0x80) {

        }
        nalu->nal_reference_idc = 0;
        nalu->forbidden_bit = *p & 0x80;
        nalu->nal_unit_type = (*p>>3)&0xf;
    } else {
        nalu->forbidden_bit = nalu->buf[nalu->startcodeprefix_len] & 0x80; //1 bit
        nalu->nal_reference_idc = nalu->buf[nalu->startcodeprefix_len] & 0x60; // 2 bit
        //nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
        nalu->nal_unit_type = (nalu->buf[nalu->startcodeprefix_len]);
    }
    free(Buf);

    return (pos + rewind);
}

static void dump(NALU_t *n)
{
    if (!n)return;
    /*
       printf("a new nal:");

       printf(" len: %d  ", n->len);
       printf("nal_unit_type: %x\n", n->nal_unit_type);
       */

}

#define BUFFER_SIZE (1024*1024*2)

int av1_frame_mode_write_dat(FILE *fp, vcodec_para_t *vpcodec, char *buffer)
{
    NALU_t *n = AllocNALU(MAX_SIZE);
    int buf_pos = 0;


    //unsigned char pic_head_found = 0;
    while (!feof(fp))
    {
        GetAnnexbNALU(fp, n, vpcodec->video_type);
        dump(n);
        if ((buf_pos + n->len) > BUFFER_SIZE) {
            printf("%d: Error: to many data to copy %d\n", __LINE__, buf_pos + n->len);
            exit(0);
        }
        memcpy(&buffer[buf_pos], n->buf, n->len);
        buf_pos += n->len;
        if (is_picture_start(vpcodec->video_type, n->nal_unit_type)) {
            break;
        }
    }

    while (!feof(fp))
    {
        GetAnnexbNALU(fp, n, vpcodec->video_type);
        if (is_picture_end(vpcodec->video_type, n->nal_unit_type)) {
            //printf("Send Data Size =%d\n", buf_pos);
#ifndef TEST_ON_PC
            /*
               int delay_time = 0;
               if (delay_time > 0)
               delay(delay_time);
               */
            if (send_buffer_to_device(buffer, buf_pos) < 0)
                goto error_ret;
#endif
            buf_pos = 0;
        }

        if ((buf_pos + n->len) > BUFFER_SIZE) {
            printf("%d: Error: to many data to copy %d\n", __LINE__, buf_pos + n->len);
            goto error_ret;
        }

        memcpy(&buffer[buf_pos], n->buf, n->len);
        buf_pos += n->len;
        dump(n);
    }
error_ret:
    free(n);
    return 0;
}

enum {
    ESPLAYER,
    ES_FILE_NAME,
    ES_WIDTH,
    ES_HEGIHT,
    ES_FPS,
    ES_MODE,    /*single, stream, frame*/
    ES_FORMAT,
    ES_SUB_FORMAT,
    ES_MAX_OPT,
} OPT_ARGS_INDEX;

int main(int argc, char *argv[])
{
    char *buffer = NULL;

    int ret = CODEC_ERROR_NONE;
    //char buffer[READ_SIZE];

    uint32_t Readlen;
    uint32_t isize;
    struct buf_status vbuf;
    int end;
    int cnt = 0;
    uint32_t last_rp = 1;

    if (argc < 7) {
        printf("Corret command: esplayer <filename> <width> <height> <fps> <mode> <format(1:mpeg4 2:h264 6:vc1)> [subformat for mpeg4/vc1]\n");
        printf("<mode> :(No stream mode will be forced to single mode)\n");
        printf("\t0   --single mode\n");
        printf("\t1   --stream mode\n");
        printf("\t2   --frame mode(av1)\n");
        return -1;
    }
    osd_blank("/sys/class/graphics/fb0/osd_display_debug", 1);
    osd_blank("/sys/class/graphics/fb0/blank", 1);
    osd_blank("/sys/class/video/disable_video", 2);
    osd_blank("/sys/class/video/video_global_output", 1);
    set_display_axis(0);

    buffer = malloc(BUFFER_SIZE);
    if (NULL == buffer) {
        printf("malloc write buffer fail\n");
        return -1;
    }
    memset(buffer, 0, BUFFER_SIZE);

    vpcodec = &v_codec_para;
    memset(vpcodec, 0, sizeof(vcodec_para_t));
    vpcodec->has_video = 1;
    vpcodec->has_audio = 0;
    vpcodec->noblock = 0;

    vpcodec->mode = atoi(argv[ES_MODE]);
    vpcodec->video_type = atoi(argv[ES_FORMAT]);
    if ((vpcodec->mode == FRAME_MODE) &&
        (vpcodec->video_type != VFORMAT_AV1)) {
        vpcodec->mode = STREAM_MODE;
    }
    printf("play with mode %s\n", (vpcodec->mode != 0)?
            ((vpcodec->mode == 1)?"STREAM_MODE":"FRAME_MODE"):"SINGLE_MODE");

    if (vpcodec->video_type == VFORMAT_H264) {
        vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_H264;
        vpcodec->am_sysinfo.param = (void *)(EXTERNAL_PTS | SYNC_OUTSIDE);
    } else if (vpcodec->video_type == VFORMAT_VC1) {
        if (argc < ES_MAX_OPT) {
            printf("No subformat for vc1, take the default VIDEO_DEC_FORMAT_WVC1\n");
            vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_WVC1;
        } else {
            vpcodec->am_sysinfo.format = atoi(argv[ES_SUB_FORMAT]);
        }
    } else if (vpcodec->video_type == VFORMAT_MPEG4) {
        if (argc < ES_MAX_OPT) {
            printf("No subformat for mpeg4, take the default VIDEO_DEC_FORMAT_MPEG4_5\n");
            vpcodec->am_sysinfo.format = VIDEO_DEC_FORMAT_MPEG4_5;
        } else {
            vpcodec->am_sysinfo.format = atoi(argv[ES_SUB_FORMAT]);
        }
    }

    vpcodec->stream_type = STREAM_TYPE_ES_VIDEO;
    vpcodec->am_sysinfo.rate =
        96000 / atoi(argv[ES_FPS]);
    vpcodec->am_sysinfo.height =
        atoi(argv[ES_HEGIHT]);
    vpcodec->am_sysinfo.width =
        atoi(argv[ES_WIDTH]);

    printf("\n*********CODEC PLAYER DEMO************\n\n");

    filename = argv[ES_FILE_NAME];
    printf("file %s to be played\n", filename);

    if ((fp = fopen(filename, "rb")) == NULL) {
        printf("open file error!\n");
        goto osd_restore;
    }

    ret = vcodec_init(vpcodec);
    if (ret != CODEC_ERROR_NONE) {
        fclose(fp);
        printf("codec init failed, ret=-0x%x", -ret);
        goto osd_restore;
    }
    printf("video codec ok!\n");

    set_tsync_enable(0);

    pcodec = vpcodec;
    end = 0;

    if (is_video_file_type_ivf(fp, vpcodec->video_type, buffer)) {
        printf("input video file is ivf with av1.\n");
        ivf_write_dat(fp, (uint8_t *)buffer);
    } else if (vpcodec->mode == FRAME_MODE) {
        av1_frame_mode_write_dat(fp, vpcodec, buffer);
    } else {
        while (1) {
            if (!end) {
                Readlen = fread(buffer, 1, READ_SIZE, fp);
                //printf("Readlen %d\n", Readlen);
                if (Readlen <= 0) {
                    printf("read file error!\n");
                    rewind(fp);
                }
            } else
                Readlen = 0;

            isize = 0;
            if (end) {
                memset(buffer, 0 ,READ_SIZE);
                Readlen = READ_SIZE - 10;
            }
            cnt = 0;
            do {
                ret = vcodec_write(pcodec, buffer + isize, Readlen);
                if (ret <= 0) {
                    if (errno != EAGAIN) {
                        printf("write data failed, errno %d\n", errno);
                        goto error;
                    } else {
                        usleep(10);
                        if (++cnt > 2000000) {
                            end = 1;
                            break;
                        }
                        continue;
                    }
                } else {
                    isize += ret;
                    cnt = 0;
                }
                //printf("ret %d, isize %d\n", ret, isize);
            } while (isize < Readlen);
            if (end)
                break;
            if (feof(fp))
                end = 1;

            signal(SIGCHLD, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
            signal(SIGTTOU, SIG_IGN);
            signal(SIGTTIN, SIG_IGN);
            signal(SIGHUP, signal_handler);
            signal(SIGTERM, signal_handler);
            signal(SIGSEGV, signal_handler);
            signal(SIGINT, signal_handler);
            signal(SIGQUIT, signal_handler);
        }
    }

    cnt = 0;
    do {
        ret = vcodec_get_vbuf_state(pcodec, &vbuf);
        if (ret != 0) {
            printf("vcodec_get_vbuf_state error: %x\n", -ret);
            goto error;
        }
        if (last_rp != vbuf.read_pointer)
            cnt = 0;
        last_rp = vbuf.read_pointer;
        usleep(10000);
#ifndef DEBUG_WITH_BLOCK
        if (++cnt > 500)
            break;
#endif
    } while (vbuf.data_len > 0x100);

    printf("play end\n");
error:
    vcodec_close(vpcodec);
    fclose(fp);
osd_restore:
    set_display_axis(1);
    osd_blank("/sys/class/graphics/fb0/blank", 0);
    osd_blank("/sys/class/graphics/fb0/osd_display_debug", 0);
    free(buffer);
    return 0;
}

