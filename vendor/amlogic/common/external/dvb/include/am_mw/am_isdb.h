/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _AM_ISDB_H
#define _AM_ISDB_H

#include "am_types.h"
#include "am_osd.h"
#ifdef __cplusplus
extern "C"
{
#endif
typedef void* AM_ISDB_Handle_t;
typedef int64_t LLONG;
#define RB16(x) (ntohs(*(unsigned short int*)(x)))
#define RB24(x) (  ((unsigned char*)(x))[0] << 16 | ((unsigned char*)(x))[1] << 8 | ((unsigned char*)(x))[2]  )
//#define RB24(x) (  ((unsigned char*)(x))[0]  | ((unsigned char*)(x))[1] << 8 | ((unsigned char*)(x))[2] << 16 )

typedef struct {
	char *po;
	int   left;
	int   has_prop;
	int flash;
} Output;

#define IS_HORIZONTAL_LAYOUT(format) \
	( (format) == WF_HORIZONTAL_STD_DENSITY\
	||(format) == WF_HORIZONTAL_HIGH_DENSITY\
	||(format) == WF_HORIZONTAL_WESTERN_LANG\
	||(format) == WF_HORIZONTAL_1920x1080\
	||(format) == WF_HORIZONTAL_960x540\
	||(format) == WF_HORIZONTAL_720x480\
	||(format) == WF_HORIZONTAL_1280x720\
	||(format) == WF_HORIZONTAL_CUSTOM )

enum csi_command
{
	/* GSM Character Deformation*/
	CSI_CMD_GSM = 0x42,
	/* Set Writing Format */
	CSI_CMD_SWF = 0x53,
	/* Composite Character Composition */
	CSI_CMD_CCC = 0x54,
	/* Set Display Format */
	CSI_CMD_SDF = 0x56,
	/* Character composition dot designation */
	CSI_CMD_SSM = 0x57,
	/* Set HOrizontal spacing */
	CSI_CMD_SHS = 0x58,
	/* Set Vertical Spacing */
	CSI_CMD_SVS = 0x59,
	/* Partially Line Down */
	CSI_CMD_PLD = 0x5B,
	/* Partially Line UP */
	CSI_CMD_PLU = 0x5C,
	/* Colouring block */
	CSI_CMD_GAA = 0x5D,
	/* Raster Colour Designation */
	CSI_CMD_SRC = 0x5E,
	/* Set Display Position */
	CSI_CMD_SDP = 0x5F,
	/* Active Coordinate Position Set */
	CSI_CMD_ACPS = 0x61,
	/* Switch control */
	CSI_CMD_TCC = 0x62,
	/* Ornament Control */
	CSI_CMD_ORN = 0x63,
	/* Font */
	CSI_CMD_MDF = 0x64,
	/* Character Font Set */
	CSI_CMD_CFS = 0x65,
	/* External Character Set */
	CSI_CMD_XCS = 0x66,
	/* Built-in sound replay */
	CSI_CMD_PRA = 0x68,
	/* Alternative Character Set */
	CSI_CMD_ACS = 0x69,
	/* Raster Color Command */
	CSI_CMD_RCS = 0x6E,
	/* Skip Character Set */
	CSI_CMD_SCS = 0x6F,
};

enum subdatatype
{
	CC_DATATYPE_GENERIC=0,
	CC_DATATYPE_DVB=1
};

enum ccx_encoding_type
{
	CCX_ENC_UNICODE = 0,
	CCX_ENC_LATIN_1 = 1,
	CCX_ENC_UTF_8 = 2,
	CCX_ENC_ASCII = 3
};



struct ISDBPos{
	int x,y;
};



enum isdb_CC_composition
{
	ISDB_CC_NONE = 0,
	ISDB_CC_AND  = 2,
	ISDB_CC_OR   = 3,
	ISDB_CC_XOR  = 4,
};

enum writing_format
{
    WF_HORIZONTAL_STD_DENSITY = 0,
    WF_VERTICAL_STD_DENSITY = 1,
    WF_HORIZONTAL_HIGH_DENSITY = 2,
    WF_VERTICAL_HIGH_DENSITY = 3,
    WF_HORIZONTAL_WESTERN_LANG = 4,
    WF_HORIZONTAL_1920x1080 = 5,
    WF_VERTICAL_1920x1080 = 6,
    WF_HORIZONTAL_960x540 = 7,
    WF_VERTICAL_960x540 = 8,
    WF_HORIZONTAL_720x480 = 9,
    WF_VERTICAL_720x480 = 10,
    WF_HORIZONTAL_1280x720 = 11,
    WF_VERTICAL_1280x720 = 12,
    WF_HORIZONTAL_CUSTOM = 100,
    WF_NONE,
};

enum subtype
{
	CC_BITMAP,
	CC_608,
	CC_708,
	CC_TEXT,
	CC_RAW,
};

enum isdb_tmd
{
	ISDB_TMD_FREE = 0,
	ISDB_TMD_REAL_TIME = 0x1,
	ISDB_TMD_OFFSET_TIME = 0x2,
};


typedef struct
{
	enum writing_format format;

	// clipping area.
	struct disp_area {
		int x, y;
		int w, h;
	} display_area;

	int font_size; // valid values: {16, 20, 24, 30, 36} (TR-B14/B15)

	struct fscale { // in [percent]
		int fscx, fscy;
	} font_scale; // 1/2x1/2, 1/2*1, 1*1, 1*2, 2*1, 2*2

	struct spacing {
		int col, row;
	} cell_spacing;

	struct ISDBPos cursor_pos;

	enum isdb_CC_composition ccc;
	int acps[2];

}ISDBSubLayout;


typedef struct {
    int auto_display; // bool. forced to be displayed w/o user interaction
    int rollup_mode;  // bool

    uint8_t need_init; // bool
    uint8_t clut_high_idx; // color = default_clut[high_idx << 4 | low_idx]

    uint32_t fg_color;
    uint32_t bg_color;
    /**
     * Colour between foreground and background in gradation font is defined that
     * colour near to foreground colour is half foreground colour and colour near to
     * background colour is half background colour.
     */
    //Half forground color
    uint32_t hfg_color;
    //Half background color
    uint32_t hbg_color;
    uint32_t mat_color;
    uint32_t raster_color;

    ISDBSubLayout layout_state;
} ISDBSubState;


typedef void (*AM_Isdb_UpdataJson_t)(AM_ISDB_Handle_t handle);

typedef struct
{
	void               *user_data;     /**< User defined data*/
	AM_Isdb_UpdataJson_t json_update;
	char *json_buffer;
}AM_Isdb_CreatePara_t;

typedef struct
{
	int dummy;
}AM_Isdb_StartPara_t;

AM_ErrorCode_t AM_ISDB_Create(AM_Isdb_CreatePara_t* para, AM_ISDB_Handle_t* handle);
AM_ErrorCode_t AM_ISDB_Decode(AM_ISDB_Handle_t handle, uint8_t *buf, int size);
AM_ErrorCode_t AM_ISDB_Start(AM_Isdb_StartPara_t *para, AM_ISDB_Handle_t handle);
AM_ErrorCode_t AM_ISDB_Stop(AM_ISDB_Handle_t handle);
AM_ErrorCode_t AM_ISDB_Destroy(AM_ISDB_Handle_t *handle);

#ifdef __cplusplus
}
#endif



#endif

