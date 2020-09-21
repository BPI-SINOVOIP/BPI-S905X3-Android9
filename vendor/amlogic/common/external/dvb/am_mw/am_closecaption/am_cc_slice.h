/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _AM_CC_SLICE_H_
#define  _AM_CC_SLICE_H_

#include <am_types.h>

/* Helper functions. */

#define VBI_SLICED_NONE                 0
#define VBI_SLICED_UNKNOWN              0

#define VBI_SLICED_ANTIOPE              0x00002000
#define VBI_SLICED_TELETEXT_A           0x00002000

#define VBI_SLICED_TELETEXT_B_L10_625   0x00000001
#define VBI_SLICED_TELETEXT_B_L25_625   0x00000002
#define VBI_SLICED_TELETEXT_B           (VBI_SLICED_TELETEXT_B_L10_625 | \
                                         VBI_SLICED_TELETEXT_B_L25_625)
#define VBI_SLICED_TELETEXT_B_625       VBI_SLICED_TELETEXT_B

#define VBI_SLICED_TELETEXT_C_625       0x00004000
#define VBI_SLICED_TELETEXT_D_625       0x00008000

#define VBI_SLICED_VPS                  0x00000004
#define VBI_SLICED_VPS_F2               0x00001000

/**
 * Closed Caption for 625 line systems
 *
 * Note this is split into field one and two services since for basic
 * caption decoding only field one is required. vbi_sliced id can be
 * VBI_SLICED_CAPTION_625, _625_F1 or _625_F2 regardless of line number.
 *
 * Reference: <a href="http://global.ihs.com">EIA 608
 * "Recommended Practice for Line 21 Data Service"</a>.
 *
 * vbi_sliced payload: First and second byte including parity,
 * lsb first transmitted.

 * Wide Screen Signalling for 625 line systems
 *
 * Reference: <a href="http://www.etsi.org">EN 300 294
 * "625-line television Wide Screen Signalling (WSS)"</a>.
 *
 * vbi_sliced payload:
 * <pre>
 * Byte         0                  1
 *       msb         lsb  msb             lsb
 * bit   7 6 5 4 3 2 1 0  x x 13 12 11 10 9 8<br></pre>
 * according to EN 300 294, Table 1, lsb first transmitted.
 */
#define VBI_SLICED_CAPTION_625_F1       0x00000008
#define VBI_SLICED_CAPTION_625_F2       0x00000010
#define VBI_SLICED_CAPTION_625          (VBI_SLICED_CAPTION_625_F1 | \
                                         VBI_SLICED_CAPTION_625_F2)
#define VBI_SLICED_WSS_625              0x00000400

#define VBI_SLICED_CAPTION_525_F1       0x00000020
#define VBI_SLICED_CAPTION_525_F2       0x00000040
#define VBI_SLICED_CAPTION_525          (VBI_SLICED_CAPTION_525_F1 | \
                                         VBI_SLICED_CAPTION_525_F2)
#define VBI_SLICED_2xCAPTION_525        0x00000080
#define VBI_SLICED_TELETEXT_B_525       0x00010000

#define VBI_SLICED_NABTS                0x00000100
#define VBI_SLICED_TELETEXT_C_525       0x00000100
#define VBI_SLICED_TELETEXT_BD_525      0x00000200
#define VBI_SLICED_TELETEXT_D_525       0x00020000

#define VBI_SLICED_WSS_CPR1204          0x00000800

#define VBI_SLICED_VBI_625              0x20000000
#define VBI_SLICED_VBI_525              0x40000000

#ifndef CLEAR
#  define CLEAR(var) memset (&(var), 0, sizeof (var))
#endif

#ifndef N_ELEMENTS
#  define N_ELEMENTS(array) (sizeof (array) / sizeof (*(array)))
#endif


typedef struct AM_VBI_SLICED {
        uint32_t        id;
        uint32_t        line;
        uint8_t         data[56];
} AM_VBI_Sliced_t;

typedef struct AM_VBI_Slice_Stream AM_VBI_Slice_Stream_t ;

typedef AM_Bool_t AM_VBI_SLICE_CALLBACK_FN(const AM_VBI_Sliced_t *     sliced,
                                    unsigned int           n_lines,
                                    double      sample_time,
                                    int64_t     stream_time,
                                    void        *user_data);

typedef AM_Bool_t write_fn(AM_VBI_Slice_Stream_t * st,
                           const AM_VBI_Sliced_t *   sliced,
                           unsigned int           n_lines,
                           const uint8_t *        raw,
                           double                 sample_time,
                           int64_t                stream_time);

struct AM_VBI_Slice_Stream {
        uint8_t                 buffer[4096];
        uint8_t                 b64_buffer[4096];
        AM_VBI_Sliced_t         sliced[64];
        const uint8_t *         bp;
        const uint8_t *         end;

         AM_VBI_SLICE_CALLBACK_FN *callback;

        write_fn *              write_func;

        double                  sample_time;
        int64_t                 stream_time;
        unsigned int            interfaces;
        unsigned int            system;
        AM_Bool_t               read_not_pull;
        int                     fd;
        AM_Bool_t               close_fd;
        void                    *userdata ;
} ;


#endif     /* _AM_CC_SLICE_H_ */

