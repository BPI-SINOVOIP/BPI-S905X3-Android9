/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_cc.h
 * \brief Close caption parser module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2011-12-27: create the document
 ***************************************************************************/

#ifndef _AM_CC_H
#define _AM_CC_H

#include <am_types.h>
#include <libzvbi.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/
/**\brief Error code of close caption parser*/
enum AM_CC_ErrorCode
{
	AM_CC_ERROR_BASE=AM_ERROR_BASE(AM_MOD_CC),
	AM_CC_ERR_INVALID_PARAM,   	/**< Invalid parameter*/
	AM_CC_ERR_SYS,                  /**< System error*/
	AM_CC_ERR_NO_MEM,               /**< Not enough memory*/
	AM_CC_ERR_LIBZVBI,              /**< libzvbi error*/
	AM_CC_ERR_BUSY,                 /**< Device is busy*/
	AM_CC_ERR_END
};

/**\brief caption mode*/
typedef enum
{
	AM_CC_CAPTION_NONE = -1,
	AM_CC_CAPTION_DEFAULT,
	/*NTSC CC channels*/
	AM_CC_CAPTION_CC1,
	AM_CC_CAPTION_CC2,
	AM_CC_CAPTION_CC3,
	AM_CC_CAPTION_CC4,
	AM_CC_CAPTION_TEXT1,
	AM_CC_CAPTION_TEXT2,
	AM_CC_CAPTION_TEXT3,
	AM_CC_CAPTION_TEXT4,
	/*DTVCC services*/
	AM_CC_CAPTION_SERVICE1,
	AM_CC_CAPTION_SERVICE2,
	AM_CC_CAPTION_SERVICE3,
	AM_CC_CAPTION_SERVICE4,
	AM_CC_CAPTION_SERVICE5,
	AM_CC_CAPTION_SERVICE6,
	AM_CC_CAPTION_XDS,
	AM_CC_CAPTION_DATA,
	AM_CC_CAPTION_MAX
}AM_CC_CaptionMode_t;

/****************************************************************************
 * Type definitions
 ***************************************************************************/
/**Isdb handle parser's handle*/
typedef void* AM_ISDB_Handle_t;

/**Close caption parser's handle*/
typedef void* AM_CC_Handle_t;
/**Draw parameter of close caption*/
typedef struct AM_CC_DrawPara AM_CC_DrawPara_t;
/**Callback function of beginning of drawing*/
typedef void (*AM_CC_DrawBegin_t)(AM_CC_Handle_t handle, AM_CC_DrawPara_t *draw_para);
/**Callback function of end of drawing*/
typedef void (*AM_CC_DrawEnd_t)(AM_CC_Handle_t handle, AM_CC_DrawPara_t *draw_para);
/**VBI program information callback.*/
typedef void (*AM_CC_VBIProgInfoCb_t)(AM_CC_Handle_t handle, vbi_program_info *pi);
/**VBI network callback.*/
typedef void (*AM_CC_VBINetworkCb_t)(AM_CC_Handle_t handle, vbi_network *n);
typedef void (*AM_CC_VBIRatingCb_t)(AM_CC_Handle_t handle, vbi_rating *rating);
/**CC data callback.*/
typedef void (*AM_CC_DataCb_t)(AM_CC_Handle_t handle, int mask);

typedef void (*AM_CC_UpdataJson_t)(AM_CC_Handle_t handle);
typedef void (*AM_CC_ReportError)(AM_CC_Handle_t handle, int error);


typedef enum {
    CC_STATE_RUNNING      = 0x1001,
    CC_STATE_STOP                 ,

    CMD_CC_START          = 0x2001,
    CMD_CC_STOP                   ,

    CMD_CC_BEGIN          = 0x3000,
    CMD_CC_1                      ,
    CMD_CC_2                      ,
    CMD_CC_3                      ,
    CMD_CC_4                      ,

    //this doesn't support currently
    CMD_TT_1              = 0x3005,
    CMD_TT_2                      ,
    CMD_TT_3                      ,
    CMD_TT_4                      ,

    //cc service
    CMD_SERVICE_1         = 0x4001,
    CMD_SERVICE_2                 ,
    CMD_SERVICE_3                 ,
    CMD_SERVICE_4                 ,
    CMD_SERVICE_5                 ,
    CMD_SERVICE_6                 ,
    CMD_CC_END                    ,

    CMD_SET_COUNTRY_BEGIN = 0x5000,
    CMD_SET_COUNTRY_USA           ,
    CMD_SET_COUNTRY_KOREA         ,
    CMD_SET_COUNTRY_END           ,
    /*set CC source type according ATV or DTV*/
    CMD_CC_SET_VBIDATA   = 0x7001,
    CMD_CC_SET_USERDATA ,

	CMD_CC_SET_CHAN_NUM = 0x8001,
	CMD_VCHIP_RST_CHGSTAT = 0x9001,

    CMD_CC_MAX
} AM_CLOSECAPTION_cmd_t;


/**\brief Font size, see details in CEA-708**/
typedef enum
{
	AM_CC_FONTSIZE_DEFAULT,
	AM_CC_FONTSIZE_SMALL,
	AM_CC_FONTSIZE_STANDARD,
	AM_CC_FONTSIZE_BIG,
	AM_CC_FONTSIZE_MAX
}AM_CC_FontSize_t;

/**\brief Font style, see details in CEA-708*/
typedef enum
{
	AM_CC_FONTSTYLE_DEFAULT,
	AM_CC_FONTSTYLE_MONO_SERIF,
	AM_CC_FONTSTYLE_PROP_SERIF,
	AM_CC_FONTSTYLE_MONO_NO_SERIF,
	AM_CC_FONTSTYLE_PROP_NO_SERIF,
	AM_CC_FONTSTYLE_CASUAL,
	AM_CC_FONTSTYLE_CURSIVE,
	AM_CC_FONTSTYLE_SMALL_CAPITALS,
	AM_CC_FONTSTYLE_MAX
}AM_CC_FontStyle_t;

/**\brief Color opacity, see details in CEA-708**/
typedef enum
{
	AM_CC_OPACITY_DEFAULT,
	AM_CC_OPACITY_TRANSPARENT,
	AM_CC_OPACITY_TRANSLUCENT,
	AM_CC_OPACITY_SOLID,
	AM_CC_OPACITY_FLASH,
	AM_CC_OPACITY_MAX
}AM_CC_Opacity_t;

/**\brief Color, see details in CEA-708-D**/
typedef enum
{
	AM_CC_COLOR_DEFAULT,
	AM_CC_COLOR_WHITE,
	AM_CC_COLOR_BLACK,
	AM_CC_COLOR_RED,
	AM_CC_COLOR_GREEN,
	AM_CC_COLOR_BLUE,
	AM_CC_COLOR_YELLOW,
	AM_CC_COLOR_MAGENTA,
	AM_CC_COLOR_CYAN,
	AM_CC_COLOR_MAX
}AM_CC_Color_t;

/**Close caption drawing parameters*/
struct AM_CC_DrawPara
{
	int caption_width;   /**< Width of the caption*/
	int caption_height;  /**< Height of the caption*/
};

/**\brief User options of close caption*/
typedef struct
{
	AM_CC_FontSize_t        font_size;	/**< Font size*/
	AM_CC_FontStyle_t       font_style;	/**< Font style*/
	AM_CC_Color_t           fg_color;	/**< Frontground color*/
	AM_CC_Opacity_t         fg_opacity;	/**< Frontground opacity*/
	AM_CC_Color_t           bg_color;	/**< Background color*/
	AM_CC_Opacity_t         bg_opacity;	/**< Background opacity*/
}AM_CC_UserOptions_t;

/**\brief Close caption's input.*/
typedef enum {
	AM_CC_INPUT_USERDATA, /**< Input from MPEG userdata.*/
	AM_CC_INPUT_VBI       /**< Input from VBI.*/
}AM_CC_Input_t;

enum AM_CC_Decoder_Error
{
	AM_CC_Decoder_Error_LoseData,
	AM_CC_Decoder_Error_InvalidData,
	AM_CC_Decoder_Error_TimeError,
	AM_CC_Decoder_Error_END
};


/**\brief Close caption parser's create parameters*/
typedef struct
{
	uint8_t            *bmp_buffer;    /**< Drawing buffer*/
	int                 pitch;         /**< Line pitch of the drawing buffer*/
	int                 bypass_cc_enable; /**< Bypass CC data flag*/
	int                 data_timeout;  /**< Data timeout value in ms*/
	int                 switch_timeout;/**< Caption 1/2 swith timeout in ms.*/
	unsigned int       decoder_param;
	void               *user_data;     /**< User defined data*/
	char                lang[10];
	AM_CC_Input_t       input;         /**< Input type.*/
	AM_CC_VBIProgInfoCb_t pinfo_cb;    /**< VBI program information callback.*/
	AM_CC_VBINetworkCb_t  network_cb;  /**< VBI network callback.*/
	AM_CC_VBIRatingCb_t   rating_cb;   /**< VBI rating callback.*/
	AM_CC_DataCb_t      data_cb;       /**< Received data callback.*/
	AM_CC_DrawBegin_t   draw_begin;    /**< Drawing beginning callback*/
	AM_CC_DrawEnd_t     draw_end;      /**< Drawing end callback*/
	AM_CC_UpdataJson_t json_update;
	AM_CC_ReportError report;
	char *json_buffer;
}AM_CC_CreatePara_t;

/**\brief Close caption parser start parameter*/
typedef struct
{
	int 				vfmt;
	AM_CC_CaptionMode_t    caption1;     /**< Mode 1*/
	AM_CC_CaptionMode_t    caption2;     /**< Mode 2.*/
	AM_CC_UserOptions_t    user_options; /**< User options*/
}AM_CC_StartPara_t;


/****************************************************************************
 * Function prototypes
 ***************************************************************************/

/**\brief Create a new close caption parser
 * \param [in] para Create parameters
 * \param [out] handle Return the parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CC_Create(AM_CC_CreatePara_t *para, AM_CC_Handle_t *handle);

/**\brief Release a close caption parser
 * \param [out] handle Close caption parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CC_Destroy(AM_CC_Handle_t handle);

/**
 * \brief Show close caption.
 * \param handle Close caption parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CC_Show(AM_CC_Handle_t handle);

/**
 * \brief Hide close caption.
 * \param handle Close caption parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CC_Hide(AM_CC_Handle_t handle);

/**\brief Start parsing the close caption data
 * \param handle Close caption parser's handle
 * \param [in] para Start parameters
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CC_Start(AM_CC_Handle_t handle, AM_CC_StartPara_t *para);

/**\brief Stop close caption parsing
 * \param handle Close caption parser's handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CC_Stop(AM_CC_Handle_t handle);

/**\brief Set the user options of the close caption
 * \param handle Close caption parser's handle
 * \param [in] options User options
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_CC_SetUserOptions(AM_CC_Handle_t handle, AM_CC_UserOptions_t *options);

/**\brief Get the user defined data of the close caption parser
 * \param handle Close caption parser's handle
 * \return The user defined data
 */
extern void *AM_CC_GetUserData(AM_CC_Handle_t handle);

#ifdef __cplusplus
}
#endif

#endif

