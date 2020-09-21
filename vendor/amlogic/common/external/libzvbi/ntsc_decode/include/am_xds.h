/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief TeletextÄ£¿é(version 2)
 *
 * \author kui.zhang <kui.zhang@amlogic.com>
 * \date 2012-09-13: create the document
 ***************************************************************************/



#ifndef _AM_XDS
#define _AM_XDS


#include <unistd.h>
#include "misc.h"
#include "trigger.h"
#include "format.h"
#include "lang.h"
#include "hamm.h"
#include "tables.h"
#include "vbi.h"
#include <android/log.h>    
#ifdef __cplusplus
extern "C"
{
#endif


typedef enum {
	VBI_XDS_PROGRAM_ID = 0x01,
	VBI_XDS_PROGRAM_LENGTH,
	VBI_XDS_PROGRAM_NAME,
	VBI_XDS_PROGRAM_TYPE,
	VBI_XDS_PROGRAM_RATING,
	VBI_XDS_PROGRAM_AUDIO_SERVICES,
	VBI_XDS_PROGRAM_CAPTION_SERVICES,
	VBI_XDS_PROGRAM_CGMS,
	VBI_XDS_PROGRAM_ASPECT_RATIO,
	
	VBI_XDS_PROGRAM_DATA = 0x0C,
	
	VBI_XDS_PROGRAM_MISC_DATA,
	VBI_XDS_PROGRAM_DESCRIPTION_BEGIN = 0x10,
	VBI_XDS_PROGRAM_DESCRIPTION_END = 0x18
} vbi_xds_subclass_program;



#ifdef __cplusplus
}
#endif

#endif
