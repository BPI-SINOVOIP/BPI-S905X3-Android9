/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef VOB_SUB_H
#define VOB_SUB_H

/**
 * @file VOB_sub.h
 * @addtogroup vob
 */
/*@{*/

//#define OSD_HALF_SIZE 720*480/8
#define OSD_HALF_SIZE 1920*1280/8

// Command enum
typedef enum
{
    FSTA_DSP = 0,
    STA_DSP = 1,
    STP_DSP = 2,
    SET_COLOR = 3,
    SET_CONTR = 4,
    SET_DAREA = 5,
    SET_DSPXA = 6,
    CHG_COLCON = 7,
    CMD_END = 0xFF,
} CommandID;

#define SUCCESS     1
#define FAIL        0

typedef struct _SP_DCSQT
{
    unsigned m_Delay;   // Delay befor execution
    //ushort        m_OffsetToThis;  Offset to start of this CDSQT
    //ushort        m_OffsetToNext;  Offset to next CDSQT. 0 if none
    //CommandsV m_Commands;      The commands;
} SP_DCSQT;

typedef struct _VOB_SUB_FRAME
{
    char id[4];
    unsigned char pts[4];
    unsigned short framelength;
    unsigned short subData;
    unsigned short cmdOffset;
} VOB_SUB_FRAME;

typedef struct _VOB_SPUVAR
{
    unsigned short spu_color;
    unsigned short spu_alpha;
    unsigned short spu_start_x;
    unsigned short spu_start_y;
    unsigned short spu_width;
    unsigned short spu_height;
    unsigned short top_pxd_addr;    // CHIP_T25
    unsigned short bottom_pxd_addr; // CHIP_T25

    unsigned mem_start; // CHIP_T25
    unsigned mem_end;   // CHIP_T25
    unsigned mem_size;  // CHIP_T25
    unsigned mem_rp;
    unsigned mem_wp;
    unsigned mem_rp2;
    unsigned char spu_cache[8];
    int spu_cache_pos;
    int spu_decoding_start_pos; //0~OSD_HALF_SIZE*2-1, start index to vob_pixData1[0~OSD_HALF_SIZE*2]

    unsigned disp_colcon_addr;  // CHIP_T25
    unsigned char display_pending;
    unsigned char displaying;
    unsigned char reser[2];
} VOB_SPUVAR;

typedef struct
{
    int left;
    int top;
    int width;
    int height;
    unsigned short colorcode;
    unsigned short contrast;
    unsigned long prtData;
    unsigned cls;
} Vob_subtitle_showdata;

extern VOB_SPUVAR uVobSPU;
extern SP_DCSQT TheDCSQT;
extern char *vob_pixData1;
extern unsigned short *vob_ptrPXDRead;
extern Vob_subtitle_showdata vob_subtitle_config;

extern int doVobSubCmd(VOB_SUB_FRAME *subFrame, unsigned short m_SubPicSize,
                       unsigned curAVtime);
extern unsigned char vob_FillPixel(int n);

typedef struct
{

    unsigned int pts100;    /* from idx */

    off_t filepos;

    //unsigned int size;

    //unsigned char *data;

} packet_t;

typedef struct
{

    char *id;

    packet_t *packets;

    unsigned int packets_reserve;

    unsigned int packets_size;

    unsigned int current_index;

} packet_queue_t;

typedef struct
{

    //FILE *file;

    int fd;

    unsigned char *data;

    unsigned long size;

    unsigned long pos;

} rar_stream_t;

typedef struct
{

    rar_stream_t *stream;

    unsigned int pts;

    int aid;

    unsigned char *packet;

    //unsigned int packet_reserve;

    unsigned int packet_size;

} mpeg_t;

typedef struct
{

    unsigned int palette[16];

    unsigned int cuspal[4];

    int delay;

    unsigned int custom;

    unsigned int have_palette;

    unsigned int orig_frame_width, orig_frame_height;

    unsigned int origin_x, origin_y;

    unsigned int forced_subs;

    /* index */

    packet_queue_t *spu_streams;

    unsigned int spu_streams_size;

    unsigned int spu_streams_current;

} vobsub_t;

typedef struct
{

    //    control_t* cntl;

    /* Add more members here */

    VOB_SPUVAR VobSPU;

    Vob_subtitle_showdata vob_subtitle_config;

    char *vob_pixData;

    unsigned short *vob_ptrPXDRead;

    vobsub_t *vobsub;

    mpeg_t *mpeg;

    char *cur_idx_url;

    int cur_track_id;

    unsigned duration;

    int cur_pts100;

    int cur_endpts100;

    int next_pts100;

    char next_filepos;

    /*end */

} subtitlevobsub_t;

extern int init_subtitle(char *fileurl);

extern subtitlevobsub_t *getIdxSubData(int ptms);

void idxsub_parser_data(const unsigned char *source, long length, int linewidth,
                        unsigned int *dist, int subtitle_alpha);
void idxsub_close_subtitle();
int idxsub_init_subtitle(char *fileurl, int index);

/*@}*/
#endif              /* VOB_SUB_H */
