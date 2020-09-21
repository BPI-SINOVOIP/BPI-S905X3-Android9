/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_xds.c
 * \brief xds module
 *
 * \author Ke Gong <ke.gong@amlogic.com>
 * \date 2015-08-11: create the document
 ***************************************************************************/
#include <am_types.h>
#include <am_thread.h>
#include <am_mem.h>
#include "am_debug.h"
#include "am_xds_internal.h"
#include "am_xds.h"
#include "am_epg.h"

#include <am_fend.h>
#include <am_db.h>
#include <assert.h>
#include "atsc/atsc_descriptor.h"

//#define XDS_DEBUG

#ifndef XDS_DEBUG
#ifdef ANDROID
#define TAG      "ATSC_CC_XDS"
#define AM_XDS_DBG(a...) __android_log_print(ANDROID_LOG_INFO, TAG, a)
#else
#define AM_XDS_DBG(a...) printf(a)
#endif//END ANDROID
#else
#define AM_XDS_DBG(a...) AM_XDS_DBG(a)
#endif

#define STEP_STMT(_stmt, _name, _sql)\
        AM_MACRO_BEGIN\
                int ret1, ret2;\
                        ret1 = sqlite3_step(_stmt);\
                        ret2 = sqlite3_reset(_stmt);\
                        if (ret1 == SQLITE_ERROR && ret2 == SQLITE_SCHEMA){\
                                AM_XDS_DBG("\n[%s:%d]Database schema changed, now re-prepare the stmts...",__FUNCTION__,__LINE__);\
                                AM_DB_GetSTMT(&(_stmt), _name, _sql, 1);\
                        }\
        AM_MACRO_END

#define AM_XDS_2_SECONDS(h,m,s)   (h*3600+m*60+s)

typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned int   UINT32;
typedef int            INT32;

#define PIN_PARAMS 			4
#define LEN_PARAMS			6
#define PROG_NAME_PARAMS		32
#define PROG_TYPE_PARAMS		32
#define PROG_RATING_PARAMS		2
#define AUDIO_PARAMS			2
#define CS_PARAMS			8
#define AR_PARAMS			4
#define NETWORK_NAME_PARAMS		32
#define NATIVE_CHANNEL_PARAMS   	6
#define	TAPE_DELAY_PARAMS		2
#define	TSID_PARAMS			4
#define	TIME_OF_DAY_PARAMS		6
#define	IMPULSE_PARAMS			6
#define	SUPPLEMENT_PARAMS		32
#define	LOCAL_TIME_ZONE_PARAMS  	2
#define	OOB_PARAMS			2
#define	WEATHER_CODE_PARAMS		0
#define	WEATHER_MSG_PARAMS		32
#define COMP_PKT_PARAMS			32
#define PROG_DESC_PKTS			8
#define PROG_DESC_PARAMS		32
#define CGMS_PARAMS			2
#define CH_MAP_POINTER_PARAMS    	2
#define CH_MAP_HEADER_PARAMS    	4
#define CH_MAP_PACKET_PARAMS    	10


AM_XDSContext_t	*xdsContext = (AM_XDSContext_t*) NULL;
const char *db_path ="/data/data/com.amlogic.tvservice/databases/dvb.db";


static AM_XDSSqlite_t xds_sqllist[] ={
 {"update evt_table set start=?, end=? where db_id=?", "update startend"},
 {"update evt_table set source_id=?,rrt_ratings=? where db_id=?", "update rrt_rating"},
 {"update evt_table set event_id=?,name=? where db_id=?", "update eventname" },
 {"update evt_table set descr=?,items=?,ext_descr=? where db_id=?", "update descr"},
 {"update srv_table set name=?,db_ts_id=?,eit_schedule_flag=?,eit_pf_flag=?,aud_langs=? where db_id=?", "update srv_table"},
 {"insert into evt_table(src,db_net_id, db_ts_id, \
   db_srv_id, event_id, name, start, end, descr, items, ext_descr,nibble_level,\
   sub_flag,sub_status,parental_rating,source_id,rrt_ratings) \
   values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)","insert xds events"}
};

static atsc_content_advisory_dr_t xds_ca_info;//save the rrt-rating from xds
struct dvb_frontend_parameters fparam;
char vchip_rrt_rating[256];
int vchip_changed = 0;
sqlite3 *hdb;
static int ntsc_current_channumner = 0;/*indicate current ntsc major and minor channel number,set by APP/

/*
 * Forward Declarations  :
 */ 
static AM_XDSState_t xds_get_classtype(UINT8	data);
static AM_XDSState_t xds_get_channeltype(UINT8 data);
static AM_XDSState_t xds_get_misctype(UINT8	data);
static AM_XDSState_t xds_get_publictype(UINT8 data);
static void xds_handle_PINstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_PINstate(AM_XDSContext_t *context);
static void xds_handle_lentimestate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_lentimestate(AM_XDSContext_t *context);
static void xds_handle_prognamestate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_prognamestate(AM_XDSContext_t *context);
static void xds_handle_progtypestate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_progtypestate(AM_XDSContext_t *context);
static void xds_handle_contentadvisorystate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_contentadvisorystate(AM_XDSContext_t *context);
static void xds_handle_audioservicesstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_audioservicesstate(AM_XDSContext_t *context);
static void xds_handle_captionservicesstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_captionservicesstate(AM_XDSContext_t *context);
static void xds_extract_content_advisory(AM_ContentAdvisoryInfo_t xds_CAInfo);
static void xds_handle_aspectratiostate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_aspectratiostate(AM_XDSContext_t *context);
static void xds_handle_cgmsstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_cgmsstate(AM_XDSContext_t *context);
static void xds_handle_comppkt1state(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_comppkt1state(AM_XDSContext_t *context);
static void xds_handle_comppkt2state(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_comppkt2state(AM_XDSContext_t *context);
static void xds_handle_progdescstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_progdescstate(AM_XDSContext_t *context);
static void xds_handle_networknamestate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_networknamestate(AM_XDSContext_t *context);
static void xds_handle_nativechannelstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_nativechannelstate(AM_XDSContext_t *context);
static void xds_handle_tapedelaystate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_tapedelaystate(AM_XDSContext_t *context);
static void xds_handle_TSIDstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_TSIDstate(AM_XDSContext_t *context);
static void xds_handle_timeofdaystate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_timeofdaystate(AM_XDSContext_t *context);
static void xds_handle_impulsestate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_impulsestate(AM_XDSContext_t *context);
static void xds_handle_supplementstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_supplementstate(AM_XDSContext_t *context);
static void xds_handle_localtimezonestate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_localtimezonestate(AM_XDSContext_t *context);
static void xds_handle_OOBstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_OOBstate(AM_XDSContext_t *context);
static void xds_handle_CHMappingstate(UINT8 data,AM_XDSState_t state,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_CHMappingstate(AM_XDSState_t state,AM_XDSContext_t *context);
static void xds_handle_weathercodestate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_weathercodestate(AM_XDSContext_t *context);
static void xds_handle_weathermsgstate(UINT8 data,AM_XDSContext_t *context);
static AM_ErrorCode_t xds_finish_weathermsgstate(AM_XDSContext_t *context);
static AM_XDSProgType_t xds_get_progtype(UINT8 data);
static AM_XDSLanguage_t xds_get_audiolanguage(UINT8 data);
static AM_XDSAudioType_t xds_get_mainaudio(UINT8 data);
static AM_XDSAudioType_t xds_get_secondaudio(UINT8 data);
static AM_XDSCaptionServices_t xds_get_ccservicetype(UINT8 data);
static AM_XDSProgRating_t xds_get_mpaaprograting(UINT8 data);
static AM_XDSProgRating_t xds_get_cflprograting(UINT8 data);
static AM_XDSProgRating_t xds_get_celprograting(UINT8 data);
static AM_XDSProgRating_t xds_get_ustvprograting(UINT8 data);
static const char *xds_get_programratingstring(AM_XDSProgRating_t rating);
static const char *xds_get_cgmsstring(AM_CgmsInfo_t type);
static const char *xds_get_programtypestring(AM_XDSProgType_t type);
static const char *xds_get_aspectratiostring(AM_XDSAR_t type);
static const char *xds_get_audiotypestring(AM_XDSAudioType_t type);
static const char *xds_get_languagestring(AM_XDSLanguage_t lang);
static AM_ErrorCode_t xds_store_useproc_event(AM_xdsUserProcEvent_t event);
void AM_Reset_XDSCounters(AM_XDSContext_t *context);
AM_ErrorCode_t AM_Handle_XDSInformationCharacters(UINT8 data1,UINT8 data2,AM_XDSState_t state,AM_XDSContext_t *context);
AM_ErrorCode_t AM_Handle_XDSEndState(UINT8 checksum,AM_XDSState_t state,AM_XDSContext_t *context);
AM_XDSState_t AM_Get_XDSClass(UINT8 data);
AM_XDSState_t AM_Get_XDSType(UINT8 data,AM_XDSState_t currState);

/**************************************************************************************
 *
 * Function 	       :  AM_Get_XDSDataInfo
 *	
 * Input Parameters  :  AM_XDSInfo_t		*info		XDS Data Structure to be returned
 *
 * Output Parameters :  
 *
 * Return Value		   : Error Code -   AM_FAILURE,
 *
 * Description       : Returns the Parsed and decoded XDS Data Information as available
 *                     by the video driver. 
 *
 ***************************************************************************************/ 

AM_ErrorCode_t AM_Get_XDSDataInfo(AM_XDSInfo_t *returnInfo)
{
	AM_ErrorCode_t err = AM_SUCCESS;
	UINT32	i;

	
	if(returnInfo == NULL)
	{
		AM_XDS_DBG("\n[%s:%d] : AM_XDSInfo_t Structure Passed is NULL",__FUNCTION__,__LINE__);
		err = AM_FAILURE;
	}
	
	pthread_mutex_lock(&xdsContext->lock);
        returnInfo->currentXDSClass = xdsContext->XDSBuffer->currentXDSClass;
	returnInfo->startTime.minute = xdsContext->XDSBuffer->startTime.minute;
	returnInfo->startTime.hour  = xdsContext->XDSBuffer->startTime.hour;
	returnInfo->startDate  = xdsContext->XDSBuffer->startDate;
	returnInfo->startMonth  = xdsContext->XDSBuffer->startMonth;
	returnInfo->progLen.minute  = xdsContext->XDSBuffer->progLen.minute;
	returnInfo->progLen.hour  = xdsContext->XDSBuffer->progLen.hour;
	returnInfo->timeElapsed.minute  = xdsContext->XDSBuffer->timeElapsed.minute;
	returnInfo->timeElapsed.hour = xdsContext->XDSBuffer->timeElapsed.hour;
	returnInfo->timeElapsedSec  = xdsContext->XDSBuffer->timeElapsedSec;
	strcpy(returnInfo->progTitle,xdsContext->XDSBuffer->progTitle);
	for(i=0;i<32;i++)
	{
		returnInfo->progType[i].basicType  = xdsContext->XDSBuffer->progType[i].basicType;
		returnInfo->progType[i].detailType  = xdsContext->XDSBuffer->progType[i].detailType;
	}
	returnInfo->rating  = xdsContext->XDSBuffer->rating;
	returnInfo->audio.mainLang = xdsContext->XDSBuffer->audio.mainLang;
	returnInfo->audio.mainType = xdsContext->XDSBuffer->audio.mainType;
	returnInfo->audio.sapLang = xdsContext->XDSBuffer->audio.sapLang;
	returnInfo->audio.sapType = xdsContext->XDSBuffer->audio.sapType;
	for(i=0;i<8;i++)
	{
			
		returnInfo->service[i] = xdsContext->XDSBuffer->service[i];
		returnInfo->serviceLang[i] = xdsContext->XDSBuffer->serviceLang[i];
	}
	returnInfo->aspectRatio.startLine = xdsContext->XDSBuffer->aspectRatio.startLine;
	returnInfo->aspectRatio.endLine = xdsContext->XDSBuffer->aspectRatio.endLine;
	returnInfo->aspectRatio.videoSqueezed = xdsContext->XDSBuffer->aspectRatio.videoSqueezed;
	returnInfo->aspectRatio.ratio = xdsContext->XDSBuffer->aspectRatio.ratio;
	returnInfo->cgmsInfoPresent = xdsContext->XDSBuffer->cgmsInfoPresent;
	returnInfo->cgmsInfo = xdsContext->XDSBuffer->cgmsInfo;
	
	for(i=0;i<5;i++)
		 returnInfo->pkt1.type[i] = xdsContext->XDSBuffer->pkt1.type[i];
			 
	returnInfo->pkt1.rating = xdsContext->XDSBuffer->pkt1.rating;
	returnInfo->pkt1.progLen.minute = xdsContext->XDSBuffer->pkt1.progLen.minute ;
	returnInfo->pkt1.progLen.hour = xdsContext->XDSBuffer->pkt1.progLen.hour;
	returnInfo->pkt1.timeShow.minute= xdsContext->XDSBuffer->pkt1.timeShow.minute;
	returnInfo->pkt1.timeShow.hour =  xdsContext->XDSBuffer->pkt1.timeShow.hour;
        strcpy(returnInfo->pkt1.title,xdsContext->XDSBuffer->pkt1.title);
	returnInfo->pkt2.startTime.minute= xdsContext->XDSBuffer->pkt2.startTime.minute;
	returnInfo->pkt2.startTime.hour= xdsContext->XDSBuffer->pkt2.startTime.hour;
	returnInfo->pkt2.startDate= xdsContext->XDSBuffer->pkt2.startDate;
	returnInfo->pkt2.startMonth= xdsContext->XDSBuffer->pkt2.startMonth;
	returnInfo->pkt2.audio.mainLang= xdsContext->XDSBuffer->pkt2.audio.mainLang;
	returnInfo->pkt2.audio.mainType= xdsContext->XDSBuffer->pkt2.audio.mainType;
	returnInfo->pkt2.audio.sapLang= xdsContext->XDSBuffer->pkt2.audio.sapLang;
	returnInfo->pkt2.audio.sapType= xdsContext->XDSBuffer->pkt2.audio.sapType;
		
	for(i=0;i<2;i++) 
	{
		 returnInfo->pkt2.services[i] = xdsContext->XDSBuffer->pkt2.services[i];
		 returnInfo->pkt2.serviceLanguage[i] = xdsContext->XDSBuffer->pkt2.serviceLanguage[i];
	}
  	strcpy(returnInfo->pkt2.station.name,xdsContext->XDSBuffer->pkt2.station.name);
  	strcpy(returnInfo->pkt2.station.chNum,xdsContext->XDSBuffer->pkt2.station.chNum);
  	strcpy(returnInfo->pkt2.networkName,xdsContext->XDSBuffer->pkt2.networkName);
	  
  	for(i=0;i<8;i++) strcpy(returnInfo->progDesc[i].title,xdsContext->XDSBuffer->progDesc[i].title);
		
	returnInfo->futureStartTime.minute = xdsContext->XDSBuffer->futureStartTime.minute;
	returnInfo->futureStartTime.hour  = xdsContext->XDSBuffer->futureStartTime.hour;
	returnInfo->futureStartDate  = xdsContext->XDSBuffer->futureStartDate;
	returnInfo->futureStartMonth  = xdsContext->XDSBuffer->futureStartMonth;
	returnInfo->futureProgLen.minute  = xdsContext->XDSBuffer->futureProgLen.minute;
	returnInfo->futureProgLen.hour  = xdsContext->XDSBuffer->futureProgLen.hour;
	returnInfo->futureTimeElapsed.minute  = xdsContext->XDSBuffer->futureTimeElapsed.minute;
	returnInfo->futureTimeElapsed.hour = xdsContext->XDSBuffer->futureTimeElapsed.hour;
	returnInfo->futureTimeElapsedSec  = xdsContext->XDSBuffer->futureTimeElapsedSec;
	strcpy(returnInfo->futureProgTitle,xdsContext->XDSBuffer->futureProgTitle);
		
	for(i=0;i<32;i++)
	{
		returnInfo->futureProgType[i].basicType  = xdsContext->XDSBuffer->futureProgType[i].basicType;
		returnInfo->futureProgType[i].detailType  = xdsContext->XDSBuffer->futureProgType[i].detailType;
	}
		
	returnInfo->futureValidTypes = xdsContext->XDSBuffer->futureValidTypes;
	returnInfo->futureRating  = xdsContext->XDSBuffer->futureRating;
	returnInfo->futureAudio.mainLang = xdsContext->XDSBuffer->futureAudio.mainLang;
	returnInfo->futureAudio.mainType = xdsContext->XDSBuffer->futureAudio.mainType;
	returnInfo->futureAudio.sapLang = xdsContext->XDSBuffer->futureAudio.sapLang;
	returnInfo->futureAudio.sapType = xdsContext->XDSBuffer->futureAudio.sapType;
		
	for(i=0;i<8;i++)
	{
		returnInfo->futureService[i] = xdsContext->XDSBuffer->futureService[i];
		returnInfo->futureServiceLang[i] = xdsContext->XDSBuffer->futureServiceLang[i];
	}
	returnInfo->futureAspectRatio.startLine = xdsContext->XDSBuffer->futureAspectRatio.startLine;
	returnInfo->futureAspectRatio.endLine = xdsContext->XDSBuffer->futureAspectRatio.endLine;
	returnInfo->futureAspectRatio.videoSqueezed = xdsContext->XDSBuffer->futureAspectRatio.videoSqueezed;
	returnInfo->futureAspectRatio.ratio = xdsContext->XDSBuffer->futureAspectRatio.ratio;
	returnInfo->futureCgmsInfoPresent = xdsContext->XDSBuffer->futureCgmsInfoPresent;
	returnInfo->futureCgmsInfo = xdsContext->XDSBuffer->futureCgmsInfo;
		
	for(i=0;i<5;i++)
		 returnInfo->futurePkt1.type[i] = xdsContext->XDSBuffer->futurePkt1.type[i];
			 
	returnInfo->futurePkt1.rating = xdsContext->XDSBuffer->futurePkt1.rating;
	returnInfo->futurePkt1.progLen.minute = xdsContext->XDSBuffer->futurePkt1.progLen.minute ;
	returnInfo->futurePkt1.progLen.hour = xdsContext->XDSBuffer->futurePkt1.progLen.hour;
	returnInfo->futurePkt1.timeShow.minute= xdsContext->XDSBuffer->futurePkt1.timeShow.minute;
	returnInfo->futurePkt1.timeShow.hour =  xdsContext->XDSBuffer->futurePkt1.timeShow.hour;
  	strcpy(returnInfo->futurePkt1.title,xdsContext->XDSBuffer->futurePkt1.title);
	returnInfo->futurePkt2.startTime.minute= xdsContext->XDSBuffer->futurePkt2.startTime.minute;
	returnInfo->futurePkt2.startTime.hour= xdsContext->XDSBuffer->futurePkt2.startTime.hour;
	returnInfo->futurePkt2.startDate= xdsContext->XDSBuffer->futurePkt2.startDate;
	returnInfo->futurePkt2.startMonth= xdsContext->XDSBuffer->futurePkt2.startMonth;
	returnInfo->futurePkt2.audio.mainLang= xdsContext->XDSBuffer->futurePkt2.audio.mainLang;
	returnInfo->futurePkt2.audio.mainType= xdsContext->XDSBuffer->futurePkt2.audio.mainType;
	returnInfo->futurePkt2.audio.sapLang= xdsContext->XDSBuffer->futurePkt2.audio.sapLang;
	returnInfo->futurePkt2.audio.sapType= xdsContext->XDSBuffer->futurePkt2.audio.sapType;
	  
  	for(i=0;i<2;i++) 
	{
		 returnInfo->futurePkt2.services[i] = xdsContext->XDSBuffer->futurePkt2.services[i];
		 returnInfo->futurePkt2.serviceLanguage[i] = xdsContext->XDSBuffer->futurePkt2.serviceLanguage[i];
	}
	  
  	strcpy(returnInfo->futurePkt2.station.name,xdsContext->XDSBuffer->futurePkt2.station.name);
  	strcpy(returnInfo->futurePkt2.station.chNum,xdsContext->XDSBuffer->futurePkt2.station.chNum);
  	strcpy(returnInfo->futurePkt2.networkName,xdsContext->XDSBuffer->futurePkt2.networkName);
  	for(i=0;i<8;i++) strcpy(returnInfo->futureProgDesc[i].title,xdsContext->XDSBuffer->futureProgDesc[i].title);	
	
	strcpy(returnInfo->networkName,xdsContext->XDSBuffer->networkName);
	strcpy(returnInfo->station.name,xdsContext->XDSBuffer->station.name);
	strcpy(returnInfo->station.chNum,xdsContext->XDSBuffer->station.chNum);
	returnInfo->delayTime.minute = xdsContext->XDSBuffer->delayTime.minute;
	returnInfo->delayTime.hour = xdsContext->XDSBuffer->delayTime.hour;
	returnInfo->tsid = xdsContext->XDSBuffer->tsid;


	returnInfo->currentDayTime.minute = xdsContext->XDSBuffer->currentDayTime.minute;
	returnInfo->currentDayTime.hour = xdsContext->XDSBuffer->currentDayTime.hour;
	returnInfo->currentDate = xdsContext->XDSBuffer->currentDate;
	returnInfo->currentMonth = xdsContext->XDSBuffer->currentMonth;
	returnInfo->currentDay = xdsContext->XDSBuffer->currentDay ;
	returnInfo->currentYear = xdsContext->XDSBuffer->currentYear;
	returnInfo->captureStartTime.minute = xdsContext->XDSBuffer->captureStartTime.minute;
	returnInfo->captureStartTime.hour = xdsContext->XDSBuffer->captureStartTime.hour;
	returnInfo->captureStartDate = xdsContext->XDSBuffer->captureStartDate;
	returnInfo->captureStartMonth = xdsContext->XDSBuffer->captureStartMonth;
	returnInfo->captureProgLen.minute = xdsContext->XDSBuffer->captureProgLen.minute;
	returnInfo->captureProgLen.hour = xdsContext->XDSBuffer->captureProgLen.hour;
		
	for(i=0;i<32;i++)
	{
		returnInfo->supplementInfo[i].field1 = xdsContext->XDSBuffer->supplementInfo[i].field1;
		returnInfo->supplementInfo[i].lineNum = xdsContext->XDSBuffer->supplementInfo[i].lineNum;
	}
		
	returnInfo->timeZoneOffset.offset = xdsContext->XDSBuffer->timeZoneOffset.offset;
	returnInfo->timeZoneOffset.present = xdsContext->XDSBuffer->timeZoneOffset.present;
	returnInfo->oobChannelNumber = xdsContext->XDSBuffer->oobChannelNumber;
	returnInfo->chMap = xdsContext->XDSBuffer->chMap;

	returnInfo->weatherCodePresent = xdsContext->XDSBuffer->weatherCodePresent;
	strcpy(returnInfo->message,xdsContext->XDSBuffer->message);
        pthread_mutex_unlock(&xdsContext->lock);

	AM_XDS_DBG("\n[%s:%d] -->(0x%08X)\n",__FUNCTION__,__LINE__, (UINT32)returnInfo);

	return err;
}

/**************************************************************************************
 *
 * Function 	       : AM_Get_XDSContentAdvisoryInfo
 *	
 * Input Parameters  :   AM_ContentAdvisoryInfo_t  *info
 *
 * Output Parameters :  
 * 					   
 *
 * Return Value		   : Error Code -  AM_FAILURE,
 *
 * Description       : This function returns the content advisory information. 
 *
 ***************************************************************************************/ 
AM_ErrorCode_t AM_Get_XDSContentAdvisoryInfo (AM_ContentAdvisoryInfo_t *info)
{
	AM_ErrorCode_t err=AM_SUCCESS;

	if(info == NULL)
	{
		AM_XDS_DBG("GetXDSAM_ContentAdvisoryInfo_t : AM_XDSInfo_t Structure Passed is NULL");
		err = AM_FAILURE;
	}

	if(AM_SUCCESS == err)
	{
		pthread_mutex_lock(&xdsContext->lock);
		info->rating  = xdsContext->XDSBuffer->rating.rating;
		info->fantasyViolence  = xdsContext->XDSBuffer->rating.fantasyViolence;
		info->violence  = xdsContext->XDSBuffer->rating.violence;
		info->sexualSituations  = xdsContext->XDSBuffer->rating.sexualSituations;
		info->adultLanguage  = xdsContext->XDSBuffer->rating.adultLanguage;
		info->ssDialog  = xdsContext->XDSBuffer->rating.ssDialog;
		pthread_mutex_unlock(&xdsContext->lock);
	}

	AM_XDS_DBG("GetXDSAM_ContentAdvisoryInfo_t -->(0x%08X,%s)\n", (UINT32)info,err == AM_SUCCESS ? "seccussfuly":"failure");	
	return err;
}


/*
 * Extended Data Support Internal Routines :
 */ 

/*
 * AM_Initialize_XDSDataServices :
 *
 * This function Initializes all the XDS Services 
 * and allocated memory for the XDS Buffer.
 */ 

AM_ErrorCode_t AM_Initialize_XDSDataServices(void)
{
	AM_ErrorCode_t err = AM_SUCCESS;

#if 0//need get current NTSC FE parameter including frequency to hit record in dvb.db
        //get NTSC dvb_frontend_parameters
        err = AM_FEND_GetLastPara(&fparam);
        if(AM_SUCCESS != err){
           AM_XDS_DBG("\n %s fail to get current ATV freqency,ret =%d !\n",__FUNCTION__,err);
           return err;
        }   

        AM_XDS_DBG("\n curren ATV lock frequency is  %u,ret =%d \n", fparam.frequency,err);
#endif
	/* Allocate XDS Context */
	xdsContext = (AM_XDSContext_t *)malloc(sizeof(AM_XDSContext_t));
	if(xdsContext == NULL)
	{
		return AM_FAILURE;
	}
	else
	{
		memset(xdsContext, 0, sizeof(AM_XDSContext_t));
	}

	/* Allocate XDS Buffers */
	xdsContext->XDSBuffer = (AM_XDSInfo_t *)malloc(sizeof(AM_XDSInfo_t));
	if(xdsContext->XDSBuffer == NULL)
	{
		free(xdsContext);
		return AM_FAILURE;
	}

	/* Create context sema4 */
        pthread_mutex_init(&xdsContext->lock, NULL);
	AM_Initialize_XDSData(xdsContext);	

        /* init dvb.db for store xds information */
        AM_DB_Setup(db_path, hdb);
        AM_DB_HANDLE_PREPARE(hdb);
        memset(&xds_ca_info ,0,sizeof(atsc_content_advisory_dr_t));
        
	return err;
}

/*
 * AM_Terminate_XDSDataServices :
 *
 * This function frees up all the resources allocated for 
 * XDS Data 
 *
 */
void AM_Terminate_XDSDataServices(void)
{
	AM_ErrorCode_t err = AM_SUCCESS;

	free(xdsContext->XDSBuffer);
	xdsContext->XDSBuffer = (AM_XDSInfo_t *) NULL;
        pthread_mutex_destroy(&xdsContext->lock);
	free(xdsContext);
	xdsContext = (AM_XDSContext_t *) NULL;

        /*terminate dvb.db access*/
        AM_DB_Quit(hdb);
        
	return;
}

/*
 * AM_Initialize_XDSData : 
 *
 * This routines initilaizes the XDS Data Buffer
 */ 
void AM_Initialize_XDSData(AM_XDSContext_t *context)
{
	 UINT32 i;

	 context->XDSBuffer->startTime.minute = 0;
	 context->XDSBuffer->startTime.hour =0 ;
	 context->XDSBuffer->startDate = 0;
	 context->XDSBuffer->startMonth = 0;
	 context->XDSBuffer->progLen.minute = 0;
	 context->XDSBuffer->progLen.hour= 0;
	 context->XDSBuffer->timeElapsed.minute= 0;
	 context->XDSBuffer->timeElapsed.hour=0 ;
	 context->XDSBuffer->timeElapsedSec=0 ;
	 strcpy(context->XDSBuffer->progTitle," ");
	 strcpy(context->progTitle," ");
	 
	 for(i=0;i<32;i++)
	 {
		 context->XDSBuffer->progType[i].basicType = AM_XDSPROG_UNAVAILABLE;
		 context->XDSBuffer->progType[i].detailType = AM_XDSPROG_UNAVAILABLE;
	 }
	 
	 context->XDSBuffer->rating.rating = RATINGNOTAVAILABLE ;
	 context->XDSBuffer->rating.fantasyViolence = AM_FALSE;
	 context->XDSBuffer->rating.violence = AM_FALSE;
	 context->XDSBuffer->rating.sexualSituations = AM_FALSE;
	 context->XDSBuffer->rating.adultLanguage = AM_FALSE;
	 context->XDSBuffer->rating.ssDialog = AM_FALSE;
	 context->XDSBuffer->audio.mainLang = XUNKNOWN;
	 context->XDSBuffer->audio.mainType = UNKNOWNTYPE;
	 context->XDSBuffer->audio.sapLang = XUNKNOWN;
	 context->XDSBuffer->audio.sapType= UNKNOWNTYPE;
	 
	 for(i=0;i<8;i++)
	 {
		 context->XDSBuffer->service[i] = AM_UNKNOWNSERV;
		 context->XDSBuffer->serviceLang[i] = XUNKNOWN;
	 }
	 
	 context->XDSBuffer->aspectRatio.startLine = 0;
	 context->XDSBuffer->aspectRatio.endLine = 0;
	 context->XDSBuffer->aspectRatio.videoSqueezed = AM_FALSE;
	 context->XDSBuffer->aspectRatio.ratio = UNKNOWNAR;
	 context->XDSBuffer->cgmsInfoPresent=AM_FALSE;
	 context->XDSBuffer->cgmsInfo.copyControl = UNKNOWNCONDITION;
	 context->XDSBuffer->cgmsInfo.analogProtectionSystem = UNKNOWNCONDITION;
	 context->XDSBuffer->cgmsInfo.analogSourceBit = AM_FALSE;
	 
	 for(i=0;i<5;i++)
		 context->XDSBuffer->pkt1.type[i] = AM_XDSPROG_UNAVAILABLE;
		 
	 context->XDSBuffer->pkt1.rating.rating = RATINGNOTAVAILABLE ;
	 context->XDSBuffer->pkt1.rating.fantasyViolence = AM_FALSE;
	 context->XDSBuffer->pkt1.rating.violence = AM_FALSE;
	 context->XDSBuffer->pkt1.rating.sexualSituations = AM_FALSE;
	 context->XDSBuffer->pkt1.rating.adultLanguage = AM_FALSE;
	 context->XDSBuffer->pkt1.rating.ssDialog = AM_FALSE;
	 context->XDSBuffer->pkt1.progLen.minute =0;
	 context->XDSBuffer->pkt1.progLen.hour = 0;
	 context->XDSBuffer->pkt1.timeShow.minute=0;
	 context->XDSBuffer->pkt1.timeShow.hour=0;
	 strcpy(context->XDSBuffer->pkt1.title," ");
	 context->XDSBuffer->pkt2.startTime.minute=0;
	 context->XDSBuffer->pkt2.startTime.hour=0;
	 context->XDSBuffer->pkt2.startDate=0;
	 context->XDSBuffer->pkt2.startMonth=0;
	 context->XDSBuffer->pkt2.audio.mainLang=XUNKNOWN;	
	 context->XDSBuffer->pkt2.audio.mainType=UNKNOWNTYPE;
	 context->XDSBuffer->pkt2.audio.sapLang=XUNKNOWN;
	 context->XDSBuffer->pkt2.audio.sapType=UNKNOWNTYPE;
	 
	 for(i=0;i<2;i++) 
	 {
		 context->XDSBuffer->pkt2.services[i]=AM_UNKNOWNSERV;
		 context->XDSBuffer->pkt2.serviceLanguage[i]=XUNKNOWN;
	 }
	 strcpy(context->XDSBuffer->pkt2.station.name," ");
	 strcpy(context->XDSBuffer->pkt2.station.chNum," ");
	 strcpy(context->XDSBuffer->pkt2.networkName," ");
	 for(i=0;i<8;i++) 
	 {
		 strcpy(context->XDSBuffer->progDesc[i].title," ");
		 strcpy(context->progDesc[i].title," ");
	 }
		
	 context->XDSBuffer->futureStartTime.minute =0;
	 context->XDSBuffer->futureStartTime.hour=0;
	 context->XDSBuffer->futureStartDate=0;
	 context->XDSBuffer->futureStartMonth=0;
	 context->XDSBuffer->futureProgLen.minute=0;
	 context->XDSBuffer->futureProgLen.hour=0;
	 context->XDSBuffer->futureTimeElapsed.minute=0;
	 context->XDSBuffer->futureTimeElapsed.hour=0;
	 context->XDSBuffer->futureTimeElapsedSec=0;
	 strcpy(context->XDSBuffer->futureProgTitle," ");
	 for(i=0;i<32;i++)
	 {
	 	context->XDSBuffer->futureProgType[i].basicType  = AM_XDSPROG_UNAVAILABLE;
	 	context->XDSBuffer->futureProgType[i].detailType = AM_XDSPROG_UNAVAILABLE;
	 }
	 context->XDSBuffer->futureValidTypes = 0;
	 context->XDSBuffer->futureRating.rating = RATINGNOTAVAILABLE ;
	 context->XDSBuffer->futureRating.fantasyViolence = AM_FALSE;
	 context->XDSBuffer->futureRating.violence = AM_FALSE;
	 context->XDSBuffer->futureRating.sexualSituations = AM_FALSE;
	 context->XDSBuffer->futureRating.adultLanguage = AM_FALSE;
	 context->XDSBuffer->futureRating.ssDialog = AM_FALSE;
	 context->XDSBuffer->futureAudio.mainLang=XUNKNOWN;
	 context->XDSBuffer->futureAudio.mainType=UNKNOWNTYPE;
	 context->XDSBuffer->futureAudio.sapLang=XUNKNOWN;
	 context->XDSBuffer->futureAudio.sapType=UNKNOWNTYPE;
	 for(i=0;i<8;i++)
	 {
	 	context->XDSBuffer->futureService[i] = AM_UNKNOWNSERV;
	 	context->XDSBuffer->futureServiceLang[i] = XUNKNOWN;
	 }
	 context->XDSBuffer->futureAspectRatio.startLine=0;
	 context->XDSBuffer->futureAspectRatio.endLine=0;
	 context->XDSBuffer->futureAspectRatio.videoSqueezed=AM_FALSE;
	 context->XDSBuffer->futureAspectRatio.ratio=UNKNOWNAR;
	 context->XDSBuffer->futureCgmsInfoPresent=AM_FALSE;
	 context->XDSBuffer->futureCgmsInfo.copyControl = UNKNOWNCONDITION;
	 context->XDSBuffer->futureCgmsInfo.analogProtectionSystem = UNKNOWNCONDITION;
	 context->XDSBuffer->futureCgmsInfo.analogSourceBit = AM_FALSE;
	 for(i=0;i<5;i++)
		 context->XDSBuffer->futurePkt1.type[i] = AM_XDSPROG_UNAVAILABLE;
	 context->XDSBuffer->futurePkt1.rating.rating = RATINGNOTAVAILABLE ;
	 context->XDSBuffer->futurePkt1.rating.fantasyViolence = AM_FALSE;
	 context->XDSBuffer->futurePkt1.rating.violence = AM_FALSE;
	 context->XDSBuffer->futurePkt1.rating.sexualSituations = AM_FALSE;
	 context->XDSBuffer->futurePkt1.rating.adultLanguage = AM_FALSE;
	 context->XDSBuffer->futurePkt1.rating.ssDialog = AM_FALSE;
	 context->XDSBuffer->futurePkt1.progLen.minute =0;
	 context->XDSBuffer->futurePkt1.progLen.hour = 0;
	 context->XDSBuffer->futurePkt1.timeShow.minute=0;
	 context->XDSBuffer->futurePkt1.timeShow.hour=0;
	 strcpy(context->XDSBuffer->futurePkt1.title," ");
	 context->XDSBuffer->futurePkt2.startTime.minute=0;
	 context->XDSBuffer->futurePkt2.startTime.hour=0;
	 context->XDSBuffer->futurePkt2.startDate=0;
	 context->XDSBuffer->futurePkt2.startMonth=0;
	 context->XDSBuffer->futurePkt2.audio.mainLang=XUNKNOWN;
	 context->XDSBuffer->futurePkt2.audio.mainType=UNKNOWNTYPE;
	 context->XDSBuffer->futurePkt2.audio.sapLang=XUNKNOWN;
	 context->XDSBuffer->futurePkt2.audio.sapType=UNKNOWNTYPE;
	 for(i=0;i<2;i++) 
	 {
		 context->XDSBuffer->futurePkt2.services[i]=AM_UNKNOWNSERV;
		 context->XDSBuffer->futurePkt2.serviceLanguage[i]=XUNKNOWN;
	 }
	 strcpy(context->XDSBuffer->futurePkt2.station.name," ");
	 strcpy(context->XDSBuffer->futurePkt2.station.chNum," ");
	 strcpy(context->XDSBuffer->futurePkt2.networkName," ");
	 for(i=0;i<8;i++) strcpy(context->XDSBuffer->futureProgDesc[i].title," ");
	 
	
	 strcpy(context->XDSBuffer->networkName," ");
	 strcpy(context->XDSBuffer->station.name," ");
	 strcpy(context->XDSBuffer->station.chNum," ");
	 strcpy(context->networkName," ");
	 strcpy(context->station.name," ");
	 strcpy(context->station.chNum," ");
	 context->XDSBuffer->delayTime.minute=0;
 	 context->XDSBuffer->delayTime.hour=0;
	 context->XDSBuffer->tsid=0;


	 context->XDSBuffer->currentDayTime.minute=0;
	 context->XDSBuffer->currentDayTime.hour=0;
	 context->XDSBuffer->currentDate=0;
	 context->XDSBuffer->currentMonth=0;
	 context->XDSBuffer->currentDay =0;
	 context->XDSBuffer->currentYear=0;
	 context->XDSBuffer->captureStartTime.minute=0;
	 context->XDSBuffer->captureStartTime.hour=0;
	 context->XDSBuffer->captureStartDate=0;
	 context->XDSBuffer->captureStartMonth=0;
	 context->XDSBuffer->captureProgLen.minute=0;
	 context->XDSBuffer->captureProgLen.hour=0;
	 for(i=0;i<32;i++)
	 {
		context->XDSBuffer->supplementInfo[i].field1=0;
	    context->XDSBuffer->supplementInfo[i].lineNum=0;
	 }
	 context->XDSBuffer->timeZoneOffset.offset=0;
	 context->XDSBuffer->timeZoneOffset.present=0;
	 context->XDSBuffer->oobChannelNumber=0;
	 context->XDSBuffer->chMap.chLow =0;
	 context->XDSBuffer->chMap.chHi =0;
	 context->XDSBuffer->chMap.version =0;
	 strcpy(context->XDSBuffer->chMap.chId," ");

	 context->XDSBuffer->weatherCodePresent=0;
	 strcpy(context->XDSBuffer->message," ");
	 strcpy(context->message," ");

	 AM_Reset_XDSCounters(context);
	 
	 /* Reset all the state variables */
	 context->currentXDSState = ILLEGALSTATE;
	 context->currentXDSClass = ILLEGALSTATE;
	 context->currentXDSType = ILLEGALTYPE;
	 context->startCaptureXDS = AM_FALSE;
	  
	 return;
}
/*
 *
 * XDS  Data Parse :
 *
 */
AM_ErrorCode_t AM_Decode_XDSData(UINT8 data1,UINT8 data2)
{
    AM_ErrorCode_t     err=AM_SUCCESS;
    AM_XDSState_t      _class,_type;
    UINT32	       i;

    /*
     * XDS Data Services
     *
     * TypeChar  - 01h to 7Fh
     * checksum  - 00h to 7Fh
     * InforChar - 20h to 7Fh (character)
     *           - 40h to 7Fh (Non Character)
     *
     *
     * Note that XDS Data only comes in Line21 Field 2
     *
     */

    if(data1==0x01 || data1==0x03 || data1==0x05 || data1==0x07 ||
            data1==0x09 || data1==0x0B || data1==0x0D)
    {
        /* Start State */
        pthread_mutex_lock(&xdsContext->lock);

        xdsContext->currentXDSState = STARTSTATE;
         AM_XDS_DBG("\n[startState]:");

        /* Find the XDS Class */
        xdsContext->currentXDSClass = AM_Get_XDSClass(data1);

        /* Find the XDS Type */
        xdsContext->currentXDSType = AM_Get_XDSType(data2,xdsContext->currentXDSClass);

        /* Reset XDS Counters */
        AM_Reset_XDSCounters(xdsContext);

	/* Start XDS checksum */
	xdsContext->currentXDSChecksum = data1 + data2;

        xdsContext->startCaptureXDS = AM_TRUE;
        pthread_mutex_unlock(&xdsContext->lock);
    }
    else if(data1==0x02 || data1==0x04 || data1==0x06 || data1==0x08 ||
                data1==0x0A || data1==0x0C || data1==0x0E)
    {
        /* Continue State */
         pthread_mutex_lock(&xdsContext->lock);

         AM_XDS_DBG("\n[continueState]:");

        /* Find the xds type - If this is not equal to the
         * xdsContext->currentXDSClass then this is not
         * valid xds data.So  reset evertyhing. */
        _class = AM_Get_XDSClass(data1);
        _type = AM_Get_XDSType(data2,_class);

        if(_class == xdsContext->currentXDSClass && _type == xdsContext->currentXDSType)
        {
            xdsContext->currentXDSState = CONTINUESTATE;
            xdsContext->startCaptureXDS = AM_TRUE;
	    AM_XDS_DBG("\n[matched xds class/type from previous]");
        }
        else
        {
            xdsContext->startCaptureXDS = AM_FALSE;
            xdsContext->currentXDSState = ILLEGALSTATE;
            xdsContext->currentXDSClass = ILLEGALSTATE;
            xdsContext->currentXDSType = ILLEGALTYPE;
            AM_Reset_XDSCounters(xdsContext);
	    AM_XDS_DBG("\n[Umatched xds class/type]");
	    AM_XDS_DBG("\n[Reset XDS from continue state]");
        }

        pthread_mutex_unlock(&xdsContext->lock);

    }
    else if(data1 == 0x0F)
    {
         /* End State */
         //AM_Dump_XDSDataInfo(xdsContext->XDSBuffer);//dump xds info for debug
	 err=AM_Handle_XDSEndState(data2,xdsContext->currentXDSType,xdsContext);
         AM_XDS_DBG("\n[endState]:");
    }
    else if(((data1 >= 0x20 && data1 <= 0x7F) || (data1 == 0x00)) &&
		   	((data2 >= 0x20 && data2 <= 0x7F) || (data2 == 0x00)) &&
            (xdsContext->startCaptureXDS))
    {
        /* 
         * XDS Informational Characters including end of string(See Table 11 of 
         * EIA608)
         * Igore the processing in case that both data are 0.
         */
	     if (!(data1 == 0x00 && data2 == 0x00))
	           err = AM_Handle_XDSInformationCharacters(data1,data2,xdsContext->currentXDSType,xdsContext);
     }

    return err;
}

/*
 *
 * AM_Handle_XDSInformationCharacters :
 *
 * Main handle dispatcher to handle the xds informational
 * characters.
 *
 */
AM_ErrorCode_t AM_Handle_XDSInformationCharacters(UINT8 data1,UINT8 data2,AM_XDSState_t state,AM_XDSContext_t *context)
{
    AM_ErrorCode_t err=AM_SUCCESS;

	/* update XDS checksum */
	xdsContext->currentXDSChecksum += data1;
	xdsContext->currentXDSChecksum += data2;

	switch(state)
	{
        /* Current / Future Class Type */
        case PINSTATE:
             AM_XDS_DBG("\n[ PINSTATE 0x%x]",data1);
             xds_handle_PINstate(data1,context);

             AM_XDS_DBG("\n[ PINSTATE 0x%x]",data2);
             xds_handle_PINstate(data2,context);
             break;

        case LENTIMESTATE:
             AM_XDS_DBG("\n[ lenState 0x%x]",data1);
             xds_handle_lentimestate(data1,context);

             AM_XDS_DBG("\n[ lenState 0x%x]",data2);
             xds_handle_lentimestate(data2,context);
             break;

        case PROGNAMESTATE:
             AM_XDS_DBG("\n[ progNameState 0x%x]",data1);
             xds_handle_prognamestate(data1,context);

             AM_XDS_DBG("\n[ progNameState 0x%x]",data2);
             xds_handle_prognamestate(data2,context);
             break;

        case PROGTYPESTATE:
             AM_XDS_DBG("\n[ progTypeState 0x%x]",data1);
	     xds_handle_progtypestate(data1,context);

             AM_XDS_DBG("\n[ progTypeState 0x%x]",data2);
	     xds_handle_progtypestate(data2,context);
            break;

        case PROGRATINGSTATE:
             AM_XDS_DBG("\n[ contentAdvisoryInfo 0x%x]",data1);
	     xds_handle_contentadvisorystate(data1,context);

             AM_XDS_DBG("\n[ contentAdvisoryInfo 0x%x]",data2);
	     xds_handle_contentadvisorystate(data2,context);
	     break;

	case AUDIOSERVICESSTATE:
	     AM_XDS_DBG("\n[ audioServicesState 0x%x]",data1);
	     xds_handle_audioservicesstate(data1,context);

	     AM_XDS_DBG("\n[ audioServicesState 0x%x]",data2);
	     xds_handle_audioservicesstate(data2,context);
	     break;

	case CAPTIONSERVICESSTATE:
	     AM_XDS_DBG("\n[ captionServicesState 0x%x]",data1);
	     xds_handle_captionservicesstate(data1,context);

             AM_XDS_DBG("\n[ captionServicesState 0x%x]",data2);
             xds_handle_captionservicesstate(data2,context);
            break;

        case ASPECTRATIOSTATE:
             AM_XDS_DBG("\n[ aspectRatioState 0x%x]",data1);
	     xds_handle_aspectratiostate(data1,context);

             AM_XDS_DBG("\n[ aspectRatioState 0x%x]",data2);
	     xds_handle_aspectratiostate(data2,context);
            break;

        case CGMSSTATE:
             AM_XDS_DBG("\n[ cgmsState 0x%x]",data1);
	     xds_handle_cgmsstate(data1,context);

             AM_XDS_DBG("\n[ cgmsState 0x%x]",data2);
	     xds_handle_cgmsstate(data2,context);
            break;

        case COMPPKT1STATE:
             AM_XDS_DBG("\n[ compPkt1State 0x%x]",data1);
	     xds_handle_comppkt1state(data1,context);

             AM_XDS_DBG("\n[ compPkt1State 0x%x]",data2);
	     xds_handle_comppkt1state(data2,context);
            break;

        case COMPPKT2STATE:
             AM_XDS_DBG("\n[ compPkt2State 0x%x]",data1);
	     xds_handle_comppkt2state(data1,context);

             AM_XDS_DBG("\n[ compPkt2State 0x%x]",data2);
	     xds_handle_comppkt2state(data2,context);
            break;

        case PROGDESCSTATE1:
        case PROGDESCSTATE2:
        case PROGDESCSTATE3:
        case PROGDESCSTATE4:
        case PROGDESCSTATE5:
        case PROGDESCSTATE6:
        case PROGDESCSTATE7:
        case PROGDESCSTATE8:
             AM_XDS_DBG("\n[ progDescState 0x%x]",data1);
	     xds_handle_progdescstate(data1,context);

             AM_XDS_DBG("\n[ progDescState 0x%x]",data2);
	     xds_handle_progdescstate(data2,context);
            break;

        /* Channel Info Class Types */
        case NETWORKNAMESTATE:
             AM_XDS_DBG("\n[ networkNameState 0x%x]",data1);
	     xds_handle_networknamestate(data1,context);

             AM_XDS_DBG("\n[ networkNameState 0x%x]",data2);
	     xds_handle_networknamestate(data2,context);
            break;

        case NATIVECHANNELSTATE:
             AM_XDS_DBG("\n[ nativeChannelState 0x%x]",data1);
	     xds_handle_nativechannelstate(data1,context);

             AM_XDS_DBG("\n[ nativeChannelState 0x%x]",data2);
	     xds_handle_nativechannelstate(data2,context);
            break;

        case TAPEDELAYSTATE:
             AM_XDS_DBG("\n[ tapeDelayState 0x%x]",data1);
	     xds_handle_tapedelaystate(data1,context);

             AM_XDS_DBG("\n[ tapeDelayState 0x%x]",data2);
	     xds_handle_tapedelaystate(data2,context);
            break;

        case  TSIDSTATE:
             AM_XDS_DBG("\n[ tsidState 0x%x]",data1);
	     xds_handle_TSIDstate(data1,context);

             AM_XDS_DBG("\n[ tsidState 0x%x]",data2);
	     xds_handle_TSIDstate(data2,context);
            break;

        /* Misc Class Types */
        case TIMEOFDAYSTATE:
             AM_XDS_DBG("\n[ timeOfDaystate 0x%x]",data1);
	     xds_handle_timeofdaystate(data1,context);

             AM_XDS_DBG("\n[ timeOfDaystate 0x%x]",data2);
	     xds_handle_timeofdaystate(data2,context);
            break;

        case IMPULSESTATE:
             AM_XDS_DBG("\n[ impulsestate 0x%x]",data1);
	     xds_handle_impulsestate(data1,context);

             AM_XDS_DBG("\n[ impulsestate 0x%x]",data2);
	     xds_handle_impulsestate(data2,context);
            break;

        case SUPPLEMENTSTATE:
             AM_XDS_DBG("\n[ audioServicesstate 0x%x]",data1);
	     xds_handle_supplementstate(data1,context);

             AM_XDS_DBG("\n[ audioServicesstate 0x%x]",data2);
	     xds_handle_supplementstate(data2,context);
            break;

        case LOCALTIMEZONESTATE:
             AM_XDS_DBG("\n[ localTimeZonestate 0x%x]",data1);
	     xds_handle_localtimezonestate(data1,context);

             AM_XDS_DBG("\n[ localTimeZonestate 0x%x]",data2);
	     xds_handle_localtimezonestate(data2,context);
            break;

        case OOBSTATE:
             AM_XDS_DBG("\n[ oobstate 0x%x]",data1);
	     xds_handle_OOBstate(data1,context);

             AM_XDS_DBG("\n[ oobstate 0x%x]",data2);
	     xds_handle_OOBstate(data2,context);
            break;

        case CHMAPPOINTERSTATE:
        case CHMAPHEADERSTATE:
        case CHMAPPACKETSTATE:
             AM_XDS_DBG("\n[ chMappingstate 0x%x]",data1);
	     xds_handle_CHMappingstate(data1,state,context);

             AM_XDS_DBG("\n[ chMappingstate 0x%x]",data2);
	     xds_handle_CHMappingstate(data2,state,context);
            break;

        /* Public Service Class */
        case WEATHERCODESTATE:
             AM_XDS_DBG("\n[ weatherCodestate 0x%x]",data1);
	     xds_handle_weathercodestate(data1,context);

             AM_XDS_DBG("\n[ weatherCodestate 0x%x]",data2);
	     xds_handle_weathercodestate(data2,context);
            break;

        case WEATHERMSGSTATE:
             AM_XDS_DBG("\n[ weatherMsgstate 0x%x]",data1);
	     xds_handle_weathermsgstate(data1,context);

             AM_XDS_DBG("\n[ weatherMsgstate 0x%x]",data2);
	     xds_handle_weathermsgstate(data2,context);
            break;

        default:
             AM_XDS_DBG("\n[ Error - Unknown state ]");
            //
            context->currentXDSState = ILLEGALSTATE;
            context->currentXDSType  = ILLEGALSTATE;
            context->startCaptureXDS = AM_FALSE;
	    AM_Reset_XDSCounters(context);
            // 
            break;
    }

	 AM_XDS_DBG("\n");

    return err;
}


/*
 *
 * AM_Handle_XDSEndState :
 *
 * Main handler/dispatcher that checks correctness of received data
 * once the "END" character has arrived, updates the stored XDS data,
 * and - if needed - does callbacks
 *
 */
AM_ErrorCode_t AM_Handle_XDSEndState(UINT8 checksum,AM_XDSState_t state,AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_SUCCESS;
	UINT8	calcSum;

	/* check wether receiving an XDS packet?
	 */
	if (!context->startCaptureXDS)
	{
		AM_XDS_DBG("\n%s: not in capture state!", __FUNCTION__); 
		err = AM_FAILURE;
		goto EXIT_POINT;
	}

	/* the calculated checksum should be the 7-bit 2's complement of the
	 * additive checksum of the packet from START to END
	 */
	context->currentXDSChecksum += 0x0F; /* END character */

	/* calculate the correct 2's complement checksum now */
	calcSum = (~context->currentXDSChecksum) + 1;
	calcSum &= 0x7F;

	/* was it a good checksum? */
	if (calcSum != checksum)
	{
	        AM_XDS_DBG("\n%s: bad checksum - received 0x%x, calculated 0x%x",__FUNCTION__, checksum, calcSum); 
		err = AM_FAILURE;
#if 0
		goto EXIT_POINT;
#endif
	}	
	
	AM_XDS_DBG("\n%s: good checksum - received 0x%x, calculated 0x%x",__FUNCTION__, checksum, calcSum); 

	/* now take appropriate action based on state 
	 */
	switch(state)
	{
        /* Current / Future Class Type */
        case PINSTATE:
	    AM_XDS_DBG("\n[ pinstate ]");
            err = xds_finish_PINstate(context);
            break;

        case LENTIMESTATE:
             AM_XDS_DBG("\n[ lenstate ]");
	     err = xds_finish_lentimestate(context);
            break;

        case PROGNAMESTATE:
             AM_XDS_DBG("\n[ progNamestate ]");
	     err = xds_finish_prognamestate(context);
            break;

        case PROGTYPESTATE:
             AM_XDS_DBG("\n[ progTypestate ]");
	     err = xds_finish_progtypestate(context);
            break;

        case PROGRATINGSTATE:
             AM_XDS_DBG("\n[ contentAdvisoryInfo ]");
	     err = xds_finish_contentadvisorystate(context);
            break;

        case AUDIOSERVICESSTATE:
             AM_XDS_DBG("\n[ audioServicesstate ]");
	     err = xds_finish_audioservicesstate(context);
            break;

        case CAPTIONSERVICESSTATE:
             AM_XDS_DBG("\n[ captionServicesstate ]");
	     err = xds_finish_captionservicesstate(context);
            break;

        case ASPECTRATIOSTATE:
             AM_XDS_DBG("\n[ aspectRatiostate ]");
	     err = xds_finish_aspectratiostate(context);
            break;

        case CGMSSTATE:
             AM_XDS_DBG("\n[ cgmsstate ]");
	     err = xds_finish_cgmsstate(context);
            break;

        case COMPPKT1STATE:
             AM_XDS_DBG("\n[ compPkt1state ]");
	     err = xds_finish_comppkt1state(context);
            break;

        case COMPPKT2STATE:
	    /* TODO - fill this in with full functionality */
             AM_XDS_DBG("\n[ compPkt2state ]");
	     err = xds_finish_comppkt2state(context);
            break;

        case PROGDESCSTATE1:
        case PROGDESCSTATE2:
        case PROGDESCSTATE3:
        case PROGDESCSTATE4:
        case PROGDESCSTATE5:
        case PROGDESCSTATE6:
        case PROGDESCSTATE7:
        case PROGDESCSTATE8:
             AM_XDS_DBG("\n[ progDescstate ]");
	     err = xds_finish_progdescstate(context);
            break;

        /* Channel Info Class Types */
        case NETWORKNAMESTATE:
             AM_XDS_DBG("\n[ networkNamestate ]");
	     err = xds_finish_networknamestate(context);
            break;

        case NATIVECHANNELSTATE:
             AM_XDS_DBG("\n[ nativeChannelstate ]");
	     err = xds_finish_nativechannelstate(context);
            break;

        case TAPEDELAYSTATE:
             AM_XDS_DBG("\n[ tapeDelaystate ]");
	     err = xds_finish_tapedelaystate(context);
            break;

        case  TSIDSTATE:
             AM_XDS_DBG("\n[ tsidstate ]");
	     err = xds_finish_TSIDstate(context);
            break;

        /* Misc Class Types */
        case TIMEOFDAYSTATE:
             AM_XDS_DBG("\n[ timeOfDaystate ]");
	     err = xds_finish_timeofdaystate(context);
            break;

        case IMPULSESTATE:
             AM_XDS_DBG("\n[ impulsestate ]");
	     err = xds_finish_impulsestate(context);
            break;

        case SUPPLEMENTSTATE:
             AM_XDS_DBG("\n[ audioServicesstate ]");
	     err = xds_finish_supplementstate(context);
            break;

        case LOCALTIMEZONESTATE:
             AM_XDS_DBG("\n[ localTimeZonestate ]");
	     err = xds_finish_localtimezonestate(context);
            break;

        case OOBSTATE:
             AM_XDS_DBG("\n[ oobstate ]");
	     err = xds_finish_OOBstate(context);
            break;

        case CHMAPPOINTERSTATE:
        case CHMAPHEADERSTATE:
        case CHMAPPACKETSTATE:
             AM_XDS_DBG("\n[ chMappingstate ]");
	     err = xds_finish_CHMappingstate(state,context);
            break;

        /* Public Service Class */

	/* TODO - parser here is a skeleton */
        case WEATHERCODESTATE:
             AM_XDS_DBG("\n[ weatherCodestate ]");
            err = xds_finish_weathercodestate(context);
            break;

        case WEATHERMSGSTATE:
             AM_XDS_DBG("\n[ weatherMsgstate ]");
            err = xds_finish_weathermsgstate(context);
            break;

        default:
             AM_XDS_DBG("\n[ Error - Unknown state ]");
    }

EXIT_POINT:

	/* we always end up here - good data or bad, we need to
	 * reset the XDS state machine
	 */
	//
	context->currentXDSState = ILLEGALSTATE;
	context->currentXDSType = ILLEGALTYPE;
	context->startCaptureXDS = AM_FALSE;
	AM_Reset_XDSCounters(context);
	AM_XDS_DBG("\n");

	return err;
}

void AM_Reset_XDSCounters(AM_XDSContext_t *context)
{
    context->pinParamCount= 0;
    context->lenParamCount= 0;
    context->progNameCount = 0;
    context->progTypeCount = 0;
    context->progRatingCount = 0;
    context->audioCount = 0;
    context->csCount = 0;
    context->aspectRatioCount=0;
    context->networkNameCount=0;
    context->nativeChannelCount=0;
    context->tapeDelayCount=0;
    context->tsidCount=0;
    context->tsidData =0;
    context->timeOfDayCount=0;
    context->impulseCount=0;
    context->supplementCount=0;
    context->localTimeZoneCount=0;
    context->oobCount=0;
    context->weatherCodeCount=0;
    context->weatherMessageCount=0;
    context->compPkt1Count=0;
    context->compPkt2Count=0;
    context->progDescCount=0;
    context->progDescLevel2Count=0;
    context->cgmsCount=0;
    context->chMapCount = 0;
    context->currentXDSChecksum = 0;
   
    memset(&xds_ca_info ,0,sizeof(atsc_content_advisory_dr_t));
    return;
}

/*
** set ntsc current major and monir numner,format as:
** MSB 4 btyes as major number,LSB 4 bytes as minor numner
** like as :0x20000 means NTSC channel 2-0
** this convience to query dvb.db database base on ntsc virtual
** channel numner,set by APP
*/
void AM_Set_XDSCurrentChanNumber(int ntscchannumber)
{
   ntsc_current_channumner = ntscchannumber;
}
void rst_vchip_rating()
{
	xdsContext->XDSBuffer->rating.rating = RATINGNOTAVAILABLE ;
	xdsContext->XDSBuffer->rating.fantasyViolence = AM_FALSE;
	xdsContext->XDSBuffer->rating.violence = AM_FALSE;
	xdsContext->XDSBuffer->rating.sexualSituations = AM_FALSE;
	xdsContext->XDSBuffer->rating.adultLanguage = AM_FALSE;
	xdsContext->XDSBuffer->rating.ssDialog = AM_FALSE;

}
void AM_Clr_Vchip_Event()
{
	rst_vchip_rating();
	xds_store_useproc_event(XDSCONTENTADVISORYEVENT);
}
/*
 * AM_Get_XDSClass :
 *
 * Returns the xds class.
 */
AM_XDSState_t AM_Get_XDSClass(UINT8 data)
{
    AM_XDSState_t state = ILLEGALSTATE;

    switch(data)
    {
        case 0x01:
        case 0x02:
            state = CURRENTCLASSSTATE;
	    AM_XDS_DBG("[CURRENTCLASSSTATE]");
            break;

        case 0x03:
        case 0x04:
            state = FUTURECLASSSTATE;
	    AM_XDS_DBG("[FUTURECLASSSTATE]");
            break;

        case 0x05:
        case 0x06:
            state = CHANNELCLASSSTATE;
	    AM_XDS_DBG("[channelClassState]");
            break;

        case 0x07:
        case 0x08:
            state = MISCCLASSSTATE;
	    AM_XDS_DBG("[miscClassState]");
            break;

        case 0x09:
        case 0x0A:
            state = PUBLICCLASSSTATE;
	    AM_XDS_DBG("[publicClassState]");
            break;

        case 0x0B:
        case 0x0C:
            state = RESERVEDCLASSSTATE;
	    AM_XDS_DBG("[reservedClassState]");
            break;

        case 0x0D:
        case 0x0E:
            state = UNDEFINEDCLASSSTATE;
	    AM_XDS_DBG("[undefinedClassState]");
            break;

        case 0x0F:
        default:
            state = ALLCLASSSTATE;
	    AM_XDS_DBG("[allClassState]");
            break;
    }

    return state;
}

/*
 * AM_Get_XDSType :
 *
 * Returns the XDS Type.
 *
 */
AM_XDSState_t AM_Get_XDSType(UINT8 data,AM_XDSState_t currState)
{
    AM_XDSState_t type = ILLEGALTYPE;

    switch(currState)
    {
        case CURRENTCLASSSTATE:
        case FUTURECLASSSTATE:
            type = xds_get_classtype(data);
            break;

        case CHANNELCLASSSTATE:
            type = xds_get_channeltype(data);
            break;

        case MISCCLASSSTATE:
            type = xds_get_misctype(data);
            break;

        case PUBLICCLASSSTATE:
            type = xds_get_publictype(data);
            break;

        case RESERVEDCLASSSTATE:
        case UNDEFINEDCLASSSTATE:
        case ALLCLASSSTATE:
        default:
            type = ILLEGALTYPE;
            break;
    }

    return type;
}


/*
 * xds_get_classtype :
 *
 * Returns the type for Current/Future Class
 */
static AM_XDSState_t xds_get_classtype(UINT8  data)
{
    switch(data)
    {
        case 0x01:
		AM_XDS_DBG("[PINSTATE]");
		return PINSTATE;

        case 0x02:
		AM_XDS_DBG("[lenState]");
		return LENTIMESTATE;

        case 0x03:
		AM_XDS_DBG("[progNameState]");
		return PROGNAMESTATE;

        case 0x04:
		AM_XDS_DBG("[progTypeState]");
		return PROGTYPESTATE;

        case 0x05:
		AM_XDS_DBG("[contentAdvisoryInfo]");
		return PROGRATINGSTATE;

        case 0x06:
		AM_XDS_DBG("[audioServicesState]");
		return AUDIOSERVICESSTATE;

        case 0x07:
		AM_XDS_DBG("[captionServicesState]");
		return CAPTIONSERVICESSTATE;

        case 0x08:
		AM_XDS_DBG("[cgmsState]");
		return CGMSSTATE;

        case 0x09:
		AM_XDS_DBG("[aspectRatioState]");
		return ASPECTRATIOSTATE;

        case 0x0C:
		AM_XDS_DBG("[compPkt1State]");
		return COMPPKT1STATE;

        case 0x0D:
		AM_XDS_DBG("[compPkt2State]");
		return COMPPKT2STATE;
			
        case 0x10:
		AM_XDS_DBG("[progDescState1]");
		return PROGDESCSTATE1;

        case 0x11:
		AM_XDS_DBG("[progDescState2]");
		return PROGDESCSTATE2;

        case 0x12:
		AM_XDS_DBG("[progDescState3]");
		return PROGDESCSTATE3;

        case 0x13:
		AM_XDS_DBG("[progDescState4]");
		return PROGDESCSTATE4;

        case 0x14:
		AM_XDS_DBG("[progDescState5]");
		return PROGDESCSTATE5;

        case 0x15:
		AM_XDS_DBG("[progDescState6]");
		return PROGDESCSTATE6;

        case 0x16:
		AM_XDS_DBG("[progDescState7]");
		return PROGDESCSTATE7;

        case 0x17:
		AM_XDS_DBG("[progDescState8]");
		return PROGDESCSTATE8;

        default:
		AM_XDS_DBG("[illegalType]");
		return ILLEGALTYPE;
    }
}

/*
 * xds_get_channeltype :
 *
 * Returns the type for Channel Class.
 */
static AM_XDSState_t xds_get_channeltype(UINT8 data)
{
    switch(data)
    {
        case 0x01:
		AM_XDS_DBG("[networkNameState]");
		return NETWORKNAMESTATE;

        case 0x02:
		AM_XDS_DBG("[nativeChannelState]");
		return NATIVECHANNELSTATE;

        case 0x03:
		AM_XDS_DBG("[tapeDelayState]");
		return TAPEDELAYSTATE;

        case 0x04:
		AM_XDS_DBG("[tsidState]");
		return TSIDSTATE;

        default:
		AM_XDS_DBG("[illegalType]");
		return ILLEGALTYPE;
    }
}

/*
 * xds_get_misctype:
 *
 * Returns the Type fro Misc Class.
 */
static AM_XDSState_t xds_get_misctype(UINT8   data)
{
    switch(data)
    {
        case 0x01:
             AM_XDS_DBG("[timeOfDayState]");
	     return TIMEOFDAYSTATE;

        case 0x02:
             AM_XDS_DBG("[impulseState]");
	     return IMPULSESTATE;

        case 0x03:
             AM_XDS_DBG("[audioServicesState]");
	     return SUPPLEMENTSTATE;

        case 0x04:
             AM_XDS_DBG("[localTimeZoneState]");
	     return LOCALTIMEZONESTATE;

        case 0x40:
             AM_XDS_DBG("[oobState]");
	     return OOBSTATE;

        case 0x41:
             AM_XDS_DBG("[chMappingState]");
	     return CHMAPPOINTERSTATE;

        case 0x42:
             AM_XDS_DBG("[chMappingState]");
	     return CHMAPHEADERSTATE;

        case 0x43:
             AM_XDS_DBG("[chMappingState]");
	     return CHMAPPACKETSTATE;

        default:
             AM_XDS_DBG("[illegalType]");
	     return ILLEGALTYPE;
    }
}


/*
 * xds_get_publictype:
 *
 * Returns the type for Public Class
 */
static AM_XDSState_t xds_get_publictype(UINT8 data)
{
    switch(data)
    {
        case 0x01:
             AM_XDS_DBG("[weatherCodeState]");
	     return WEATHERCODESTATE;

        case 0x02:
             AM_XDS_DBG("[weatherMsgState]");
	     return WEATHERMSGSTATE;

        default:
             AM_XDS_DBG("[illegalType]");
	     return ILLEGALTYPE;
    }
}

/*
 * xds_handle_PINstate : 
 *
 * Parses and Stores the Prog Identification Number
 *
 */
static void xds_handle_PINstate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSTime_t	tempTime;
	UINT32		date,month;
	AM_XDSInfo_t 	*info = (AM_XDSInfo_t *) NULL;

	/* do nothing with stuffing characters 
	 */
	if (data == 0x00)
		return;

	info = context->XDSBuffer;
	context->pinParamCount++;
	
	switch(context->pinParamCount)
	{
		case 1:
			tempTime.minute = data & 0x3F;
			if(context->currentXDSClass== CURRENTCLASSSTATE) info->startTime.minute = tempTime.minute;
			else if (context->currentXDSClass== FUTURECLASSSTATE) info->futureStartTime.minute = tempTime.minute;				
			 AM_XDS_DBG("[Min=%d]",tempTime.minute);
			break;

		case 2:
			tempTime.hour = data & 0x1F;
			if(context->currentXDSClass== CURRENTCLASSSTATE) info->startTime.hour = tempTime.hour;
			else if (context->currentXDSClass== FUTURECLASSSTATE) info->futureStartTime.hour = tempTime.hour;
			 AM_XDS_DBG("[Hr=%d]",tempTime.hour);
			break;

		case 3:
			date = data & 0x1F;
			if(context->currentXDSClass== CURRENTCLASSSTATE) info->startDate = date;
			else if (context->currentXDSClass== FUTURECLASSSTATE) info->futureStartDate = date;
			 AM_XDS_DBG("[Date=%d]",date);
			break;

		case 4:
			month = data & 0xF;
			if(context->currentXDSClass== CURRENTCLASSSTATE) info->startMonth = month;
			else if (context->currentXDSClass== FUTURECLASSSTATE)	info->futureStartMonth = month;
			 AM_XDS_DBG("[Month=%d]",month);
			break;

		default:
			
			AM_XDS_DBG("[bad pinParamCount=%d]", context->pinParamCount); 
			break;
	}
	
	return;
}	

static AM_ErrorCode_t xds_finish_PINstate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t		err = AM_FAILURE;

	if(context->pinParamCount == PIN_PARAMS)
	{       
                /*save info acquired from xds into database for application */
	        xds_store_useproc_event(XDSPROGRAMTIMEEVENT);
		err = AM_SUCCESS;
	}
	
        return err;
}	

/*
 * xds_handle_lentimestate :
 *
 * Parses and calculates Length and Time in show
 *
 */
static void xds_handle_lentimestate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSTime_t 	temp;
	AM_XDSInfo_t 	*info = (AM_XDSInfo_t *) NULL;
	UINT32  	x;
	
	info = context->XDSBuffer;
	context->lenParamCount++;

	switch(context->lenParamCount)
	{
		case 1:
			temp.minute = data & 0x3F;
			if(context->currentXDSClass== CURRENTCLASSSTATE)  info->progLen.minute = temp.minute;
			else if(context->currentXDSClass== FUTURECLASSSTATE)  info->futureProgLen.minute = temp.minute;
			 AM_XDS_DBG("[Min=%d]",temp.minute);
			break;

		case 2:
			temp.hour = data & 0x3F;
			if(context->currentXDSClass== CURRENTCLASSSTATE) info->progLen.hour = temp.hour;
			else if(context->currentXDSClass== FUTURECLASSSTATE) info->futureProgLen.hour = temp.hour;
			 AM_XDS_DBG("[Hr=%d]",temp.hour);
			break;
			
		case 3:
			temp.minute = data & 0x3F;
			if(context->currentXDSClass== CURRENTCLASSSTATE)  info->timeElapsed.minute = temp.minute;
			else if(context->currentXDSClass== FUTURECLASSSTATE)  info->futureTimeElapsed.minute = temp.minute;
			 AM_XDS_DBG("[elapsedMin=%d]",temp.minute);
			break;

		case 4:
			temp.hour = data & 0x3F;
			if(context->currentXDSClass== CURRENTCLASSSTATE)  info->timeElapsed.hour = temp.hour;
			else if(context->currentXDSClass== FUTURECLASSSTATE) info->futureTimeElapsed.hour = temp.hour;
			 AM_XDS_DBG("[elapsedHr=%d]",temp.minute);
			break;
			
		case 5:
			x = data & 0x3F;
			if(context->currentXDSClass== CURRENTCLASSSTATE) info->timeElapsedSec = x;
			else if(context->currentXDSClass== FUTURECLASSSTATE) info->futureTimeElapsedSec = x;
			 AM_XDS_DBG("[elapsedSec=%d]",x);
			break;
			
		case 6:
		default:
			break;
	}

	return;
}

static AM_ErrorCode_t xds_finish_lentimestate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t		err = AM_FAILURE;
	
	/* special case here - the time may be 2, 4, or 6 bytes long
	 */
	if( ( context->lenParamCount == LEN_PARAMS ) ||
		( context->lenParamCount == (LEN_PARAMS - 2) ) ||
		( context->lenParamCount == (LEN_PARAMS - 4) ) )
	{
            /*save info acquired from xds into database for application */
	    xds_store_useproc_event(XDSPROGRAMTIMEEVENT);
	    err = AM_SUCCESS;
	}
	
	return err;
}


/*
 * xds_handle_prognamestate : 
 *
 * Stores the Prog Name
 *
 */
static void xds_handle_prognamestate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSInfo_t	*info = (AM_XDSInfo_t *) NULL;
	char 	c;
	UINT32  index=0;
	
	info = context->XDSBuffer;

	if(data >= 0x20 && data <= 0x7F)
	{
		c = data;
		index = context->progNameCount;
		/* don't overrun the end of the buffer!
		 */
		if (index < PROG_NAME_PARAMS)
		{
			context->progTitle[index]=c;

			/* if buffer is not full, make sure that
			 * the string is null terminated
			 */
			if(index+1 < PROG_NAME_PARAMS)
				context->progTitle[index+1]='\0';
		}
		
		 AM_XDS_DBG("[%c %x]\n",c,data);
	}

	context->progNameCount++;
	
        return;
}

static AM_ErrorCode_t xds_finish_prognamestate(AM_XDSContext_t *context)
{
	AM_XDSInfo_t		*info = (AM_XDSInfo_t *) NULL;
	AM_ErrorCode_t		err = AM_FAILURE;

	info = context->XDSBuffer;

	/* length should be in range 2 - PROG_NAME_PARAMS)
	 */
	if((context->progNameCount > 1) &&
       (context->progNameCount <= PROG_NAME_PARAMS))
	{
		/* Call userproc if registered */
		if(context->currentXDSClass == CURRENTCLASSSTATE)
			strcpy(info->progTitle,context->progTitle);
		else if(context->currentXDSClass == FUTURECLASSSTATE)
			strcpy(info->futureProgTitle,context->progTitle);
		/*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSPROGRAMNAMEEVENT);
		err = AM_SUCCESS;
	}

	return err;
}

/*
 * xds_handle_progtypestate : 
 *
 * Parses and stores prog type 
 */
static void xds_handle_progtypestate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSProgType_t temp;
	AM_XDSInfo_t		*info = (AM_XDSInfo_t *) NULL;
	info = context->XDSBuffer;

	/* don't overrun the program type buffer!
	 */
	if (context->progTypeCount < PROG_TYPE_PARAMS)
	{
		temp = xds_get_progtype(data);
		switch(temp)
		{
			case AM_XDSPROG_EDU:
			case AM_XDSPROG_ENTERTAINEMENT:
			case AM_XDSPROG_MOVIE:
			case AM_XDSPROG_NEWS:
			case AM_XDSPROG_RELIGIOUS:
			case AM_XDSPROG_SPORTS:
			case AM_XDSPROG_OTHERS:
				/* Basic Prog Group */
				if(context->currentXDSClass== CURRENTCLASSSTATE) 
				{
					info->progType[context->progTypeCount].basicType = temp;
				}
				else if(context->currentXDSClass== FUTURECLASSSTATE)
				{
					info->futureValidTypes++;
					info->futureProgType[context->progTypeCount].basicType = temp;
				}
				 AM_XDS_DBG("[Basic->%s]",xds_get_programtypestring(temp));
				break;
				
			default:
				/* Detail Prog Group */
				if(context->currentXDSClass== CURRENTCLASSSTATE)
				{
					info->progType[context->progTypeCount].detailType = temp;
				}
				else if(context->currentXDSClass== FUTURECLASSSTATE)
				{
					info->futureValidTypes++;
					info->futureProgType[context->progTypeCount].detailType = temp;
				}
				 AM_XDS_DBG("[Detail->%s]",xds_get_programtypestring(temp));
				break;
		}
	}
	context->progTypeCount++;

	return;
}

static AM_ErrorCode_t xds_finish_progtypestate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t		err = AM_FAILURE;

	if((context->progTypeCount > 1) &&
	   (context->progTypeCount == PROG_TYPE_PARAMS))
	{
	       /*save info acquired from xds into database for application */	
		xds_store_useproc_event(XDSPROGRAMTYPEEVENT);
		err = AM_SUCCESS;
	}
	return err;
}

/*
 * xds_handle_contentadvisorystate : 
 *
 * Parses and stores the Prog Ratings
 */

static void xds_handle_contentadvisorystate(UINT8 data,AM_XDSContext_t *context)
{
	UINT8		tData=0,tData1=0;
	UINT32		caData=0;
	AM_XDSProgRating_t	temp;
	AM_XDSInfo_t	*info = (AM_XDSInfo_t *) NULL;
	
	/* Transport of Content Advisory Information accoring to EIA-744A */
	//
	info = context->XDSBuffer;
	context->progRatingCount++;
	/*
	 * Reset TV-PG dimension information before processing rating.
	 */
	if(context->currentXDSClass== CURRENTCLASSSTATE)
	{
		info->rating.ssDialog         = AM_FALSE;
		info->rating.adultLanguage    = AM_FALSE;
		info->rating.sexualSituations = AM_FALSE;
		info->rating.violence         = AM_FALSE;
		info->rating.fantasyViolence  = AM_FALSE;
	}
	else if(context->currentXDSClass== FUTURECLASSSTATE)
	{
		info->futureRating.ssDialog         = AM_FALSE;
		info->futureRating.adultLanguage    = AM_FALSE;
		info->futureRating.sexualSituations = AM_FALSE;
		info->futureRating.violence         = AM_FALSE;
		info->futureRating.fantasyViolence  = AM_FALSE;
	}

	switch(context->progRatingCount)
	{
		case 1:
			/* First Byte of Data */
			tData = (data & 0x18) >> 3 ;  /* Bit a1 a0 */
			switch(tData)
			{
				case 0:
				case 2:
					/* MPAA Rating System */
					tData = data & 0x07;
					temp = xds_get_mpaaprograting(tData);
					if(context->currentXDSClass== CURRENTCLASSSTATE)
 						 info->rating.rating = temp;
					else if(context->currentXDSClass== FUTURECLASSSTATE)
						 info->futureRating.rating = temp;
					context->contentAdvisoryWait = AM_FALSE;  
					AM_XDS_DBG("MPAA->%s",xds_get_programratingstring(temp));
					break;
					
				case 1:
				case 3:
					context->contentAdvisoryPrevData = data; /* Store this data */
					context->contentAdvisoryWait = AM_TRUE;  /* have to parse the 2nd Byte to get complete info */
 				    break;
			}
			break;

		case 2:
			/* 2nd Byte of Data */
			if(context->contentAdvisoryWait)
			{
				tData = (context->contentAdvisoryPrevData & 0x18) >> 3;
				switch(tData)
				{
					case 0:
					case 2:
						/* MPAA Rating Nothing to do now */
						AM_XDS_DBG("MPAA->NotUsed");
						break;

					case 1:
						/* US TV Parental Guidance */
						tData = (data & 0x07);
						temp = xds_get_ustvprograting(tData);
						if(context->currentXDSClass== CURRENTCLASSSTATE) 
                                                	info->rating.rating = temp;
						else if(context->currentXDSClass== FUTURECLASSSTATE) 
                           				info->futureRating.rating = temp;
             					AM_XDS_DBG("USTV->%s",xds_get_programratingstring(temp));

						tData = (context->contentAdvisoryPrevData & 0x20) >> 5;
						if(tData == 0x1)
						{
							/* Bit b5 of byte 1 applies only if rating is TV_PG or TV_14 */
							if(temp == TV_PG || temp == TV_14)
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) info->rating.ssDialog = AM_TRUE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
										info->futureRating.ssDialog = AM_TRUE;
								 AM_XDS_DBG("[ssDialog]");
							}
							else
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) info->rating.ssDialog = AM_FALSE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
										info->futureRating.ssDialog = AM_FALSE;
							}
						}

						tData = (data & 0x8) >> 3;
						if(tData == 0x1)
						{
							/* bit b3 of byte 2 applies only if rating is TV_PG or TV_14 or TV_MA */
							if(temp == TV_PG || temp == TV_14 || temp == TV_MA)	
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
										info->rating.adultLanguage = AM_TRUE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
										info->futureRating.adultLanguage = AM_TRUE;
								 AM_XDS_DBG("[adultLang]");
							}
							else
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
										info->rating.adultLanguage = AM_FALSE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
										info->futureRating.adultLanguage = AM_FALSE;
							}
						}

						tData = (data & 0x10) >> 4;
						if(tData == 0x1)
						{
							/* bit b4 of byte 2 applies only if rating is TV_PG/TV_14 or TV_MA */
							if(temp == TV_PG || temp == TV_14 || temp == TV_MA)	
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
										info->rating.sexualSituations = AM_TRUE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
										info->futureRating.sexualSituations = AM_TRUE;
								 AM_XDS_DBG("[ss]");
							}
							else
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
										info->rating.sexualSituations = AM_FALSE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
										info->futureRating.sexualSituations = AM_FALSE;
							}
						}

						tData = (data & 0x20) >> 5;
						if(tData == 0x1)
						{
							/* bit b5 of byte 2 applies only if rating is TV_PG/TV_14/TV_MA*/
							if(temp == TV_PG || temp == TV_14 || temp == TV_MA)	
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
									info->rating.violence = AM_TRUE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
									info->futureRating.violence = AM_TRUE;
								 AM_XDS_DBG("[violence]");
							}
							else
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
									info->rating.violence = AM_FALSE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
									info->futureRating.violence = AM_FALSE;
							}
						}
						
						tData = (data & 0x20) >> 5;
						if(tData == 0x1)
						{
							/* bit b5 of byte 2 applies only if rating is TV_Y7 */
							if(temp == TV_Y7)
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
									info->rating.fantasyViolence = AM_TRUE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
									info->futureRating.fantasyViolence = AM_TRUE;
								 AM_XDS_DBG("[fantasyViolence]");
							}
							else
							{
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
									info->rating.fantasyViolence = AM_FALSE;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
									info->futureRating.fantasyViolence = AM_FALSE;
							}
						}
						
						break;

					case 3:
						/* bit b6 from Byte 1 = a2 */
						tData = (context->contentAdvisoryPrevData  & 0x20) >> 5;
						/* bit b3 fro Byte 2  = a3 */
						tData1 = (data & 0x08) >> 2;
						tData = tData | tData1;   /* Combine to get a3 a2 */
						switch(tData)
						{
							case 0:
								/* Canadian English Language Rating System */
								tData = data & 0x7;
								temp = xds_get_celprograting(tData);
								if(context->currentXDSClass== CURRENTCLASSSTATE) 
						                    info->rating.rating = temp;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
								    info->futureRating.rating = temp;
								 AM_XDS_DBG("CEL->%s",xds_get_programratingstring(temp));
								break;
								
							case 1:
								/* Canadian French Language Rating System */
								tData = data & 0x7;
								temp = xds_get_cflprograting(tData);
								if(context->currentXDSClass== CURRENTCLASSSTATE)
                                                                    info->rating.rating = temp;
								else if(context->currentXDSClass== FUTURECLASSSTATE) 
								    info->futureRating.rating = temp;
								AM_XDS_DBG("CFL->%s",xds_get_programratingstring(temp));
								break;
								
							case 2:
							case 3:
								/* Reserved for non US & non Canadian system */
								 AM_XDS_DBG("ResevedRating");
							
								break;

							default:
								break;
						}
						break;

					default:
						break;
				}
				
				context->contentAdvisoryWait = AM_FALSE;  
			}
			break;

		default:
			break;
	}
	
	return;
}


static AM_ErrorCode_t xds_finish_contentadvisorystate(AM_XDSContext_t *context)
{
	UINT8			tData=0,tData1=0;
	UINT32			caData=0;
	static UINT32		lastTime = 0;
	UINT32			curTime = 0;
	/*AM_XDSProgRating_t	temp;*/
	AM_XDSInfo_t		*info = (AM_XDSInfo_t *) NULL;
	AM_ErrorCode_t		err = AM_FAILURE;
	
	/* Content Advisory Information according to EIA-744A */
	if(context->progRatingCount == PROG_RATING_PARAMS)
	{
                /*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSCONTENTADVISORYEVENT);
		err = AM_SUCCESS;
	}
	//curTime = Demux_GetSTC32(displayUnit_main);
	if (lastTime != 0)
		caData = curTime - lastTime;
	lastTime = curTime;
	 
	{
	   	//AM_XDS_DBG(" -> delta %u mS\n", ((caData * 1000) / STC_TICKS_PER_SECOND));
	}
	return err;
}

/*
 * xds_handle_audioservicesstate : 
 *
 * Parses and stores the audio info in XDS
 */

static void xds_handle_audioservicesstate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSLanguage_t 	tL;
	AM_XDSAudioType_t	tA;
	AM_XDSInfo_t		*info = (AM_XDSInfo_t *) NULL;
	
	info = context->XDSBuffer;
	context->audioCount++;

	tL = xds_get_audiolanguage(data & 0x38);
	tA = xds_get_mainaudio(data & 0x07);

	switch(context->audioCount)
	{
		case 1:
			if(context->currentXDSClass== CURRENTCLASSSTATE)
			{
				info->audio.mainLang = tL;
				info->audio.mainType = tA;
			}
			else if(context->currentXDSClass== FUTURECLASSSTATE)
			{
				info->futureAudio.mainLang = tL;
				info->futureAudio.mainType = tA;
			}
			 AM_XDS_DBG("[Main->%s]",xds_get_languagestring(tL));
			 AM_XDS_DBG("[Type->%s]",xds_get_audiotypestring(tA));
			break;

		case 2:
			if(context->currentXDSClass== CURRENTCLASSSTATE)
			{
				info->audio.sapLang = tL;
				info->audio.sapType = tA;
			}
			else if(context->currentXDSClass== FUTURECLASSSTATE)
			{
				info->futureAudio.sapLang = tL;
				info->futureAudio.sapType = tA;
			}
			 AM_XDS_DBG("[Sec->%s]",xds_get_languagestring(tL));
			 AM_XDS_DBG("[Type->%s]",xds_get_audiotypestring(tA));
			break;
			
		default:
			break;
	}

	return;
}

static AM_ErrorCode_t xds_finish_audioservicesstate(AM_XDSContext_t *context)
{
	//AM_XDSLanguage_t 	tL;
	/*AM_XDSAudioType_t	tA;*/
	AM_XDSInfo_t	*info = (AM_XDSInfo_t *) NULL;
	AM_ErrorCode_t	err = AM_FAILURE;
	
	info = context->XDSBuffer;

	if(context->audioCount == AUDIO_PARAMS)
	{
		/*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSAUDIOSERVICESEVENT);
		err = AM_SUCCESS;
	}

	return err;
}

/*
 *  xds_handle_captionservicesstate
 */
static void  xds_handle_captionservicesstate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSLanguage_t	lang;
	AM_XDSCaptionServices_t serv;
	AM_XDSInfo_t		*info = (AM_XDSInfo_t *) NULL;
	
	info = context->XDSBuffer;

	if (context->csCount < CS_PARAMS)
	{
		lang = xds_get_audiolanguage(data & 0x38);
		serv = xds_get_ccservicetype(data & 0x07);
		if(context->currentXDSClass== CURRENTCLASSSTATE)
		{
			info->service[context->csCount] = serv;
			info->serviceLang[context->csCount] = lang;
		}
		else if(context->currentXDSClass== FUTURECLASSSTATE)
		{
			info->futureService[context->csCount] = serv;
			info->futureServiceLang[context->csCount] = lang;
		}
	}

	context->csCount++;

	return;
}

static AM_ErrorCode_t xds_finish_captionservicesstate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t		err = AM_FAILURE;
	
	/* captions services count should be in range 2-CS_PARAMS
	 */
	if((context->csCount > 1) &&
	   (context->csCount <= CS_PARAMS))
	{
		/*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSCAPTIONSERVICESEVENT);
		err = AM_SUCCESS;
	}

	return err;
}

/* 
 * xds_handle_aspectratiostate : 
 */

static void xds_handle_aspectratiostate(UINT8 data,AM_XDSContext_t *context)
{
	UINT32 		temp;
	AM_XDSInfo_t	*info = (AM_XDSInfo_t *) NULL;

	info = context->XDSBuffer;
	context->aspectRatioCount++;

	switch(context->aspectRatioCount)
	{
		case 1:
			temp = (data & 0x3F) + 22 ;
			if(context->currentXDSClass== CURRENTCLASSSTATE) info->aspectRatio.startLine = temp;	
			else if(context->currentXDSClass== FUTURECLASSSTATE) info->futureAspectRatio.startLine = temp;
			 AM_XDS_DBG("[Start=%d]",temp);
			break;
			
		case 2:
			temp = 262 - (data & 0x3F); 
			if(context->currentXDSClass== CURRENTCLASSSTATE) 
			{
				info->aspectRatio.endLine = temp;	
				temp = info->aspectRatio.endLine - info->aspectRatio.startLine;
				if(temp >= 173 && temp <= 180)
					info->aspectRatio.ratio = VISTAVISION;
				if(temp >= 136 && temp <= 145)
					info->aspectRatio.ratio =CINEMASCOPE ;
				else
					info->aspectRatio.ratio = UNKNOWNAR;
			}
			else if(context->currentXDSClass== FUTURECLASSSTATE)
			{
				info->futureAspectRatio.endLine = temp;
				if(temp >= 173 && temp <= 180)
					info->futureAspectRatio.ratio = VISTAVISION;
				if(temp >= 136 && temp <= 145)
					info->futureAspectRatio.ratio = CINEMASCOPE;
				else
					info->futureAspectRatio.ratio = UNKNOWNAR;
			}
			 AM_XDS_DBG("[AR]");
			break;
			
		case 3:
			temp = data & 0x01 ;
			if(context->currentXDSClass== CURRENTCLASSSTATE)
				info->aspectRatio.videoSqueezed = (temp ? AM_TRUE : AM_FALSE);
			else if(context->currentXDSClass == FUTURECLASSSTATE)
				info->futureAspectRatio.videoSqueezed = (temp ? AM_TRUE : AM_FALSE);
			break;
			
		case 4:
		default:
			break;
	}
	
	return;
}

static AM_ErrorCode_t xds_finish_aspectratiostate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t		err = AM_FAILURE;
	
        /* either 2 or 4 parameters should be seen
	 * here - if no match, we have an error condition
	 */
	if((context->aspectRatioCount == 2) ||
	   (context->aspectRatioCount == AR_PARAMS))
	{
                /*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSASPECTRATIOEVENT);
		err = AM_SUCCESS;
	}

	return err;
}

/*
 * xds_handle_cgmsstate :
 *
 *
 */
static void xds_handle_cgmsstate(UINT8 data,AM_XDSContext_t *context)
{

	AM_XDSInfo_t 	*info = (AM_XDSInfo_t *) NULL;
	UINT8		tData=0,tData1=0;
	AM_CgmsInfo_t	temp = UNKNOWNCONDITION;

	/* Copy Generation Management System : CGMS EIA 702 */
	info = context->XDSBuffer;
	context->cgmsCount++;

	if(context->currentXDSClass == CURRENTCLASSSTATE)
		info->cgmsInfoPresent = AM_TRUE;
	else if(context->currentXDSClass == FUTURECLASSSTATE)
		info->futureCgmsInfoPresent = AM_TRUE;

	switch(context->cgmsCount)
	{
		case 1:
			/* 1st Byte */

			/* ASB */ 
			tData = data & 0x1;
			if(context->currentXDSClass == CURRENTCLASSSTATE) info->cgmsInfo.analogSourceBit = tData;
			else if(context->currentXDSClass == FUTURECLASSSTATE) info->futureCgmsInfo.analogSourceBit = tData;
			
			/* Copy control System Information */
			tData = (data & 0x18) >>3;
			switch(tData)
			{
				case 0:
					temp = COPYPERMITTED;
					break;
					
				case 1:
					temp = INVALIDCONDITION;
					break;
					
				case 2:
					temp = ONEGENERATIONCOPY;
					break;
					
				case 3:
					temp = COPYNOTPERMITTED;
					break;

				default:
					break;
			}
			
			if(context->currentXDSClass == CURRENTCLASSSTATE) 
					info->cgmsInfo.copyControl = temp;
			else if(context->currentXDSClass == FUTURECLASSSTATE) 
				    info->futureCgmsInfo.copyControl = temp;

			/* Analog Protection System Information */
			tData = (data & 0x8) >> 3 ;
			tData1 = (data & 0x10) >> 4;
			if(tData != 0 && tData1 != 0)
			{
				/* Only if either CGMS-A bit are not 0 we have APs
				 * valid Info */
				tData = (data & 0x06) >>1;
				switch(tData)
				{
					case 0:
						temp = NOAPS;
						break;

					case 1:
						temp = PSPONSPLITBURSTOFF;	
						break;

					case 2:
						temp = PSPON2LINESPLITBURSTON;	
						break;

					case 3:
						temp = PSPON4LINESPLITBURSTON;	
						break;

					default:
						break;
				}
			if(context->currentXDSClass == CURRENTCLASSSTATE) 
	      		    info->cgmsInfo.analogProtectionSystem = temp;
			else if(context->currentXDSClass == FUTURECLASSSTATE) 
			    info->futureCgmsInfo.analogProtectionSystem = temp;
			}

			break;

		case 2:
			/* 2ndByte is Reserved */
			break;

		default:
			break;
	}

	return;
}

static AM_ErrorCode_t xds_finish_cgmsstate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_FAILURE;

	/* Copy Generation Management System : CGMS EIA 702 */
	if(context->cgmsCount == CGMS_PARAMS)
	{
                /*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSCGMSEVENT);
		err = AM_SUCCESS;
	}
	
        return err;
}

/*
 * xds_handle_comppkt1state : 
 *
 * Parses Combined Packet Info 
 */ 
static void xds_handle_comppkt1state(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	UINT32	len = 0;
	
	info = context->XDSBuffer;
	context->compPkt1Count++;
	
	len = context->compPkt1Count;

	switch(len)
	{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			if(context->currentXDSClass == CURRENTCLASSSTATE) 
				info->pkt1.type[len-1] = xds_get_progtype(data);
			else if(context->currentXDSClass == FUTURECLASSSTATE) 
				info->futurePkt1.type[len-1] = xds_get_progtype(data);
			break;
			
		case 6:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
				info->pkt1.rating.rating = 
						xds_get_mpaaprograting(data & 0x07);
			else if(context->currentXDSClass == FUTURECLASSSTATE)
				info->futurePkt1.rating.rating = 
						xds_get_mpaaprograting(data & 0x07);
			break;
			
		case 7:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
				info->pkt1.progLen.minute = data & 0x3F ;
			else if(context->currentXDSClass == FUTURECLASSSTATE)
				info->futurePkt1.progLen.minute = data & 0x3F ;
			break;
			
		case 8:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
				info->pkt1.progLen.hour = data & 0x3F;
			else if(context->currentXDSClass == FUTURECLASSSTATE)
				info->futurePkt1.progLen.hour = data & 0x3F;
			break;
			
		case 9:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
			   	info->pkt1.timeShow.minute = data & 0x3F;
			else if(context->currentXDSClass == FUTURECLASSSTATE)
			   	info->futurePkt1.timeShow.minute = data & 0x3F;
			break;
			
		case 10:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
				info->pkt1.timeShow.hour = data & 0x3F;
			else if(context->currentXDSClass == CURRENTCLASSSTATE)
				info->futurePkt1.timeShow.hour = data & 0x3F;
			break;
			
		default:
			{	
				char c;

				/* Don't overrun the buffer! And make sure that the 
				 * last byte of the name is always NULL
				 */
				if (context->compPkt1Count < (COMP_PKT_PARAMS-1))
				{
					c = data;
					if(context->currentXDSClass == CURRENTCLASSSTATE)
					{
						info->pkt1.title[len-10] = c;
						info->pkt1.title[len-9] = 0;
					}
					else if(context->currentXDSClass == FUTURECLASSSTATE)
					{
						info->futurePkt1.title[len-10] = c;
						info->futurePkt1.title[len-9] = 0;
					}
				}
			}
			break;
	}

	return;
}	

static AM_ErrorCode_t xds_finish_comppkt1state(AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	AM_XDSProgType_t temp;
	UINT32	len = 0;
	INT32	i, numTypes;
	AM_ErrorCode_t	err = AM_FAILURE;

	info = context->XDSBuffer;
	len = context->compPkt1Count;

	/* invalid length - too short, too long, or not an even number of
	 * characters - just exit with AM_FAILURE
	 */
	if ((len < 2) || (len > COMP_PKT_PARAMS) || (len & 0x1))
		goto EXIT_POINT;

	err = AM_SUCCESS;

	switch (len)
	{
		case 10:	/* elapsed time in show */
			if(context->currentXDSClass== CURRENTCLASSSTATE)
			   	info->startTime = info->pkt1.timeShow;
			else if(context->currentXDSClass == FUTURECLASSSTATE)
			   	info->startTime = info->futurePkt1.timeShow;

		case 8:		/* program length */
			if(context->currentXDSClass== CURRENTCLASSSTATE)
			   	info->progLen = info->pkt1.progLen;
			else if(context->currentXDSClass == FUTURECLASSSTATE)
			   	info->progLen = info->futurePkt1.progLen;
                        /*save info acquired from xds into database for application */
			xds_store_useproc_event(XDSPROGRAMTIMEEVENT);
                        break;

		case 6:		/* rating info */
			if(context->currentXDSClass == CURRENTCLASSSTATE)
				info->rating = info->pkt1.rating;
			else if(context->currentXDSClass == FUTURECLASSSTATE)
				info->futureRating = info->futurePkt1.rating;
                        /*save info acquired from xds into database for application */
			xds_store_useproc_event(XDSCONTENTADVISORYEVENT);
                        break;

		case 4:		/* program type */
		case 2:

			/* number of valid program types to copy across
			 */
			if (len <= 4)
				numTypes = len;
			else
				numTypes = 5;	/* if more than 4 type chars receieved, have  the maximum number - 5 */

			if(context->currentXDSClass == CURRENTCLASSSTATE) 
			{
				for (i = 0; i < numTypes; i++)
				{
					temp = info->pkt1.type[i];
					switch(temp)
					{
						case AM_XDSPROG_EDU:
						case AM_XDSPROG_ENTERTAINEMENT:
						case AM_XDSPROG_MOVIE:
						case AM_XDSPROG_NEWS:
						case AM_XDSPROG_RELIGIOUS:
						case AM_XDSPROG_SPORTS:
						case AM_XDSPROG_OTHERS:
						     info->progType[i].basicType  = temp;
                                                     break;
						default:
						     info->progType[i].detailType = temp;
                                                     break;
					}
				}
			}
			else if(context->currentXDSClass == FUTURECLASSSTATE) 
			{
				info->futureValidTypes = numTypes;
				for (i = 0; i < numTypes; i++)
				{
					temp = info->pkt1.type[i];
					switch(temp)
					{
						case AM_XDSPROG_EDU:
						case AM_XDSPROG_ENTERTAINEMENT:
						case AM_XDSPROG_MOVIE:
						case AM_XDSPROG_NEWS:
						case AM_XDSPROG_RELIGIOUS:
						case AM_XDSPROG_SPORTS:
						case AM_XDSPROG_OTHERS:
							info->futureProgType[i].basicType  = temp;

						default:
							info->futureProgType[i].detailType = temp;
					}
				}
			}
                        break;

                        default: /* anything greater than 10 - we have program name info */

                        /* copy the name across to the higher-level info context and
                         * the global XDS context
                         */
                        if(context->currentXDSClass == CURRENTCLASSSTATE)
                        {
                                strcpy(info->pkt1.title,info->progTitle);
                                strcpy(info->progTitle,context->progTitle);
                        }
                        else if(context->currentXDSClass == FUTURECLASSSTATE)
                        {
                                strcpy(info->pkt1.title, info->futureProgTitle);
                                strcpy(info->futureProgTitle,context->progTitle);
                        }
                        break;
              }
                          
              
              /*save info acquired from xds into database for application */
              xds_store_useproc_event(XDSPROGRAMNAMEEVENT);
	      xds_store_useproc_event(XDSPROGRAMTYPEEVENT);
EXIT_POINT:
	return err;
}	

/*
 * xds_handle_comppkt2state :
 *
 * Parses Combined Packet 2 Info
 */ 
static void xds_handle_comppkt2state(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	
	info = context->XDSBuffer;
	context->compPkt2Count++;
	
	switch(context->compPkt2Count)
	{
		case 1:
			if(context->currentXDSClass == CURRENTCLASSSTATE)info->pkt2.startTime.minute = data & 0x3F;
			else if(context->currentXDSClass == FUTURECLASSSTATE)info->futurePkt2.startTime.minute = data & 0x3F;
			break;
		case 2:
			if(context->currentXDSClass == CURRENTCLASSSTATE)info->pkt2.startTime.hour = data & 0x1F;
			else if(context->currentXDSClass == CURRENTCLASSSTATE)info->futurePkt2.startTime.hour = data & 0x1F;
			break;
			
		case 3:
			if(context->currentXDSClass == CURRENTCLASSSTATE)info->pkt2.startDate = data & 0x1F;
			else if(context->currentXDSClass == FUTURECLASSSTATE)info->futurePkt2.startDate = data & 0x1F;
			break;
			
		case 4:
			if(context->currentXDSClass == CURRENTCLASSSTATE)info->pkt2.startMonth = data & 0xF;
			else if(context->currentXDSClass == FUTURECLASSSTATE)info->futurePkt2.startMonth = data & 0xF;
			break;
			
		case 5:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
			{
				info->pkt2.audio.mainLang = xds_get_audiolanguage(data & 0x38);
				info->pkt2.audio.mainType = xds_get_mainaudio(data & 0x07);
			}
			else if(context->currentXDSClass == FUTURECLASSSTATE)
			{
				info->futurePkt2.audio.mainLang = xds_get_audiolanguage(data & 0x38);
				info->futurePkt2.audio.mainType = xds_get_mainaudio(data & 0x07);
			}
			break;

		case 6:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
			{
				info->pkt2.audio.sapLang = xds_get_audiolanguage(data & 0x38);
				info->pkt2.audio.sapType = xds_get_secondaudio(data & 0x07);
			}
			else if(context->currentXDSClass == FUTURECLASSSTATE)
			{
				info->futurePkt2.audio.sapLang = xds_get_audiolanguage(data & 0x38);
				info->futurePkt2.audio.sapType = xds_get_secondaudio(data & 0x07);
			}
			break;

		case 7:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
			{
				info->pkt2.serviceLanguage[0] =  xds_get_audiolanguage(data & 0x38);
				info->pkt2.services[0] = (AM_XDSCaptionServices_t) (data & 0x07);
			}
			else if(context->currentXDSClass == FUTURECLASSSTATE)
			{
				info->futurePkt2.serviceLanguage[0] =  xds_get_audiolanguage(data & 0x38);
				info->futurePkt2.services[0] = (AM_XDSCaptionServices_t) (data & 0x07);
			}
			break;

		case 8:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
			{
				info->pkt2.serviceLanguage[1] =  xds_get_audiolanguage(data & 0x38);
				info->pkt2.services[1] = (AM_XDSCaptionServices_t) (data & 0x07);
				
			}
			else if(context->currentXDSClass == FUTURECLASSSTATE)
			{
				info->futurePkt2.serviceLanguage[1] =  xds_get_audiolanguage(data & 0x38);
				info->futurePkt2.services[1] = (AM_XDSCaptionServices_t) (data & 0x07);
				
			}
			break;
			
		case 9:
		case 10:
		case 11:
		case 12:
			if(context->currentXDSClass == CURRENTCLASSSTATE)
                            strcat(info->pkt2.station.name,(const char *)&data);
			else if(context->currentXDSClass == FUTURECLASSSTATE)
                            strcat(info->futurePkt2.station.name,(const char *)&data);
			break;
			
		case 13:
		case 14:
			if(context->currentXDSClass == CURRENTCLASSSTATE) 
                            strcat(info->pkt2.station.chNum,(const char *)&data);
			else if(context->currentXDSClass == FUTURECLASSSTATE) 
                            strcat(info->futurePkt2.station.chNum,(const char *)&data);
			break;
			
		default:
			{	
				char c;
				c = data;
		                if(context->currentXDSClass == CURRENTCLASSSTATE)
				     strcat(info->pkt2.networkName,&c);
				else if(context->currentXDSClass == FUTURECLASSSTATE) 
                                     strcat(info->futurePkt2.networkName,&c);
			}
			break;

	}
	
	return;
}

/* add full handling here, but the functionality matches
 * the original XDS driver
 *
 */
static AM_ErrorCode_t xds_finish_comppkt2state(AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *)NULL;
	UINT32	len;
	
	info = context->XDSBuffer;
	
	len = context->compPkt2Count;
	
	if ((len < 4) || (len > COMP_PKT_PARAMS) || (len & 0x1))
		return AM_FAILURE;

	return  AM_SUCCESS;

}

/* 
 * handleProgdescstate : 
 *
 */ 
static void xds_handle_progdescstate(UINT8 data,AM_XDSContext_t *context)
{
	UINT32 	index=0,sIndex=0;
	char	c;
	AM_XDSInfo_t *info = (AM_XDSInfo_t *)NULL;

	info = context->XDSBuffer;

	if(data >= 0x20 && data <= 0x7F)
	{
		switch(context->currentXDSType)
		{
			case PROGDESCSTATE1: index=0;break;
			case PROGDESCSTATE2: index=1;break;
			case PROGDESCSTATE3: index=2;break;
			case PROGDESCSTATE4: index=3;break;
			case PROGDESCSTATE5: index=4;break;
			case PROGDESCSTATE6: index=5;break;
			case PROGDESCSTATE7: index=6;break;
			case PROGDESCSTATE8: index=7;break;
			default: 
				return;
		}

		c = data;
		sIndex = context->progDescLevel2Count;
		if(sIndex+1 < 32)
		{
			context->progDesc[index].title[sIndex] = c;
			context->progDesc[index].title[sIndex+1] = '\0';
		}

		 AM_XDS_DBG("[%c %x]\n",c,c);
	}

	if (context->progDescLevel2Count < 32)
		context->progDescLevel2Count++;

	return;
}

static AM_ErrorCode_t xds_finish_progdescstate(AM_XDSContext_t *context)
{
	UINT32 	len=0, index = 0;
	AM_XDSInfo_t *info = (AM_XDSInfo_t *)NULL;
	AM_ErrorCode_t	err = AM_FAILURE;

	info = context->XDSBuffer;
	len = context->progDescLevel2Count;

	/* determine which of the fields  received 
	 */
	switch(context->currentXDSType)
	{
		case PROGDESCSTATE1: index=0;break;
		case PROGDESCSTATE2: index=1;break;
		case PROGDESCSTATE3: index=2;break;
		case PROGDESCSTATE4: index=3;break;
		case PROGDESCSTATE5: index=4;break;
		case PROGDESCSTATE6: index=5;break;
		case PROGDESCSTATE7: index=6;break;
		case PROGDESCSTATE8: index=7;break;
		default: goto EXIT_POINT;
	}

	/* check to make sure that just ended a valid field
	 */
	if ((len > 0) && (len <= PROG_DESC_PARAMS))
	{
		/* copy the data across from to the main context */
		if(xdsContext->currentXDSClass == CURRENTCLASSSTATE)
		{
			strcpy(info->progDesc[index].title,
			xdsContext->progDesc[index].title);
		}
		else if(xdsContext->currentXDSClass == FUTURECLASSSTATE)
		{
			 strcpy(info->futureProgDesc[index].title,
			 xdsContext->progDesc[index].title);
		}
           
                /*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSPROGRAMDESCRIPTIONEVENT);

		AM_XDS_DBG("[field %d good]", (index-1));
	}
			
EXIT_POINT:
	return err;
}

/*
 * xds_handle_NetworkNamestate :
 * Parses and stores the network name info.
 * 
 */ 
static void xds_handle_networknamestate(UINT8 data,AM_XDSContext_t *context)
{
	char c;
	AM_XDSInfo_t *info = (AM_XDSInfo_t *)NULL;
	UINT32 index=0;
	
	info = context->XDSBuffer;
	
	if(data >= 0x20 && data <= 0x7F)
	{
		c = data;
		index = context->networkNameCount;
		if(index < NETWORK_NAME_PARAMS)
			context->networkName[index] = c;
		if(index+1 < NETWORK_NAME_PARAMS)
		   	context->networkName[index+1]='\0';

		 AM_XDS_DBG("[%c %x]\n",c,data);
	}

	context->networkNameCount++;

	return;
}

static AM_ErrorCode_t xds_finish_networknamestate(AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	AM_ErrorCode_t	err = AM_FAILURE;
	
	info = context->XDSBuffer;
	
	if(context->networkNameCount <= NETWORK_NAME_PARAMS)
	{
		strcpy(info->networkName,context->networkName);
                /*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSNETWORKINFOEVENT);
		err = AM_SUCCESS;
	}
	
	return	err;
}

/*
 * xds_handle_nativechannelstate:
 * Calculates the native channel state.
 *
 */
static void xds_handle_nativechannelstate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	char c;
	UINT32 index=0;
	
	info = context->XDSBuffer;
	context->nativeChannelCount++;
	
	if(data >= 0x20 && data <= 0x7F)
	{
		c = data;
		if(context->nativeChannelCount <= 4)
		{
			index = context->nativeChannelCount - 1;
			context->station.name[index]=c;
			if(context->nativeChannelCount != 4) context->station.name[index+1]='\0';
		}
		else if(context->nativeChannelCount == 5 || context->nativeChannelCount == 6)
		{
			index = context->nativeChannelCount - 5;
			context->station.chNum[index]=c;
		}

		 AM_XDS_DBG("[%c %x]\n",c,data);
	}

	return;
}

static AM_ErrorCode_t xds_finish_nativechannelstate(AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	AM_ErrorCode_t	err = AM_FAILURE;
	
	info = context->XDSBuffer;
	
	if(context->nativeChannelCount <= NATIVE_CHANNEL_PARAMS)
	{
		/* this copies everything */
		context->station = info->station;
                /*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSNETWORKINFOEVENT);
		err = AM_SUCCESS;
	}
	
	return err;
}

/*
 * xds_handle_tapedelaystate:
 * Calculates the Tape Delat and stores it.
 * 
 */
static void xds_handle_tapedelaystate(UINT8 data,AM_XDSContext_t *context)
{
	UINT32	temp;
	AM_XDSInfo_t *info= (AM_XDSInfo_t *) NULL;
	
	info = context->XDSBuffer;
	context->tapeDelayCount++;

	switch(context->tapeDelayCount)
	{
		case 1:
			temp = data & 0x3F;
			info->delayTime.minute = temp;
			break;
			
		case 2:
			temp = data & 0x1F;
			info->delayTime.hour = temp;
			break;
			
		default:break;
	}
	
	return;
}

static AM_ErrorCode_t xds_finish_tapedelaystate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_FAILURE;
	
	if(context->tapeDelayCount == TAPE_DELAY_PARAMS)
		err = AM_SUCCESS;

	return err;
}

/*
 * xds_handle_ TSIDstate : 
 * Calculates and stores the TSID Info.
 *
 */ 
static void  xds_handle_TSIDstate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	UINT16 temp=0;
	
	/* 
	 * Transport of Transmission Signal Indentifier (TSID) using
	 * XDS Data Service - 
	 * EIA 752
	 *
	 */

	info = context->XDSBuffer;
	context->tsidCount++;

	switch(context->tsidCount)
	{
		case 1:
			/* Bit t0 - t3 */
			context->tsidData = data & 0xF;
			break;

		case 2:
			/* Bit t4 - t7 */
			temp = (data & 0xF) << 4;
			context->tsidData = context->tsidData | temp;
			break;

		case 3:
			/* Bit t8 - t11 */
			temp = (data & 0xF) << 8;
			context->tsidData = context->tsidData | temp;
			break;
			
		case 4:
			/* Bit t12 - t15 */
			temp = (data & 0xF) << 12;
			context->tsidData = context->tsidData | temp;
			info->tsid = context->tsidData;
			context->tsidData = 0;
			break;
			
		default:
			break;
	}
	
	return;
}

static AM_ErrorCode_t xds_finish_TSIDstate(AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	AM_ErrorCode_t	err = AM_FAILURE;
	
	/* 
	 * Transport of Transmission Signal Indentifier (TSID) using
	 * XDS Data Service - 
	 * EIA 752
	 *
	 */

	if(context->tsidCount == TSID_PARAMS)
	{
	    AM_XDS_DBG("[TSID = %d]\n",info->tsid);
	    err = AM_SUCCESS;
	}
	
	return err;
}


/*
 * xds_handle_timeofdaystate: 
 */ 
static void xds_handle_timeofdaystate(UINT8 data,AM_XDSContext_t *context)
{
	UINT32 temp;
	AM_XDSInfo_t	*info = (AM_XDSInfo_t *) NULL;
	
	info = context->XDSBuffer;
	context->timeOfDayCount++;

	switch(context->timeOfDayCount)
	{
		case 1:
			temp = data & 0x3F;
			info->currentDayTime.minute = temp;
			 AM_XDS_DBG("[Min=%d]",temp);
			break;

		case 2:
			temp = data & 0x1F;
			info->currentDayTime.hour = temp;
			 AM_XDS_DBG("[Hr=%d]",temp);
			break;

		case 3:
			temp = data & 0x1F;
			info->currentDate = temp;
			 AM_XDS_DBG("[Date=%d]",temp);
			break;
		case 4:
			temp = data & 0xF;
			info->currentMonth = temp;
			 AM_XDS_DBG("[Month=%d]",temp);
			break;

		case 5:
			temp = data & 0x07;
			info->currentDay = temp;
			 AM_XDS_DBG("[Day=%d]",temp);
			break;
			
		case 6:
			temp = data & 0x3F;
			info->currentYear = 1990 + temp;
			 AM_XDS_DBG("[Year=%d]",temp);
			break;

		default:
			break;
	}
	
	return;
}

static AM_ErrorCode_t xds_finish_timeofdaystate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_FAILURE;
	
	if(context->timeOfDayCount == TIME_OF_DAY_PARAMS)
	{
                /*save info acquired from xds into database for application */
		xds_store_useproc_event(XDSDATETIMEEVENT);
		err = AM_SUCCESS;
	}
	
	return err;
}


static void  xds_handle_impulsestate(UINT8 data,AM_XDSContext_t *context)
{
	UINT32 temp;
	AM_XDSInfo_t	*info = (AM_XDSInfo_t *) NULL;
	
	info = context->XDSBuffer;
	context->impulseCount++;

	switch(context->impulseCount)
	{
		case 1:
			temp = data & 0x3F;
			info->captureStartTime.minute = temp;
			break;

		case 2:
			temp = data & 0x1F;
			info->captureStartTime.hour = temp;
			break;

		case 3:
			temp = data & 0x1F;
			info->captureStartDate = temp;
			break;

		case 4:
			temp = data & 0xF;
			info->captureStartMonth = temp;
			break;

		case 5:
			temp = data & 0x3F;
			info->captureProgLen.minute = temp;	
			break;
			
		case 6:
			temp = data & 0x3F;
			info->captureProgLen.hour = temp;	
			break;

		default:
			break;
	}
	
	return;
}

static AM_ErrorCode_t xds_finish_impulsestate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t err = AM_FAILURE;
	
	if(context->impulseCount == IMPULSE_PARAMS)
	{
		err = AM_SUCCESS;
	}
	
	return err;
}

static void  xds_handle_supplementstate(UINT8 data,AM_XDSContext_t *context)
{
	UINT32	temp;
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;

	/* don't copy data out of range */
	if (context->supplementCount >= SUPPLEMENT_PARAMS)
		goto EXIT_POINT;

	info = context->XDSBuffer;

	temp = data & 0x20;

	info->supplementInfo[context->supplementCount].field1 = (temp ? AM_FALSE : AM_TRUE);
	info->supplementInfo[context->supplementCount].lineNum = data & 0x1F;

EXIT_POINT:
	context->supplementCount++;
	return;
}

static AM_ErrorCode_t xds_finish_supplementstate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_FAILURE;

	if(context->supplementCount == SUPPLEMENT_PARAMS)
	{
		err = AM_SUCCESS;
	}
	
	return err;
}

static void  xds_handle_localtimezonestate(UINT8 data,AM_XDSContext_t *context)
{
	UINT32 temp;
	AM_XDSInfo_t	*info = (AM_XDSInfo_t *) NULL;

	info = context->XDSBuffer;
	context->localTimeZoneCount++;

	switch(context->localTimeZoneCount)
	{
		case 1:
			temp = data & 0x20;
			info->timeZoneOffset.present = (temp ? AM_TRUE : AM_FALSE);
			info->timeZoneOffset.offset = data & 0x1F;
			break;
			
		case 2:
		default:
			break;
	}

	return;
}

static AM_ErrorCode_t xds_finish_localtimezonestate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_FAILURE;

	if(context->localTimeZoneCount == LOCAL_TIME_ZONE_PARAMS)
	{
		err = AM_SUCCESS;
	}
	
	return err;
}


static void xds_handle_OOBstate(UINT8 data,AM_XDSContext_t *context)
{
	UINT16 		temp=0;
	AM_XDSInfo_t		*info = (AM_XDSInfo_t *) NULL;

	info = context->XDSBuffer;
	context->oobCount++;

	switch(context->oobCount)
	{
		case 1:
			temp = data & 0x3F;
			info->oobChannelNumber &= temp;
			break;
			
		case 2:
			temp = (data & 0x3F) << 6 ;
			info->oobChannelNumber |= temp;
			break;

		default:
			break;
	}
	
	return;
}

static AM_ErrorCode_t xds_finish_OOBstate(AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_FAILURE;

	if(context->oobCount == OOB_PARAMS)
	{
		err = AM_SUCCESS;
	}

	return err;
}

static void xds_handle_CHMappingstate(UINT8 data,AM_XDSState_t state,AM_XDSContext_t *context)
{
	AM_XDSInfo_t	*info= (AM_XDSInfo_t *) NULL;
	UINT8	temp=0;
	char c;
	UINT32	index=0;
	
	info = context->XDSBuffer;
	context->chMapCount++;
	/* 
	 * Transmission of Cable Channel Mapping System Information
	 * using XDS Data Services
	 * EIA 745
	 */ 
	switch(state)
	{
		case CHMAPPOINTERSTATE:
			switch(context->chMapCount)
			{
				case 1:
					/* 1st byte */
					info->chMap.chLow = data & 0x3F;
					break;
					
				case 2:
					/* 2nd Byte */
					info->chMap.chHi = data & 0xF;
					break;
					
				default:
					break;
			}
			break;

		case CHMAPHEADERSTATE:
			switch(context->chMapCount)
			{
				case 1:
					/* 1st Byte */
					info->chMap.chLow = data & 0x3F;
					break;
					
				case 2:
					/* 2nd Byte */
					info->chMap.chHi = data & 0xF;
					break;
					
				case 3:
					/* 3rd Byte  - Version Number*/
					info->chMap.version = data & 0x3F;
					break;
					
				case 4:
					/* 4th Byte */
				default:
					break;
			}
			break;
			
		case CHMAPPACKETSTATE:
			switch(context->chMapCount)
			{
				case 1:
					/* 1st Byte */
					info->chMap.chLow = data & 0x3F;
					break;
					
				case 2:
					/* 2nd Byte */
					info->chMap.chHi = data & 0xF;
					temp = (data & 0x20) >> 5;
					if(temp == 0) 
					{
						/* 
						 * Bit rm (b5) is not set - this means
						 * channel is not remapped so next 2 bytes will
						 * not come hence increament count.
						 */ 
						context->chMapCount +=2; 
					}
					break;
					
				case 3:
					/* 3rd Byte */
					info->chMap.chLow = data & 0x3F;
					break;
					
				case 4:
					/* 4th Byte */
					info->chMap.chHi = data & 0xF;
					break;
					
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				        c = data;
					index = context->chMapCount - 5;
					info->chMap.chId[index]=c;	
					break;

				default:
					break;
			}
			break;
			
		default:
			break;
			
	}

	return;
}

static AM_ErrorCode_t xds_finish_CHMappingstate(AM_XDSState_t state,AM_XDSContext_t *context)
{
	AM_ErrorCode_t	err = AM_FAILURE;
	
	/* 
	 * Transmission of Cable Channel Mapping System Information
	 * using XDS Data Services
	 * EIA 745
	 *
	 */ 
	switch(state)
	{
		case CHMAPPOINTERSTATE:
			if(context->chMapCount == CH_MAP_POINTER_PARAMS)
			{
				err = AM_SUCCESS;
			}
			break;

		case CHMAPHEADERSTATE:
			if(context->chMapCount == CH_MAP_HEADER_PARAMS)
			{
				err = AM_SUCCESS;
			}
			break;
			
		case CHMAPPACKETSTATE:
			if((context->chMapCount >= 4) &&
			   (context->chMapCount <= CH_MAP_PACKET_PARAMS))
			{
				err = AM_SUCCESS;
			}
			break;
			
		default:
			break;
	}
	
	return err;
}

static void  xds_handle_weathercodestate(UINT8 data,AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	
	/*
	 * Currently Not supported
	 */

	info = context->XDSBuffer;
	
	/* NEED TO ADD PARSER HERE */
	info->weatherCodePresent = AM_TRUE;

	return;
}

static AM_ErrorCode_t xds_finish_weathercodestate(AM_XDSContext_t *context)
{
	AM_XDSInfo_t *info = (AM_XDSInfo_t *) NULL;
	AM_ErrorCode_t err = AM_FAILURE;
	
	/*
	 * Currently Not supported,
	 */

	info = context->XDSBuffer;
	info->weatherCodePresent = AM_TRUE;
        /*save info acquired from xds into database for application */
	xds_store_useproc_event(XDSWEATHERINFOEVENT);
	err = AM_SUCCESS;

	return err;
}

static void  xds_handle_weathermsgstate(UINT8 data,AM_XDSContext_t *context)
{
	char c;
	AM_XDSInfo_t 	*info = (AM_XDSInfo_t *) NULL;
	UINT32 index=0;
	
	info = context->XDSBuffer;
	
	c = data;
	index = context->weatherMessageCount;

	if(context->weatherMessageCount < WEATHER_MSG_PARAMS)
		context->message[index]=c;

	 AM_XDS_DBG("[%c %x]\n",c,c);

	context->weatherMessageCount++;

	if(context->weatherMessageCount < WEATHER_MSG_PARAMS)
		context->message[index+1]='\0';
	
	return;
}

static AM_ErrorCode_t xds_finish_weathermsgstate(AM_XDSContext_t *context)
{
	AM_XDSInfo_t 	*info = (AM_XDSInfo_t *) NULL;
	UINT32 index=0, i = 0;
	AM_ErrorCode_t	err = AM_FAILURE;
	
	info = context->XDSBuffer;
	index = context->weatherMessageCount;

	if(index <= WEATHER_MSG_PARAMS)
	{
		for (i = 0; i < index; i++)
			context->message[i] = info->message[i];
             
                 /*save info acquired from xds into database for application */
		 xds_store_useproc_event(XDSWEATHERINFOEVENT);
		 err = AM_SUCCESS;
	}
	
	return err;
}

/*
 * xds_get_progtype :
 * Returns the Prog Type
 */
static AM_XDSProgType_t xds_get_progtype(UINT8 data)
{
	if (data < 0x20 || data > 0x7f) {
		return(AM_XDSPROG_UNAVAILABLE);
	}
	else {
		return((AM_XDSProgType_t)data);
	}
}

/*
 * xds_get_mpaaprograting : 
 * Returns the program rating according to MPAA Rating
 * System.
 */
static AM_XDSProgRating_t xds_get_mpaaprograting(UINT8 data)
{
	switch(data)
	{
		case 0:	return NOTAPPLICABLE;
		case 1:	return GENERALPROGRAMMING;
		case 2:	return PARENTALGUIDANCE;
		case 3:	return PARENTALGUIDANCE13;
		case 4:	return RESTRICTEDPROGRAMMING;
		case 5:	return NC17;
		case 6:	return XRATED;
		case 7:	return NOTRATED;
		default:	return NOTAPPLICABLE;
	}
}

/*
 * xds_get_ustvprograting : 
 * Returns the program rating according to US-TV Parental
 * Guidance System.
 */
static AM_XDSProgRating_t xds_get_ustvprograting(UINT8 data)
{
	switch(data)
	{
		case 1:	return TV_Y;
		case 2:	return TV_Y7;
		case 3:	return TV_G;
		case 4:	return TV_PG;
		case 5: return TV_14;
		case 6: return TV_MA;
		case 0:	
		case 7:
		default: return NOTAPPLICABLE;
	}
}

/*
 * xds_get_celprograting : 
 * Returns the program rating according to Canadian English Language
 * Rating System.
 */
static AM_XDSProgRating_t xds_get_celprograting(UINT8 data)
{
	switch(data)
	{
		case 0:	return CE_EXEMPT;
		case 1:	return CE_CHILDREN;
		case 2:	return CE_CHILDREN8;
		case 3:	return CE_GENERALPROGRAMMING;
		case 4: return CE_PARENTALGUIDANCE;
		case 5: return CE_FOURTEENPLUS;
		case 6:	return CE_EIGHTEENPLUS;
		default: return RATINGNOTAVAILABLE;
	}
}

/*
 * xds_get_cflprograting : 
 * Returns the program rating according to Canadian French Language
 * Rating System.
 */
static AM_XDSProgRating_t xds_get_cflprograting(UINT8 data)
{
	switch(data)
	{
		case 0:	return CF_EXEMPT;
		case 1:	return CF_GENERAL;
		case 2:	return CF_EIGHTPLUS;
		case 3:	return CF_THIRTEENPLUS;
		case 4: return CF_SIXTEENPLUS;
		case 5: return CF_EIGHTEENPLUS;
		default: return RATINGNOTAVAILABLE;
	}
}

/*
 * xds_get_audiolanguage :
 */
static AM_XDSLanguage_t xds_get_audiolanguage(UINT8 data)
{
	switch(data)
	{
		case 0: return XUNKNOWN;
		case 1:	return XENGLISH;
		case 2: return XSPANISH;
		case 3:	return XFRENCH;
		case 4:	return XGERMAN;
		case 5:	return XITALIAN;
		case 6:	return XOTHER;
		case 7: return XNONE;
		default: return XUNKNOWN;
	}
}

/*
 * xds_get_mainaudio :
 */

static AM_XDSAudioType_t xds_get_mainaudio(UINT8 data)
{
	switch(data)
	{
		case 0:	return UNKNOWNTYPE;
		case 1:	return MONO;
		case 2:	return SIMULATEDSTEREO;
		case 3:	return TRUESTEREO;
		case 4:	return STEREOSURROUND;
		case 5:	return DATASERVICE;
		case 6:	return OTHERTYPE;
		case 7:	return NONETYPE;
		default:return UNKNOWNTYPE;
	}
}

/*
 * xds_get_secondaudio :
 */

static AM_XDSAudioType_t xds_get_secondaudio(UINT8 data)
{
	switch(data)
	{
		case 0:	return UNKNOWNTYPE;
		case 1:	return MONO;
		case 2:	return VIDEODESCRIPTIONS;
		case 3:	return NONPROGAUDIO;
		case 4:	return SPECIALEFFECTS;
		case 5:	return DATASERVICE;
		case 6:	return OTHERTYPE;
		case 7:	return NONETYPE;
		default:return UNKNOWNTYPE;
	}
}

/*
 * xds_get_ccservicetype:
 */
static AM_XDSCaptionServices_t xds_get_ccservicetype(UINT8 data)
{
	switch(data)
	{
		case 0:	return AM_UNKNOWNSERV;
		case 1:	return AM_CS1;
		case 2:	return AM_CS2;
		case 3:	return AM_CS3;	
		case 4:	return AM_CS4;
		case 5:	return AM_CS5;
		case 6: return AM_CS6;
		case 7: return AM_CS7;
		default: return AM_UNKNOWNSERV;
	}
}

/* Debugging Functions */

static const char *xds_get_programratingstring(AM_XDSProgRating_t rating)
{
	switch(rating)
	{
		case RATINGNOTAVAILABLE: 	return "RATINGNPTAVAILABLE";	
		case NOTAPPLICABLE: 		return "notApplicable";	
		
		case CE_GENERALPROGRAMMING:
		case CF_GENERAL:
		case GENERALPROGRAMMING:	return "generalProgramming";	
		
		case CE_PARENTALGUIDANCE:
		case PARENTALGUIDANCE:		return "parentalGuidance";	
		
		case PARENTALGUIDANCE13:	return "parentalGuidance13";	
		case RESTRICTEDPROGRAMMING:	return	"restrictedProgramming";	
		case NC17:	       		return "nc17";	
		case XRATED:			return "X-Rated";	
		case NOTRATED:			return "notRated";
		case TV_Y:			return "TV_Y";
		case TV_Y7: 			return "TV_Y7";	
		case TV_G: 			return "TV_G";	
		case TV_PG:			return "TV_PG";	
		case TV_14:			return "TV_14";	
		case TV_MA:			return "TV_MA";	
		
		case CE_EXEMPT:
		case CF_EXEMPT:			return "exempt";
									
		case CE_CHILDREN:		return "children";
		case CE_CHILDREN8:		return "children eight plus";
		case CE_FOURTEENPLUS:		return "fourteen plus";
		
		case CE_EIGHTEENPLUS:		
		case CF_EIGHTEENPLUS:		return "eighteen plus";

		case CF_EIGHTPLUS:		return "eight plus";
		case CF_THIRTEENPLUS:		return "thirteen plus";
		case CF_SIXTEENPLUS:		return "sixteen plus";
		default : 			return "noAvailable";
	}
}

static const char *xds_get_languagestring(AM_XDSLanguage_t lang)
{
	switch(lang)
	{
		case XUNKNOWN: 	return "Unknown";	
		case XENGLISH: 	return "eng";	
		case XSPANISH:	return "spa";	
		case XFRENCH:	return "fre";	
		case XGERMAN:	return "ger";	
		case XITALIAN:	return "ita";	
		case XOTHER: 	return "Other";	
		case XNONE:	return "None";	
		default: 	return "Audio";
	}
}

static const char *xds_get_audiotypestring(AM_XDSAudioType_t type)
{
	switch(type)
	{
		case UNKNOWNTYPE:	 	return "UNKNOWNTYPE";	
		case MONO: 			return "mono";	
		case SIMULATEDSTEREO:		return "simulatedStereo";	
		case TRUESTEREO:		return "trueStereo";	
		case STEREOSURROUND:		return "stereoSurround";	
		case DATASERVICE:		return	"dataService";	
		case VIDEODESCRIPTIONS: 	return "videoDescriptions";	
		case NONPROGAUDIO:		return "nonProgAudio";	
		case SPECIALEFFECTS:		return "specialEffects";
		case OTHERTYPE:			return "otherType";	
		case NONETYPE:			return "noneType";
		default: 			return "notAvailable";
	}
}

static const char *xds_get_aspectratio(AM_XDSAR_t type)
{
	switch(type)
	{
		case VISTAVISION: 	return "VISTAVISION";	
		case CINEMASCOPE: 	return "CINEMASCOPE";	
		case DEFAULTAR:		return "defaultAR";	
		case UNKNOWNAR: 	return "UNKNOWNAR";
		default: 		return "notAvailable";
	}
}


static const char *xds_get_programtypestring(AM_XDSProgType_t type)
{
	switch(type)
	{
		case AM_XDSPROG_EDU :					return "education";
		case AM_XDSPROG_ENTERTAINEMENT:				return "entertainement";
		case AM_XDSPROG_MOVIE:					return "movie";
		case AM_XDSPROG_NEWS:					return "news";
		case AM_XDSPROG_RELIGIOUS:				return "religious";
		case AM_XDSPROG_SPORTS:					return "sports";
		case AM_XDSPROG_OTHERS:					return "others";
		case AM_XDSPROG_ACTION :				return "action";
		case AM_XDSPROG_ADVERTISEMENT:				return "advertisement";
		case AM_XDSPROG_ANIMATED:				return "animated";
		case AM_XDSPROG_ANTHOLOGY:				return "anthology";
		case AM_XDSPROG_AUTOMOBILE:				return "automobile";
		case AM_XDSPROG_AWARDS:					return "awards";
		case AM_XDSPROG_BASEBALL:				return "baseball";
		case AM_XDSPROG_BASKETBALL :				return "basketball";
		case AM_XDSPROG_BULLETIN:				return "bulletin";
		case AM_XDSPROG_BUSINESS:				return "business";
		case AM_XDSPROG_CLASSICAL:				return "classical";
		case AM_XDSPROG_COLLEGE:				return "college";
		case AM_XDSPROG_COMBAT:					return "combat";
		case AM_XDSPROG_COMEDY:					return "comedy";
		case AM_XDSPROG_COMMENTARY :				return "commentary";
		case AM_XDSPROG_CONCERT:				return "concert";
		case AM_XDSPROG_CONSUMER:				return "consumer";
		case AM_XDSPROG_CONTEMPORARY:				return "contemporary";
		case AM_XDSPROG_CRIME:					return "crime";
		case AM_XDSPROG_DANCE:					return "dance";
		case AM_XDSPROG_DOCUMENTARY:				return "documentary";
		case AM_XDSPROG_DRAMA :					return "drama";
		case AM_XDSPROG_ELEMENTARY:				return "elementary";
		case AM_XDSPROG_EROTICA:				return "erotica";
		case AM_XDSPROG_EXERCISE:				return "exercise";
		case AM_XDSPROG_FANTASY:				return "fantasy";
		case AM_XDSPROG_FARM:					return "farm";
		case AM_XDSPROG_FASHION:				return "fashion";
		case AM_XDSPROG_FICTION :				return "fiction";
		case AM_XDSPROG_FOOD:					return "food";
		case AM_XDSPROG_FOOTBALL:				return "football";
		case AM_XDSPROG_FOREIGN:				return "foreign";
		case AM_XDSPROG_FUNDRAISER:				return "fundRaiser";
		case AM_XDSPROG_GAMEQUIZ:				return "gameQuiz";
		case AM_XDSPROG_GARDEN:					return "garden";
		case AM_XDSPROG_GOLF :					return "golf";
		case AM_XDSPROG_GOVERNMENT:				return "government";
		case AM_XDSPROG_HEALTH:					return "health";
		case AM_XDSPROG_HIGHSCHOOL:				return "highSchool";
		case AM_XDSPROG_HISTORY:				return "history";
		case AM_XDSPROG_HOBBY:					return "hobby";
		case AM_XDSPROG_HOCKEY:					return "hockey";
		case AM_XDSPROG_HOME :					return "home";
		case AM_XDSPROG_HORROR:					return "horror";
		case AM_XDSPROG_INFORMATION:				return "information";
		case AM_XDSPROG_INSTRUCTION:				return "instruction";
		case AM_XDSPROG_INTERNATIONAL:				return "international";
		case AM_XDSPROG_INTERVIEW:				return "interview";
		case AM_XDSPROG_LANGUAGE:				return "language";
		case AM_XDSPROG_LEGAL:					return "legal";
		case AM_XDSPROG_LIVE:					return "live";
		case AM_XDSPROG_LOCAL:					return "local";
		case AM_XDSPROG_MEDICAL :				return "medical";
		case AM_XDSPROG_MEETING:				return "meeting";
		case AM_XDSPROG_MILLITARY:				return "millitary";
		case AM_XDSPROG_MINISERIES:				return "miniseries";
		case AM_XDSPROG_MUSIC:					return "music";
		case AM_XDSPROG_MYSTERY:				return "mystery";
		case AM_XDSPROG_NATIONAL:				return "national";
		case AM_XDSPROG_NATURE :				return "nature";
		case AM_XDSPROG_POLICE:					return "police";
		case AM_XDSPROG_POLITICS:				return "politics";
		case AM_XDSPROG_PREMIERE:				return "premiere";
		case AM_XDSPROG_PRERECORDED:				return "prerecorded";
		case AM_XDSPROG_PRODUCT:				return "product";
		case AM_XDSPROG_PROFESSIONAL:				return "professional";
		case AM_XDSPROG_PUBLICGR:				return "publicGr";
		case AM_XDSPROG_RACING:					return "racing";
		case AM_XDSPROG_READING:				return "reading";
		case AM_XDSPROG_REPAIR :				return "repair";
		case AM_XDSPROG_REPEAT:					return "repeat";
		case AM_XDSPROG_REVIEW:					return "review";
		case AM_XDSPROG_ROMANCE:				return "romance";
		case AM_XDSPROG_SCIENCE:				return "science";
		case AM_XDSPROG_SERIES:					return "series";
		case AM_XDSPROG_SERVICE:				return "service";
		case AM_XDSPROG_SHOPPING :				return "shopping";
		case AM_XDSPROG_SOAPOPERA:				return "soapOpera";
		case AM_XDSPROG_SPECIAL:				return "special";
		case AM_XDSPROG_SUSPENSE:				return "suspense";
		case AM_XDSPROG_TALK:					return "talk";
		case AM_XDSPROG_TECHNICAL:				return "technical";
		case AM_XDSPROG_TENNIS:				        return "tennis";
		case AM_XDSPROG_TRAVEL:					return "travel";
		case AM_XDSPROG_VARIETY:				return "variety";
		case AM_XDSPROG_VIDEO:					return "video";
		case AM_XDSPROG_WEATHER:				return "weather";
		case AM_XDSPROG_WESTERN:				return "western";
		default: 						return "notAvaialble";
	}
}

static const char *xds_get_cgmsstring(AM_CgmsInfo_t type)
{
	switch(type)
	{
		case UNKNOWNCONDITION: 	return "UNKNOWNCONDITION";	
		case COPYPERMITTED: 	return "COPYPERMITTED";	
		case INVALIDCONDITION:	return "invalidCondition";	
		case ONEGENERATIONCOPY: return "oneGenerationCopy";	
		case COPYNOTPERMITTED:	return "copyNotPermitted";	
		case NOAPS:		return	"noAPS";	
		case PSPONSPLITBURSTOFF:return "pspOnSplitBurstOff";	
		case PSPON2LINESPLITBURSTON: return "pspOn2LineSplitBurstOn";	
		case PSPON4LINESPLITBURSTON: return "pspOn4LineSplitBurstOn";	
		default: 		return "notAvailable";
	}
}

/*
**  This function transfer AM_ContentAdvisoryInfo_t get from xds into atsc_content_advisory_dr_t
**  which used format to save rrt_rating info into dvb.db
*/
static void xds_extract_content_advisory(AM_ContentAdvisoryInfo_t xds_CAInfo)
{
     
     AM_XDS_DBG("\n[%s:%d] ----get xds_CAInfo.rating is :%d \n",__FUNCTION__,__LINE__,xds_CAInfo.rating);
     memset(&xds_ca_info ,0,sizeof(atsc_content_advisory_dr_t));
    
     xds_ca_info.i_region_count = 1;
     if(xds_CAInfo.rating >= RATINGNOTAVAILABLE && xds_CAInfo.rating <= TV_MA){

          xds_ca_info.region[0].i_rating_region = 1;
          xds_ca_info.region[0].i_dimension_count = 1;

          if(xds_CAInfo.rating >= NOTAPPLICABLE && xds_CAInfo.rating <=NOTRATED)
               xds_ca_info.region[0].dimension[0].i_dimension_j = 7;
          else if (xds_CAInfo.rating >= TV_Y && xds_CAInfo.rating <= TV_MA){
              
               if(TV_Y == xds_CAInfo.rating || TV_Y7 == xds_CAInfo.rating)
                   xds_ca_info.region[0].dimension[0].i_dimension_j = 5;
               else if (AM_TRUE == xds_CAInfo.ssDialog)
                   xds_ca_info.region[0].dimension[0].i_dimension_j = 1; 
               else if( AM_TRUE == xds_CAInfo.adultLanguage)
                   xds_ca_info.region[0].dimension[0].i_dimension_j = 2;
               else if (AM_TRUE == xds_CAInfo.sexualSituations)
                   xds_ca_info.region[0].dimension[0].i_dimension_j = 3;
               else if (AM_TRUE == xds_CAInfo.violence)
                   xds_ca_info.region[0].dimension[0].i_dimension_j = 4;
               else if (AM_TRUE == xds_CAInfo.fantasyViolence)
                   xds_ca_info.region[0].dimension[0].i_dimension_j = 6;
               else
                   xds_ca_info.region[0].dimension[0].i_dimension_j = 0;//Entire Audience   
           }
     }
     else if(xds_CAInfo.rating >= CE_EXEMPT && xds_CAInfo.rating <= CF_EIGHTEENPLUS){
           xds_ca_info.region[0].i_rating_region = 2;
           xds_ca_info.region[0].i_dimension_count = 1;
          
          if(xds_CAInfo.rating >= CE_EXEMPT && xds_CAInfo.rating <= CE_EIGHTEENPLUS)
               xds_ca_info.region[0].dimension[0].i_dimension_j = 0;
          else if (xds_CAInfo.rating >= CF_EXEMPT && xds_CAInfo.rating <= CF_EIGHTEENPLUS)
               xds_ca_info.region[0].dimension[0].i_dimension_j = 1;
     }
     else {
           xds_ca_info.region[0].i_rating_region = 1;  
           xds_ca_info.i_region_count = 0;   
           xds_ca_info.region[0].i_dimension_count = 0;
     }

     switch(xds_CAInfo.rating){
        /* NONE,no rating */
        case RATINGNOTAVAILABLE:
           xds_ca_info.region[0].dimension[0].i_rating_value = 1;
           xds_ca_info.region[0].dimension[0].i_dimension_j = 0;
	break;                     

        /* MPAA rating */
        case NOTAPPLICABLE:  
        case GENERALPROGRAMMING:
        case PARENTALGUIDANCE:  
        case PARENTALGUIDANCE13:  
        case RESTRICTEDPROGRAMMING: 
        case NC17: 
        case XRATED: 
        case NOTRATED:
          xds_ca_info.region[0].dimension[0].i_rating_value = xds_CAInfo.rating - NOTAPPLICABLE + 1;
        break;
                         
        /*US TV rating */         
        case TV_Y: 
        case TV_Y7: 
          xds_ca_info.region[0].dimension[0].i_rating_value = xds_CAInfo.rating - TV_Y + 1;
        break;
                                    
        case TV_G: 
        case TV_PG:
        case TV_14: 
        case TV_MA:
          xds_ca_info.region[0].dimension[0].i_rating_value = xds_CAInfo.rating - TV_G + 1; 
        break;           
                        
        /*canadian english rating */ 
        case CE_EXEMPT: 
        case CE_CHILDREN: 
        case CE_CHILDREN8: 
        case CE_GENERALPROGRAMMING: 
        case CE_PARENTALGUIDANCE: 
        case CE_FOURTEENPLUS:   
        case CE_EIGHTEENPLUS:  
          xds_ca_info.region[0].dimension[0].i_rating_value = xds_CAInfo.rating - CE_EXEMPT;
        break;     

        /* canadian french rating */                    
        case CF_EXEMPT: 
        case CF_GENERAL:    
        case CF_EIGHTPLUS:      
        case CF_THIRTEENPLUS:   
        case CF_SIXTEENPLUS:   
        case CF_EIGHTEENPLUS: 
           xds_ca_info.region[0].dimension[0].i_rating_value = xds_CAInfo.rating - CF_EXEMPT;
        break;

        default :break;
    }

    if(0){//just for test
       int i, j;

       for (i=0; i<xds_ca_info.i_region_count; i++)
       {
            for (j=0; j<xds_ca_info.region[i].i_dimension_count; j++)

                   AM_XDS_DBG("\n[%s:%d] ---> tmp_xds_ca_info.region[%d]->i_rating_region:%d ,dimension[%d]->i_dimension_j:%d,i_rating_value:%d \n",
                                __FUNCTION__,__LINE__, i,
                                xds_ca_info.region[i].i_rating_region,j,
                                xds_ca_info.region[i].dimension[j].i_dimension_j,
                                xds_ca_info.region[i].dimension[j].i_rating_value);
       } 

    }

    return;
}

static void xds_format_rrt_rating_string(atsc_content_advisory_dr_t *tmp_xds_ca_info,char *rrt_rating)
{
       int i, j;

       for (i=0; i<tmp_xds_ca_info->i_region_count; i++)
       {
            for (j=0; j<tmp_xds_ca_info->region[i].i_dimension_count; j++)
	    {						
        	    if(rrt_rating[0] == 0)
		        sprintf(rrt_rating, "%d %d %d", 
				tmp_xds_ca_info->region[i].i_rating_region,
				tmp_xds_ca_info->region[i].dimension[j].i_dimension_j,
				tmp_xds_ca_info->region[i].dimension[j].i_rating_value);
		    else
			sprintf(rrt_rating, "%s,%d %d %d", rrt_rating, 
				tmp_xds_ca_info->region[i].i_rating_region,
				tmp_xds_ca_info->region[i].dimension[j].i_dimension_j,
				tmp_xds_ca_info->region[i].dimension[j].i_rating_value);
                   
	    }
	}
       if(strcmp(vchip_rrt_rating, rrt_rating))
       {
	        strcpy(vchip_rrt_rating,rrt_rating);
		   	vchip_changed = 1;
       }
      AM_XDS_DBG("\n[%s:%d] ----get rrt_rating string is :%s\n",__FUNCTION__,__LINE__,rrt_rating);
}
/*
** add api for invode access dvb.db to store xds items information 
** from NTSC live stream
*/

static sqlite3 * xds_get_sqlite_handle(void)
{
   sqlite3 *sqdb;

   AM_DB_HANDLE_PREPARE(sqdb);

   return sqdb;
}

/*
** delete the expired EVT_Table record 
*/
static AM_ErrorCode_t xds_delete_expired_events(sqlite3 *hdb)
{
	int now;
	char sql[128];
	char *errmsg;
	
	if (hdb == NULL)
		return AM_FAILURE;
		
	AM_XDS_DBG("\nDeleting expired xds events...\n");
	AM_EPG_GetUTCTime(&now);
	snprintf(sql, sizeof(sql), "delete from evt_table where end<%d", now);

	if (sqlite3_exec(hdb, sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		AM_XDS_DBG("\nDelete expired events failed: %s\n", errmsg ? errmsg : "Unknown");
		if (errmsg)
			sqlite3_free(errmsg);
		return AM_FAILURE;
	}

	AM_XDS_DBG("\nDelete expired xds events done!\n");

	return AM_SUCCESS;
}

static AM_ErrorCode_t xds_get_svr_si_table_items(sqlite3 *sqldb,int *db_ts_id,int *srv_db_id,int *source_id,int *src)
{
   AM_ErrorCode_t ret = AM_SUCCESS;
   int row = 1;
   int ts_dbid = -1,srv_dbid =-1,src_id = -1,tmp_source_id =-1;
   char sql[256];
   int major_chan_num = -1,minor_chan_num =-1,default_chan_num = -1;
   char namestring[256];

   /*Prepare sqlite3 stmts*/
   memset(sql, 0, sizeof(sql));

#if 0 //use fparam.frequency to acquire items from ts_table and srv_table
   snprintf(sql, sizeof(sql), "select db_id,src from ts_table where freq=%d",  fparam.frequency);
   snprintf(sql, sizeof(sql), "select db_id,src from ts_table where chan_num=%d",  ntsc_current_channumner);//need APK set correct virtual channel numner
   if(AM_DB_Select(sqldb, sql, &row, "%d,%d", &ts_dbid,&src_id) !=AM_SUCCESS || row == 0){
      AM_XDS_DBG("\n==== cannot get current ts(%d)!", ts_dbid);
      return AM_FAILURE;
   }

   AM_XDS_DBG("\n====current ATV ts_db_id :%d,src_id %d,freg:%d ,ntsc_current_channumner :0x%x \n",ts_dbid,src_id,fparam.frequency,ntsc_current_channumner);

   snprintf(sql, sizeof(sql), "select db_id,name,major_chan_num,minor_chan_num,chan_num,source_id from srv_table where db_ts_id=%d \
                                                                and src=%d limit 1", ts_dbid,src_id );
   if (AM_DB_Select(sqldb, sql, &row, "%d,%s:256,%d,%d,%d,%d", &srv_dbid,namestring, &major_chan_num,
                    &minor_chan_num,&default_chan_num,&tmp_source_id) != AM_SUCCESS || row == 0)
   {
       /*No such service*/
       AM_XDS_DBG("\n[%s:%d]==== cannot get current srv_table\n",__FUNCTION__,__LINE__);
       return AM_FAILURE;
   }
#else
   //directly use chan_num like 0x20000 means major numner is 2,minor numner is 0
  snprintf(sql, sizeof(sql), "select db_id,src,db_ts_id,name,major_chan_num,minor_chan_num,chan_num,source_id from srv_table where db_id=%d limit 1", ntsc_current_channumner);
  AM_XDS_DBG("\n%s",sql);

 if (AM_DB_Select(sqldb, sql, &row, "%d,%d,%d,%s:256,%d,%d,%d,%d", &srv_dbid,&src_id,&ts_dbid,namestring, &major_chan_num,
                    &minor_chan_num,&default_chan_num,&tmp_source_id) != AM_SUCCESS || row == 0)
   {
       /*No such service*/
       AM_XDS_DBG("\n[%s:%d]==== cannot get current srv_table\n",__FUNCTION__,__LINE__);
       return AM_FAILURE;
   }
#endif

   AM_XDS_DBG("\n==== current srv_dbid : (%d)!\n", srv_dbid);
   AM_XDS_DBG("\n==== current ts_dbid : (%d)!\n", ts_dbid);
   AM_XDS_DBG("\n==== current channel name: (%s)!\n", namestring);
   AM_XDS_DBG("\n==== current  major_chan_num:%d,minor_chan_num:%d,chan_num :0x%x ,source_id:%d\n",major_chan_num, minor_chan_num,default_chan_num,tmp_source_id);
  
   *db_ts_id = ts_dbid;
   *srv_db_id = srv_dbid;
   *source_id = tmp_source_id;
   *src = src_id;

   return ret;
}

static AM_ErrorCode_t xds_insert_evt_table_items(sqlite3 *sqldb,int src,int ts_dbid,int srv_dbid,int source_id)
{
   AM_ErrorCode_t ret = AM_SUCCESS;
   int starttime,endtime;
   int row = 1;
   sqlite3_stmt *stmt;
   char rrt_rating[256];
   AM_XDSInfo_t lastXdsInfo;
   memset(rrt_rating,0,sizeof(rrt_rating));

   //add new record to evt_table
   AM_XDS_DBG("\n==== insert new record into evt_table,sqldb:0x%x,src:%d,db_ts_id:%d,srv_db_id:%d,source_id:%d\n",sqldb,src,ts_dbid,srv_dbid,source_id);
   if(AM_DB_GetSTMT(&stmt,xds_sqllist[SQL_INSERT_EVTTABLE].sql_action_nam,xds_sqllist[SQL_INSERT_EVTTABLE].sql_action_cmd,0) != AM_SUCCESS)
   {
       AM_XDS_DBG("\n===== prepare insert events stmt failed");
       return AM_FAILURE;
   }

   ret =  AM_Get_XDSDataInfo(&lastXdsInfo);
   if(AM_SUCCESS != ret){
      AM_XDS_DBG("\n[%s:%d]=====get current XDSDataInfo failed",__FUNCTION__,__LINE__);
      return AM_FAILURE;
   }
 
   xds_extract_content_advisory(lastXdsInfo.rating);
   xds_format_rrt_rating_string(&xds_ca_info,rrt_rating);
   
   starttime = AM_XDS_2_SECONDS(lastXdsInfo.startTime.hour,lastXdsInfo.startTime.minute,0);
   endtime = AM_XDS_2_SECONDS(lastXdsInfo.progLen.hour,lastXdsInfo.progLen.minute,0);
   endtime +=starttime ;
   
   //insert evt_table items
#if 0//test code
   sqlite3_bind_int(stmt, 1, src);
   sqlite3_bind_int(stmt, 2, -1);
   sqlite3_bind_int(stmt, 3, ts_dbid);
   sqlite3_bind_int(stmt, 4, srv_dbid);
   sqlite3_bind_int(stmt, 5, 119);//event_id
   sqlite3_bind_text(stmt, 6,"Event-test-0", -1, SQLITE_STATIC);//event name
   sqlite3_bind_int(stmt, 7, 10000);//starttime
   sqlite3_bind_int(stmt, 8, 10000+999);//endtime
   sqlite3_bind_text(stmt, 9, "event-descr-test-0", -1, SQLITE_STATIC);//event descr
   sqlite3_bind_text(stmt, 10, "items-1", -1, SQLITE_STATIC);//items descr
   sqlite3_bind_text(stmt, 11, "exd-descr-test-2", -1, SQLITE_STATIC);//event extend descr
   sqlite3_bind_int(stmt, 12, 0);
   sqlite3_bind_int(stmt, 13, 0);
   sqlite3_bind_int(stmt, 14, 0);
   sqlite3_bind_int(stmt, 15, 0);
   sqlite3_bind_int(stmt, 16, source_id);
   sqlite3_bind_text(stmt, 17, "5 7 3", -1, SQLITE_STATIC);//rrt_rating string line "5 7 3,1 3 0"
#else
   sqlite3_bind_int(stmt, 1, src);
   sqlite3_bind_int(stmt, 2, -1);
   sqlite3_bind_int(stmt, 3, ts_dbid);
   sqlite3_bind_int(stmt, 4, srv_dbid);
   sqlite3_bind_int(stmt, 5, 0);//event_id
   sqlite3_bind_text(stmt, 6," ", -1, SQLITE_STATIC);//event name
   sqlite3_bind_int(stmt, 7, starttime);//starttime
   sqlite3_bind_int(stmt, 8, endtime);//endtime
   sqlite3_bind_text(stmt, 9, " ", -1, SQLITE_STATIC);//event descr
   sqlite3_bind_text(stmt, 10, " ", -1, SQLITE_STATIC);//items descr
   sqlite3_bind_text(stmt, 11, " ", -1, SQLITE_STATIC);//event extend descr
   sqlite3_bind_int(stmt, 12, 0);
   sqlite3_bind_int(stmt, 13, 0);
   sqlite3_bind_int(stmt, 14, 0);
   sqlite3_bind_int(stmt, 15, 0);
   sqlite3_bind_int(stmt, 16, source_id);
   sqlite3_bind_text(stmt,17, (const char*)rrt_rating, strlen(rrt_rating), SQLITE_STATIC);
#endif
   STEP_STMT(stmt,xds_sqllist[SQL_INSERT_EVTTABLE].sql_action_nam,xds_sqllist[SQL_INSERT_EVTTABLE].sql_action_cmd);

   return ret;
}

static AM_ErrorCode_t xds_update_evt_table_items(sqlite3 *sqldb,AM_xdsUserProcEvent_t event,int ts_dbid,int srv_dbid,int evt_dbid,int source_id)
{
   AM_ErrorCode_t ret = AM_SUCCESS;
   int row = 1;
   sqlite3_stmt *update_stmt;
   int index;
   int starttime,endtime,now = 0;
   char rrt_rating[256];
   AM_XDSInfo_t lastXdsInfo;
   char progname[256],aud_langs[256];
   int eit_schedule_flag=0,eit_pf_flag =0,i;

   AM_XDS_DBG("\n[%s:%d]--->event: 0x%x :source_id :%d,evt_dbid :%d\n",__FUNCTION__,__LINE__,event,source_id,evt_dbid);
   ret =  AM_Get_XDSDataInfo(&lastXdsInfo); 
   if(AM_SUCCESS != ret){
      AM_XDS_DBG("\n[%s:%d]=====get current XDSDataInfo failed",__FUNCTION__,__LINE__);
      return AM_FAILURE;
   }


   switch(event){
      case XDSPROGRAMTIMEEVENT://"update evt_table set start=?, end=? where db_id=?"
       //update record
       index = SQL_UPDATE_STARTEND;
       if (AM_DB_GetSTMT(&update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd,0) != AM_SUCCESS)
       {
           AM_XDS_DBG("\n=====prepare update events stmt failed");
           break;
       }

       starttime = AM_XDS_2_SECONDS(lastXdsInfo.startTime.hour,lastXdsInfo.startTime.minute,0);
       endtime = AM_XDS_2_SECONDS(lastXdsInfo.progLen.hour,lastXdsInfo.progLen.minute,0);
       endtime +=starttime ;
      
       if (endtime < now){
          AM_XDS_DBG("\n[%s:%d] event time :0x%x is expired current time :0x%x and dropped",__FUNCTION__,__LINE__,endtime,now);
          break;
       }
                   
       sqlite3_bind_int(update_stmt, 1, starttime);
       sqlite3_bind_int(update_stmt, 2, endtime);
       sqlite3_bind_int(update_stmt, 3, evt_dbid);
       STEP_STMT(update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd);
        
       break;
      
      case XDSPROGRAMNAMEEVENT://update srv_table set name=?,db_ts_id=?,eit_schedule_flag=?,eit_pf_flag=?,sub_langs=? where db_id=?
       index = SQL_UPDATE_SRVTABLE;
       if (AM_DB_GetSTMT(&update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd,0) != AM_SUCCESS)
       {
           AM_XDS_DBG("\n=====prepare update events stmt failed");
           break;
       }
       
       if(lastXdsInfo.currentXDSClass == CURRENTCLASSSTATE){
           eit_pf_flag = 1;
           strcpy(progname,lastXdsInfo.progTitle);
           sprintf(aud_langs, "%s %s", xds_get_languagestring(lastXdsInfo.audio.mainLang),xds_get_languagestring(lastXdsInfo.audio.sapLang));
       }
       else if(lastXdsInfo.currentXDSClass == FUTURECLASSSTATE){
           eit_schedule_flag = 1;
           strcpy(progname,lastXdsInfo.futureProgTitle);
           sprintf(aud_langs, "%s %s", xds_get_languagestring(lastXdsInfo.futureAudio.mainLang),xds_get_languagestring(lastXdsInfo.futureAudio.sapLang));
       }
       
       sqlite3_bind_text(update_stmt, 1, (const char*)progname, strlen(progname), SQLITE_STATIC);//save channel program name
       sqlite3_bind_int(update_stmt, 2, ts_dbid);
       sqlite3_bind_int(update_stmt, 3, eit_schedule_flag);
       sqlite3_bind_int(update_stmt, 4, eit_pf_flag);
       sqlite3_bind_text(update_stmt, 5, (const char*)progname, strlen(aud_langs), SQLITE_STATIC);//save audio_languages 
       sqlite3_bind_int(update_stmt, 6, srv_dbid);
       STEP_STMT(update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd);
       
       break;

      case XDSPROGRAMDESCRIPTIONEVENT://"update evt_table set descr=?,items=?,ext_descr=? where db_id=?" //now reserve for furture use if in need.
       index = SQL_UPDATE_DESCR;
       if (AM_DB_GetSTMT(&update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd,0) != AM_SUCCESS)
       {
           AM_XDS_DBG("\n=====prepare update events stmt failed");
           break;
       }

       sqlite3_bind_text(update_stmt,1, " ", -1, SQLITE_STATIC);//save channel program description
       sqlite3_bind_text(update_stmt,1, " ", -1, SQLITE_STATIC);//save program items
       sqlite3_bind_text(update_stmt,3, (const char*)progname, strlen(progname), SQLITE_STATIC);//save channel program exd-description
       sqlite3_bind_int(update_stmt, 4, evt_dbid);
       STEP_STMT(update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd);
       
       break;

      case XDSCONTENTADVISORYEVENT://"update evt_table set source_id=?,rrt_ratings=? where db_id=?"
       //update record
       memset(rrt_rating,0,sizeof(rrt_rating));
       xds_extract_content_advisory(lastXdsInfo.rating);
       xds_format_rrt_rating_string(&xds_ca_info,rrt_rating);
       index = SQL_UPDATE_RRT_RATING;

       if (AM_DB_GetSTMT(&update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd,0) != AM_SUCCESS)
       {
           AM_XDS_DBG("\n=====prepare update events stmt failed");
           break;
       }
         
       AM_XDS_DBG("\n[%s:%d]--->source_id :%d,evt_dbid :%d\n",__FUNCTION__,__LINE__,source_id,evt_dbid);   

       sqlite3_bind_int(update_stmt, 1, source_id);
       sqlite3_bind_text(update_stmt,2, (const char*)rrt_rating, strlen(rrt_rating), SQLITE_STATIC);
       sqlite3_bind_int(update_stmt, 3, evt_dbid);
       STEP_STMT(update_stmt,xds_sqllist[index].sql_action_nam,xds_sqllist[index].sql_action_cmd);
     
       break;

      default:
       AM_XDS_DBG("\n[%s] not support process event :%d save into databse!\n",__FUNCTION__,event);
       break;
   }

   return ret;
}

static AM_ErrorCode_t xds_store_useproc_event(AM_xdsUserProcEvent_t event)
{
     int row = 1;    
     AM_ErrorCode_t ret = AM_SUCCESS;
     char sql[256];
     AM_Bool_t need_update = AM_FALSE;
     int starttime, endtime, evt_dbid,event_id;
     char event_name[256],event_descr[256],event_ext_descr[256],rrt_rating[256];
     sqlite3 *sqldb = xds_get_sqlite_handle();     
     int ts_dbid=-1,srv_dbid=-1,source_id=-1,src=-1;
     static clear_expired_evt_table = 0;

     ret = xds_get_svr_si_table_items(sqldb,&ts_dbid,&srv_dbid,&source_id,&src);
     if(AM_SUCCESS != ret)
     {
        AM_XDS_DBG("\n[%s:%d] fail to get svr_si table items\n",__FUNCTION__,__LINE__);
     }
     //query wether it need update exist record or add new record.
     snprintf(sql, sizeof(sql), "select db_id,event_id,name,start,end ,descr,ext_descr,source_id,rrt_ratings from evt_table where db_ts_id=%d and db_srv_id=%d limit 1",ts_dbid, srv_dbid);
	 AM_XDS_DBG("\n%s",sql);

    if (AM_DB_Select(sqldb, sql, &row, "%d,%d,%s:256,%d,%d,%s:256,%s:256,%d,%s:256",&evt_dbid,&event_id,event_name, &starttime, &endtime,event_descr,
                     event_ext_descr,&source_id,rrt_rating) == AM_SUCCESS && row > 0)
     {
         need_update = AM_TRUE;
         AM_XDS_DBG("\n==== current evt_dbid : (%d),event_id:%d,starttime:%ld,endtime:%ld\n", evt_dbid,event_id,starttime,endtime);
         AM_XDS_DBG("\n==== current evt_name: (%s)!\n",event_name);
         AM_XDS_DBG("\n==== current event_descr: (%s)!\n",event_descr);
         AM_XDS_DBG("\n==== current event_ext_descr: (%s)!\n",event_ext_descr);

         AM_XDS_DBG("\n==== current  source_id:%d\n",source_id);
         AM_XDS_DBG("\n==== current rrt_rating: (%s)!\n",rrt_rating);
         AM_XDS_DBG("\n==== record already exist in evt_table and update it\n");

     }
     else
     {
         if(0 == clear_expired_evt_table){
            ret = xds_delete_expired_events(sqldb);
            if(AM_SUCCESS != ret)
            {
                AM_XDS_DBG("\n[%s:%d] fail to clear expited event in evt_table items\n",__FUNCTION__,__LINE__);
            }
            clear_expired_evt_table = 1;
         } 

         ret = xds_insert_evt_table_items(sqldb,src,ts_dbid,srv_dbid,source_id);
         if(AM_SUCCESS != ret)
         {
            AM_XDS_DBG("\n[%s:%d] fail to insert evt_table items\n",__FUNCTION__,__LINE__);
         }
     }
    
     if(AM_TRUE == need_update) {
         ret = xds_update_evt_table_items(sqldb,event,ts_dbid,srv_dbid,evt_dbid,source_id);      
         if(AM_SUCCESS != ret)
         {
            AM_XDS_DBG("\n[%s:%d] fail to update evt_table items\n",__FUNCTION__,__LINE__);
         }
     }

     return ret;
}
int Am_Get_Vchip_Change_Status()
{
	return vchip_changed;
}
void Am_Reset_Vchip_Change_Status()
{
	vchip_changed = 0;
}

#if 1
void AM_Dump_XDSDataInfo(AM_XDSInfo_t *info)
{
	UINT32 i;
	
	if (info == NULL)
	{
		AM_XDS_DBG("%s: info can not be NULL\n", __FUNCTION__);
		return;
	}

	AM_XDS_DBG("\n\n\n==========================================================\n");
	AM_XDS_DBG("						XDS Data\n");
	AM_XDS_DBG("==========================================================\n");
	AM_XDS_DBG("                Current Class Information                 \n");
	AM_XDS_DBG("               ---------------------------				\n");
	AM_XDS_DBG("StartTime=[%d hr %d min] Date=[%d/%d] \n",info->startTime.hour,
	            info->startTime.minute,info->startDate,info->startMonth);
	AM_XDS_DBG("ProgmLen =[%d hr %d min] TimeElapsed=[%d %d %d]\n",
		    info->progLen.hour,info->progLen.minute,info->timeElapsed.hour,
		    info->timeElapsed.minute,info->timeElapsedSec);
	AM_XDS_DBG("ProgTitle  = [%s] \n",info->progTitle);
	for(i=0;i<8;i++)
		AM_XDS_DBG("Description= [%s]\n",info->progDesc[i].title);
	AM_XDS_DBG("\n");
	for(i=0;i<32;i++)
		AM_XDS_DBG("Program Type : [%s] [%s]\n",xds_get_programtypestring(info->progType[i].basicType),
		           xds_get_programtypestring(info->progType[i].detailType));
	AM_XDS_DBG("Content Advisory Information : Rating %s ",xds_get_programratingstring(info->rating.rating));
	if(info->rating.fantasyViolence) AM_XDS_DBG("[FantasyViolence]");
	if(info->rating.violence) AM_XDS_DBG("[Violence]");
	if(info->rating.sexualSituations) AM_XDS_DBG("[SexualSituations]");
	if(info->rating.adultLanguage) AM_XDS_DBG("[AdultLanguage]");
	if(info->rating.ssDialog) AM_XDS_DBG("[SexuallySuggestiveDialog]");
	AM_XDS_DBG("\n");
	AM_XDS_DBG("MainAudioLanguage=%s SapAudioLang=%s\n",xds_get_languagestring(info->audio.mainLang),
		    xds_get_languagestring(info->audio.sapLang));	
	AM_XDS_DBG("MainAudioType    =%s SapAudioType=%s\n",xds_get_audiotypestring(info->audio.mainType),
		    xds_get_audiotypestring(info->audio.sapType));	
	for(i=0;i<8;i++)
		AM_XDS_DBG("CaptionServices%d = %d Lang = %s \n",i,info->service[i],
		            xds_get_languagestring(info->serviceLang[i]));
	AM_XDS_DBG("ARstartline=%d ARendLine=%d VideoSqueezed=%d Ratio=%s\n",
	            info->aspectRatio.startLine,info->aspectRatio.endLine,
		    info->aspectRatio.videoSqueezed,xds_get_aspectratio(info->aspectRatio.ratio));
	AM_XDS_DBG("CGMS Info : %s %s %d \n",xds_get_cgmsstring(info->cgmsInfo.copyControl),
		    xds_get_cgmsstring(info->cgmsInfo.analogProtectionSystem),
		    info->cgmsInfo.analogSourceBit);

	AM_XDS_DBG("==========================================================\n");
	AM_XDS_DBG("                Future Class Information                 \n");
	AM_XDS_DBG("               ---------------------------				\n");
	AM_XDS_DBG("StartTime=[%d hr %d min] Date=[%d/%d] \n",info->futureStartTime.hour,
		   info->futureStartTime.minute,info->futureStartDate,info->futureStartMonth);
	AM_XDS_DBG("ProgmLen =[%d hr %d min] TimeElapsed=[%d %d %d]\n",
		   info->futureProgLen.hour,info->futureProgLen.minute,info->futureTimeElapsed.hour,
		   info->futureTimeElapsed.minute,info->futureTimeElapsedSec);
	AM_XDS_DBG("ProgTitle  = [%s] \n",info->futureProgTitle);
	for(i=0;i<8;i++)
		AM_XDS_DBG("Description= [%s]\n",info->futureProgDesc[i].title);
	AM_XDS_DBG("\n");
	for(i=0;i<info->futureValidTypes;i++)
		AM_XDS_DBG("Program Type : [%s] [%s]\n",xds_get_programtypestring(info->futureProgType[i].basicType),
	                   xds_get_programtypestring(info->futureProgType[i].detailType));
	AM_XDS_DBG("Content Advisory Information : Rating %s ",xds_get_programratingstring(info->futureRating.rating));
	if(info->futureRating.fantasyViolence) AM_XDS_DBG("[FantasyViolence]");
	if(info->futureRating.violence) AM_XDS_DBG("[Violence]");
	if(info->futureRating.sexualSituations) AM_XDS_DBG("[SexualSituations]");
	if(info->futureRating.adultLanguage) AM_XDS_DBG("[AdultLanguage]");
	if(info->futureRating.ssDialog) AM_XDS_DBG("[SexuallySuggestiveDialog]");
	AM_XDS_DBG("\n");
	AM_XDS_DBG("MainAudioLanguage=%s SapAudioLang=%s\n",xds_get_languagestring(info->futureAudio.mainLang),
	           xds_get_languagestring(info->futureAudio.sapLang));	
	AM_XDS_DBG("MainAudioType    =%s SapAudioType=%s\n",xds_get_audiotypestring(info->futureAudio.mainType),
	           xds_get_audiotypestring(info->futureAudio.sapType));	
	for(i=0;i<8;i++)
		AM_XDS_DBG("CaptionServices%d = %d Lang = %s \n",i,info->futureService[i],xds_get_languagestring(info->futureServiceLang[i]));
	 AM_XDS_DBG("ARstartline=%d ARendLine=%d VideoSqueezed=%d Ratio=%s\n",
		   info->futureAspectRatio.startLine,info->futureAspectRatio.endLine,
		   info->futureAspectRatio.videoSqueezed,xds_get_aspectratio(info->futureAspectRatio.ratio));
	AM_XDS_DBG("CGMS Info : %s %s %d \n",xds_get_cgmsstring(info->futureCgmsInfo.copyControl),
		   xds_get_cgmsstring(info->futureCgmsInfo.analogProtectionSystem),
		   info->futureCgmsInfo.analogSourceBit);


	AM_XDS_DBG("==========================================================\n");
	AM_XDS_DBG("                Channel Info Class Information                 \n");
	AM_XDS_DBG("               --------------------------------\n");
	AM_XDS_DBG(" Network Name = %s\n",info->networkName);
	AM_XDS_DBG(" Station Name = %s \n Station Number = %s \n",info->station.name,info->station.chNum);
	AM_XDS_DBG(" TSID = %d\n",info->tsid);
	AM_XDS_DBG("==========================================================\n");
	
	AM_XDS_DBG("==========================================================\n");
	AM_XDS_DBG("                MISC Class Information                 \n");
	AM_XDS_DBG("               --------------------------------\n");
	AM_XDS_DBG(" Chan Low = %d Chan Hi =%d version = %d ID = %s\n\n\n",info->chMap.chLow,
	           info->chMap.chHi,info->chMap.version,info->chMap.chId);
	
	return;
}

void AM_Print_XDSDataInfo(void)
{
	AM_Dump_XDSDataInfo(xdsContext->XDSBuffer);
}
#endif

