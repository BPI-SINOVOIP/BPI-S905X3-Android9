/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief Basic datatypes
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-19: create the document
 ***************************************************************************/

#ifndef _AM_TYPES_H
#define _AM_TYPES_H

#include <stdint.h>
#include "pthread.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************
* Global Definitions
*****************************************************************************/

/**\brief Boolean value*/
typedef uint8_t        AM_Bool_t;


/**\brief Error code of the function result,
 * Low 24 bits store the error number.
 * High 8 bits store the module's index.
 */
typedef int            AM_ErrorCode_t;

/**\brief The module's index */
enum AM_MOD_ID
{
	AM_MOD_EVT,    /**< Event module*/
	AM_MOD_DMX,    /**< Demux module*/
	AM_MOD_DVR,    /**< DVR module*/
	AM_MOD_NET,    /**< Network manager module*/
	AM_MOD_OSD,    /**< OSD module*/
	AM_MOD_AV,     /**< AV decoder module*/
	AM_MOD_AOUT,   /**< Audio output device module*/
	AM_MOD_VOUT,   /**< Video output device module*/
	AM_MOD_SMC,    /**< Smartcard module*/
	AM_MOD_INP,    /**< Input device module*/
	AM_MOD_FEND,   /**< DVB frontend device module*/
	AM_MOD_DSC,    /**< Descrambler device module*/
	AM_MOD_CFG,    /**< Configure file manager module*/
	AM_MOD_SI,     /**< SI decoder module*/
	AM_MOD_SCAN,   /**< Channel scanner module*/
	AM_MOD_EPG,    /**< EPG scanner module*/
	AM_MOD_IMG,    /**< Image loader module*/
	AM_MOD_FONT,   /**< Font manager module*/
	AM_MOD_DB,     /**< Database module*/
	AM_MOD_GUI,    /**< GUI module*/
	AM_MOD_REC,    /**< Recorder module*/
	AM_MOD_TV,     /**< TV manager module*/
	AM_MOD_SUB,    /**< Subtitle module*/
	AM_MOD_SUB2,   /**< Subtitle(version 2) module*/
	AM_MOD_TT,     /**< Teletext module*/
	AM_MOD_TT2,    /**< Teletext(version 2) module*/
	AM_MOD_FEND_DISEQCCMD,/**< Diseqc command module*/
	AM_MOD_FENDCTRL, /**< DVB frontend high level control module*/
	AM_MOD_PES,    /**< PES parser module*/
	AM_MOD_CAMAN,  /**< CA manager module*/
	AM_MOD_CI,     /**< DVB-CI module*/
	AM_MOD_USERDATA, /**< MPEG user data reader device module*/
	AM_MOD_CC,     /**< Close caption module*/
	AM_MOD_AD,     /**< Audio description module*/
	AM_MOD_UPD,    /**< Uploader module*/
	AM_MOD_TFILE, /*File wrapper module*/
	AM_MOD_SCTE27,
	AM_MOD_MAX
};

/**\brief Get the error code base of each module
 * \param _mod The module's index
 */
#define AM_ERROR_BASE(_mod)    ((_mod)<<24)

#ifndef AM_SUCCESS
/**\brief Function result: Success*/
#define AM_SUCCESS     (0)
#endif

#ifndef AM_FAILURE
/**\brief Function result: Unknown error*/
#define AM_FAILURE     (-1)
#endif

#ifndef AM_TRUE
/**\brief Boolean value: true*/
#define AM_TRUE        (1)
#endif

#ifndef AM_FALSE
/**\brief Boolean value: false*/
#define AM_FALSE       (0)
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifdef __cplusplus
}
#endif

#endif

