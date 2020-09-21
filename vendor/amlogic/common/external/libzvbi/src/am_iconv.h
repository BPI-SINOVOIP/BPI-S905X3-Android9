/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief iconv
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2012-08-10: create the document
 ***************************************************************************/

#ifndef _AM_ICONV_H
#define _AM_ICONV_H

//#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ANDROID
#include <malloc.h>
#include <unicode/ucnv.h>
#include <android/log.h>

typedef struct {
	UConverter *target;
	UConverter *source;
} AM_IConv_t;

typedef AM_IConv_t* iconv_t;

static inline iconv_t
iconv_open(const char *tocode, const char *fromcode)
{
	iconv_t cd = NULL;
	UErrorCode err1 = U_ZERO_ERROR, err2 = U_ZERO_ERROR;

	cd = (iconv_t)malloc(sizeof(AM_IConv_t));
	if(!cd)
		goto error;
	
	cd->target = ucnv_open(tocode, &err1);
	cd->source = ucnv_open(fromcode, &err2);
	if((!U_SUCCESS(err1)) || (!U_SUCCESS(err2)))
		goto error;
	
	return cd;
error:
	if(cd)
	{
		if(U_SUCCESS(err1))
			ucnv_close(cd->target);
		if(U_SUCCESS(err2))
			ucnv_close(cd->source);
		free(cd);
	}
	return (iconv_t)-1;
}

static inline int
iconv_close(iconv_t cd)
{
	if(!cd)
		return 0;
	
	ucnv_close(cd->target);
	ucnv_close(cd->source);
	free(cd);

	return 0;
}

static inline size_t
iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
	UErrorCode err = U_ZERO_ERROR;
	const char *sbegin, *send;
	char *tbegin, *tend;

	if(!cd)
		return -1;

	sbegin = *inbuf;
	send   = sbegin + *inbytesleft;
	tbegin = *outbuf;
	tend   = tbegin + *outbytesleft;

	ucnv_convertEx(cd->target, cd->source, &tbegin, tend, &sbegin, send,
			NULL, NULL, NULL, NULL, FALSE, TRUE, &err);
	if(!U_SUCCESS(err))
		return -1;
	
	*inbuf  = (char*)sbegin;
	*inbytesleft  = send - sbegin;
	*outbuf = tbegin;
	*outbytesleft = tend - tbegin;
	return 0;
}

#else /*!defined(ANDROID)*/
#include <iconv.h>
#endif /*ANDROID*/

#ifdef __cplusplus
}
#endif

#endif

