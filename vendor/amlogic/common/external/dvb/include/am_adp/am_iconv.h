/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief iconv
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2012-08-10: create the document
 ***************************************************************************/

#ifndef _AM_ICONV_H
#define _AM_ICONV_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ANDROID
#include <malloc.h>
#include <unicode/ucnv.h>
#include <unicode/putil.h>
#include <unicode/uclean.h>
#include <android/log.h>
#include <dlfcn.h>
#include <assert.h>
#include <errno.h>
#include "am_util.h"

typedef struct {
	UConverter *target;
	UConverter *source;
} AM_IConv_t;

typedef AM_IConv_t* iconv_t;

extern UConverter* (*am_ucnv_open_ptr)(const char *converterName, UErrorCode *err);
extern void (*am_ucnv_close_ptr)(UConverter * converter);
extern void (*am_ucnv_convertEx_ptr)(UConverter *targetCnv, UConverter *sourceCnv,
		char **target, const char *targetLimit,
		const char **source, const char *sourceLimit,
		UChar *pivotStart, UChar **pivotSource,
		UChar **pivotTarget, const UChar *pivotLimit,
		UBool reset, UBool flush,
		UErrorCode *pErrorCode);

extern void am_ucnv_dlink(void);


#ifdef USE_VENDOR_ICU
extern void am_first_action(void);
#define am_ucnv_open(conv, err)\
	({\
        UConverter *ret;\
        am_first_action();\
        ret = ucnv_open(conv, err);\
        ret;\
	 })
#define am_ucnv_close(conv)\
	AM_MACRO_BEGIN\
		am_first_action();\
		ucnv_close(conv);\
	AM_MACRO_END
#define am_ucnv_convertEx(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)\
	AM_MACRO_BEGIN\
		am_first_action();\
		ucnv_convertEx(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);\
	AM_MACRO_END
#else
#define am_ucnv_open(conv, err)\
	({\
	 	UConverter *ret;\
		if(!am_ucnv_open_ptr)\
	 		am_ucnv_dlink();\
	 	ret = am_ucnv_open_ptr(conv, err);\
	 	ret;\
	 })
#define am_ucnv_close(conv)\
	AM_MACRO_BEGIN\
		if(!am_ucnv_close_ptr)\
	 		am_ucnv_dlink();\
		am_ucnv_close_ptr(conv);\
	AM_MACRO_END
#define am_ucnv_convertEx(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)\
	AM_MACRO_BEGIN\
		if(!am_ucnv_convertEx_ptr)\
	 		am_ucnv_dlink();\
		am_ucnv_convertEx_ptr(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13);\
	AM_MACRO_END
#endif

static inline iconv_t
iconv_open(const char *tocode, const char *fromcode)
{
	iconv_t cd = NULL;
	UErrorCode err1 = U_ZERO_ERROR, err2 = U_ZERO_ERROR;

	cd = (iconv_t)malloc(sizeof(AM_IConv_t));
	if(!cd)
		goto error;
	
	cd->target = (UConverter *)am_ucnv_open(tocode, &err1);
	cd->source = (UConverter *)am_ucnv_open(fromcode, &err2);
	if((!U_SUCCESS(err1)) || (!U_SUCCESS(err2)))
		goto error;
	
	return cd;
error:
	if(cd)
	{
		if(U_SUCCESS(err1))
			am_ucnv_close(cd->target);
		else
			errno = err1;//use errno for error code export
		if(U_SUCCESS(err2))
			am_ucnv_close(cd->source);
		else
			errno = err2;//use errno for error code export
		free(cd);
	}

	return (iconv_t)-1;
}

static inline int
iconv_close(iconv_t cd)
{
	if(!cd)
		return 0;
	
	am_ucnv_close(cd->target);
	am_ucnv_close(cd->source);
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

	am_ucnv_convertEx(cd->target, cd->source, &tbegin, tend, &sbegin, send,
			NULL, NULL, NULL, NULL, FALSE, TRUE, &err);
	if(!U_SUCCESS(err)) {
		errno = err;//use errno for error code export
		return -1;
	}
	
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

