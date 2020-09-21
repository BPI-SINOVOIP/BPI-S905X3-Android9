/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_xds_internal.h
                          ®
 * \brief CC模å部数æ*
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2011-12-27: create the document
 ***************************************************************************/

#ifndef _AM_XDS_INTERNAL_H
#define _AM_XDS_INTERNAL_H

#include <pthread.h>
#include <am_types.h>


/*
** AM_XDSPROGType : 
*/
typedef enum AM_XDSProgType{
	AM_XDSPROG_UNAVAILABLE = 0,
	/* Basic Group */
	AM_XDSPROG_EDU = 0x20,
	AM_XDSPROG_ENTERTAINEMENT,
	AM_XDSPROG_MOVIE,
	AM_XDSPROG_NEWS,
	AM_XDSPROG_RELIGIOUS,
	AM_XDSPROG_SPORTS,
	AM_XDSPROG_OTHERS,
	/* Detail Group */
	AM_XDSPROG_ACTION,
	AM_XDSPROG_ADVERTISEMENT,
	AM_XDSPROG_ANIMATED,
	AM_XDSPROG_ANTHOLOGY,
	AM_XDSPROG_AUTOMOBILE,
	AM_XDSPROG_AWARDS,
	AM_XDSPROG_BASEBALL,
	AM_XDSPROG_BASKETBALL,
	AM_XDSPROG_BULLETIN,
	AM_XDSPROG_BUSINESS,
	AM_XDSPROG_CLASSICAL,
	AM_XDSPROG_COLLEGE,
	AM_XDSPROG_COMBAT,
	AM_XDSPROG_COMEDY,
	AM_XDSPROG_COMMENTARY,
	AM_XDSPROG_CONCERT,
	AM_XDSPROG_CONSUMER,
	AM_XDSPROG_CONTEMPORARY,
	AM_XDSPROG_CRIME,
	AM_XDSPROG_DANCE,
	AM_XDSPROG_DOCUMENTARY,
	AM_XDSPROG_DRAMA,
	AM_XDSPROG_ELEMENTARY,
	AM_XDSPROG_EROTICA,
	AM_XDSPROG_EXERCISE,
	AM_XDSPROG_FANTASY,
	AM_XDSPROG_FARM,
	AM_XDSPROG_FASHION,
	AM_XDSPROG_FICTION,
	AM_XDSPROG_FOOD,
	AM_XDSPROG_FOOTBALL,
	AM_XDSPROG_FOREIGN,
	AM_XDSPROG_FUNDRAISER,
	AM_XDSPROG_GAMEQUIZ,
	AM_XDSPROG_GARDEN,
	AM_XDSPROG_GOLF,
	AM_XDSPROG_GOVERNMENT,
	AM_XDSPROG_HEALTH,
	AM_XDSPROG_HIGHSCHOOL,
	AM_XDSPROG_HISTORY,
	AM_XDSPROG_HOBBY,
	AM_XDSPROG_HOCKEY,
	AM_XDSPROG_HOME,
	AM_XDSPROG_HORROR,
	AM_XDSPROG_INFORMATION,
	AM_XDSPROG_INSTRUCTION,
	AM_XDSPROG_INTERNATIONAL,
	AM_XDSPROG_INTERVIEW,
	AM_XDSPROG_LANGUAGE,
	AM_XDSPROG_LEGAL,
	AM_XDSPROG_LIVE,
	AM_XDSPROG_LOCAL,
	AM_XDSPROG_MATH,
	AM_XDSPROG_MEDICAL,
	AM_XDSPROG_MEETING,
	AM_XDSPROG_MILLITARY,
	AM_XDSPROG_MINISERIES,
	AM_XDSPROG_MUSIC,
	AM_XDSPROG_MYSTERY,
	AM_XDSPROG_NATIONAL,
	AM_XDSPROG_NATURE,
	AM_XDSPROG_POLICE,
	AM_XDSPROG_POLITICS,
	AM_XDSPROG_PREMIERE,
	AM_XDSPROG_PRERECORDED,
	AM_XDSPROG_PRODUCT,
	AM_XDSPROG_PROFESSIONAL,
	AM_XDSPROG_PUBLICGR,
	AM_XDSPROG_RACING,
	AM_XDSPROG_READING,
	AM_XDSPROG_REPAIR,
	AM_XDSPROG_REPEAT,
	AM_XDSPROG_REVIEW,
	AM_XDSPROG_ROMANCE,
	AM_XDSPROG_SCIENCE,
	AM_XDSPROG_SERIES,
	AM_XDSPROG_SERVICE,
	AM_XDSPROG_SHOPPING,
	AM_XDSPROG_SOAPOPERA,
	AM_XDSPROG_SPECIAL,
	AM_XDSPROG_SUSPENSE,
	AM_XDSPROG_TALK,
	AM_XDSPROG_TECHNICAL,
	AM_XDSPROG_TENNIS,
	AM_XDSPROG_TRAVEL,
	AM_XDSPROG_VARIETY,
	AM_XDSPROG_VIDEO,
	AM_XDSPROG_WEATHER,
	AM_XDSPROG_WESTERN
} AM_XDSProgType_t;

/** AM_XDSProgGroup : */
typedef struct AM_XDSProgGroup{
	AM_XDSProgType_t	basicType;
	AM_XDSProgType_t	detailType;
} AM_XDSProgGroup_t;

/** AM_XDSProgRating : */
typedef enum AM_XDSProgRating{
	RATINGNOTAVAILABLE,
	NOTAPPLICABLE,
	GENERALPROGRAMMING,
	PARENTALGUIDANCE,
	PARENTALGUIDANCE13,
	RESTRICTEDPROGRAMMING,
	NC17,
	XRATED,
	NOTRATED,
	TV_Y,
	TV_Y7,
	TV_G,
	TV_PG,
	TV_14,
	TV_MA,
	CE_EXEMPT, /*canadian english rating */
	CE_CHILDREN,
	CE_CHILDREN8,
	CE_GENERALPROGRAMMING,
	CE_PARENTALGUIDANCE,
	CE_FOURTEENPLUS,
	CE_EIGHTEENPLUS,
	CF_EXEMPT, /* canadian french rating */
	CF_GENERAL,
	CF_EIGHTPLUS,
	CF_THIRTEENPLUS,
	CF_SIXTEENPLUS,
	CF_EIGHTEENPLUS
} AM_XDSProgRating_t;

typedef struct AM_ContentAdvisoryInfo{
	AM_XDSProgRating_t	rating;
	AM_Bool_t		fantasyViolence;
	AM_Bool_t		violence;
	AM_Bool_t		sexualSituations;
	AM_Bool_t		adultLanguage;
	AM_Bool_t		ssDialog;
} AM_ContentAdvisoryInfo_t;
/*  AM_XDSTime : */
typedef struct AM_XDSTime{
	unsigned int	minute;
	unsigned int	hour;
} AM_XDSTime_t;

/*  AM_XDSLanguage : */
typedef enum AM_XDSLanguage{
	XUNKNOWN,
	XENGLISH,
	XSPANISH,
	XFRENCH,
	XGERMAN,
	XITALIAN,
	XOTHER,
	XNONE
} AM_XDSLanguage_t;

/** AM_XDSAudioType: */
typedef enum AM_XDSAudioType{
	UNKNOWNTYPE,
	MONO,
	SIMULATEDSTEREO,
	TRUESTEREO,
	STEREOSURROUND,
	DATASERVICE,
	VIDEODESCRIPTIONS,
	NONPROGAUDIO,
	SPECIALEFFECTS,
	OTHERTYPE,
	NONETYPE
} AM_XDSAudioType_t;

/** AM_XDSAudioInfo: */
typedef struct AM_XDSAudioInfo{
	AM_XDSLanguage_t	mainLang;
	AM_XDSAudioType_t	mainType;
	AM_XDSLanguage_t	sapLang;
	AM_XDSAudioType_t	sapType;
} AM_XDSAudioInfo_t;

/** AM_XDSCaptionServices: */
typedef enum AM_XDSCaptionSerives {
	AM_UNKNOWNSERV = 0,
	AM_CS1,
	AM_CS2,
	AM_CS3,
	AM_CS4,
	AM_CS5,
	AM_CS6,
	AM_CS7,
	AM_CS8
} AM_XDSCaptionServices_t;

/** Type of Aspect Ratio */
typedef enum AM_XDSAR{
        VISTAVISION = 0,		/* 1.78:1 TO 1.85:1 */
	CINEMASCOPE,		/* 2.20:1 TO 2.35:1 */
	DEFAULTAR,		/* 4:3 */
	UNKNOWNAR
} AM_XDSAR_t;



/** AM_XDSAspectRatio: */
typedef struct AM_XDSAspectRatio{
	unsigned int	 startLine;
	unsigned int	 endLine;
	AM_Bool_t	 videoSqueezed;
	AM_XDSAR_t ratio;
} AM_XDSAspectRatio_t;


/** AM_XDSStationCode: */
typedef struct AM_XDSStationCode{
	char	name[4];
	char	chNum[2];
} AM_XDSStationCode_t;


/** AM_XDSSupplement: */
typedef struct AM_XDSSupplement{
	AM_Bool_t	field1;
	AM_Bool_t	lineNum;
} AM_XDSSupplement_t;


/** AM_XDSTimeZone: */
typedef struct AM_XDSTimeZone{
	unsigned int	offset;
	AM_Bool_t	present;
} AM_XDSTimeZone_t;


/** AM_XDSCompositePacket 1 */
typedef struct AM_XDSCompP1{
	AM_XDSProgType_t  	   type[5];
	AM_ContentAdvisoryInfo_t   rating;
	AM_XDSTime_t		   progLen;
	AM_XDSTime_t		   timeShow;
	char		           title[22];
} AM_XDSCompP1_t;


/** AM_XDSCompositePacket 2 */
typedef struct AM_XDSCompP2_t{
	AM_XDSTime_t		   startTime;
	unsigned int		   startDate;
	unsigned int		   startMonth;
	AM_XDSAudioInfo_t	   audio;
	AM_XDSCaptionServices_t	   services[2];
	AM_XDSLanguage_t	   serviceLanguage[2];
	AM_XDSStationCode_t	   station;
	char			   networkName[32];
} AM_XDSCompP2_t;


/** AM_XDSProgramDescription Row 1 to 8 */
typedef struct AM_XDSProgDesc{
	char title[33];	/* Max 32 chars + NULL */
} AM_XDSProgDesc_t;


/** CGMS Information : */
typedef enum AM_CgmsInfo{
	UNKNOWNCONDITION,
	COPYPERMITTED,
	INVALIDCONDITION,
	ONEGENERATIONCOPY,
	COPYNOTPERMITTED,
	NOAPS,
	PSPONSPLITBURSTOFF,
	PSPON2LINESPLITBURSTON,
	PSPON4LINESPLITBURSTON
} AM_CgmsInfo_t;

typedef struct AM_CgmsDataStructure{
	AM_CgmsInfo_t	copyControl;
	AM_CgmsInfo_t	analogProtectionSystem;
	AM_Bool_t	analogSourceBit;
} AM_CgmsDataStructure_t;

typedef struct AM_XDSChannelMap{
	unsigned int	chLow;
	unsigned int	chHi;
	unsigned int	version;
	char		chId[6];
} AM_XDSChannelMap_t;

/*
 * XDS State Machine :
 */
typedef enum AM_XDSState{
        /* Classses */
        ILLEGALSTATE,
        STARTSTATE,
        CONTINUESTATE,
        ENDSTATE,
        CURRENTCLASSSTATE,
        FUTURECLASSSTATE,
        CHANNELCLASSSTATE,
        MISCCLASSSTATE,
        PUBLICCLASSSTATE,
        RESERVEDCLASSSTATE,
        UNDEFINEDCLASSSTATE,
        ALLCLASSSTATE,
        /* Types */
        ILLEGALTYPE,
        PINSTATE,
        LENTIMESTATE,
        PROGNAMESTATE,
        PROGTYPESTATE,
        PROGRATINGSTATE,
        AUDIOSERVICESSTATE,
        CAPTIONSERVICESSTATE,
        ASPECTRATIOSTATE,
        CGMSSTATE,
        COMPPKT1STATE,
        COMPPKT2STATE,
        PROGDESCSTATE1,
        PROGDESCSTATE2,
        PROGDESCSTATE3,
        PROGDESCSTATE4,
        PROGDESCSTATE5,
        PROGDESCSTATE6,
        PROGDESCSTATE7,
        PROGDESCSTATE8,
        NETWORKNAMESTATE,
        NATIVECHANNELSTATE,
        TAPEDELAYSTATE,
        TSIDSTATE,
  	TIMEOFDAYSTATE,
        IMPULSESTATE,
        SUPPLEMENTSTATE,
        LOCALTIMEZONESTATE,
        OOBSTATE,
        CHMAPPOINTERSTATE,
        CHMAPHEADERSTATE,
        CHMAPPACKETSTATE,
        WEATHERCODESTATE,
        WEATHERMSGSTATE
} AM_XDSState_t;


/** XDSINFO STRUCTURE : */
typedef struct AM_XDSInfo{
        AM_XDSState_t                  currentXDSClass;                /*indicate XDSClass state*/
	/* Current Class */
	AM_XDSTime_t			startTime;   			/* Current Class */
	unsigned int			startDate;
	unsigned int			startMonth;
	AM_XDSTime_t            	progLen;
	AM_XDSTime_t            	timeElapsed;
	unsigned int			timeElapsedSec;			/* Time Elapsed in Second */
	char				progTitle[33];			/* Max 32 chars + NULL */
	AM_XDSProgGroup_t		progType[32];
	AM_ContentAdvisoryInfo_t     	rating;
	AM_XDSAudioInfo_t		audio;
	AM_XDSCaptionServices_t  	service[8];
	AM_XDSLanguage_t		serviceLang[8];
	AM_XDSAspectRatio_t		aspectRatio;
	AM_Bool_t			cgmsInfoPresent; 		/* CGMS Info Parsing  - Refer EIA702 */
	AM_CgmsDataStructure_t	        cgmsInfo;
	AM_XDSCompP1_t			pkt1;
	AM_XDSCompP2_t			pkt2;
	AM_XDSProgDesc_t		progDesc[8];
	/* Future Class */
	AM_XDSTime_t    		futureStartTime;
	unsigned int			futureStartDate;
	unsigned int			futureStartMonth;
	AM_XDSTime_t			futureProgLen;
	AM_XDSTime_t			futureTimeElapsed;
	unsigned int			futureTimeElapsedSec;
	char				futureProgTitle[33];	/* Max 32 chars + NULL */
	AM_XDSProgGroup_t		futureProgType[32];
	unsigned int			futureValidTypes;		/* Number of valid Type available */
	AM_ContentAdvisoryInfo_t        futureRating;
	AM_XDSAudioInfo_t		futureAudio;
	AM_XDSCaptionServices_t	        futureService[8];
	AM_XDSLanguage_t		futureServiceLang[8];
	AM_XDSAspectRatio_t	        futureAspectRatio;
	AM_Bool_t			futureCgmsInfoPresent;	 /* CGMS Info Parsing  Refer EIA702 */
	AM_CgmsDataStructure_t	        futureCgmsInfo;
	AM_XDSCompP1_t			futurePkt1;
	AM_XDSCompP2_t			futurePkt2;
	AM_XDSProgDesc_t		futureProgDesc[8];
	/* Channel Info Class */
	char				networkName[33];		/* Max 32 chars + NULL */
	AM_XDSStationCode_t	        station;
	AM_XDSTime_t			delayTime;
	short				tsid;					/* TSID Info  - Refer EIA752 */
	/* Misc CLass */
	AM_XDSTime_t			currentDayTime;
	unsigned int			currentDate;
	unsigned int			currentMonth;
	unsigned int			currentDay;
	unsigned int			currentYear;
	AM_XDSTime_t			captureStartTime;
	unsigned int			captureStartDate;
	unsigned int			captureStartMonth;
	AM_XDSTime_t			captureProgLen;
	AM_XDSSupplement_t		supplementInfo[32];
	AM_XDSTimeZone_t		timeZoneOffset;
	unsigned short			oobChannelNumber;
	AM_XDSChannelMap_t		chMap;			/* Channel Map Info Pkt - Refer EIA745 */
	/* Public Service Class */
	AM_Bool_t			weatherCodePresent;		/* Weather Code Not Supported - Refer FIPS PUB 6-4 */
	char				message[32];
} AM_XDSInfo_t;


/*  XDS Callback Event used by the application */

typedef enum AM_xdsUserProcEvent{
	XDSPROGRAMTIMEEVENT=0,
	XDSPROGRAMNAMEEVENT,
	XDSPROGRAMTYPEEVENT,
	XDSPROGRAMDESCRIPTIONEVENT,
	XDSCAPTIONSERVICESEVENT,
	XDSCONTENTADVISORYEVENT,
	XDSAUDIOSERVICESEVENT,
	XDSASPECTRATIOEVENT,
	XDSCGMSEVENT,
	XDSDATETIMEEVENT,
	XDSNETWORKINFOEVENT,
	XDSWEATHERINFOEVENT
} AM_xdsUserProcEvent_t;


/*  userProcMask : is a bit mask which can mask out
 * any of the xds callback events.
 */
/** userProcMask : is a bit mask which can mask out
 * any of the xds callback events.
 */
#define XDS_PROGRAM_TIME_EVENT                          (1<<XDSPROGRAMTIMEEVENT)
#define XDS_PROGRAM_NAME_EVENT                          (1<<XDSPROGRAMNAMEEVENT)
#define XDS_PROGRAM_TYPE_EVENT                          (1<<XDSPROGRAMTYPEEVENT)
#define XDS_PROGRAM_DESCRIPTION_EVENT                   (1<<XDSPROGRAMDESCRIPTIONEVENT)
#define XDS_CAPTION_SERVICES_EVENT                      (1<<XDSCAPTIONSERVICESEVENT)
#define XDS_CONTENT_ADVISORY_EVENT                      (1<<XDSCONTENTADVISORYEVENT)
#define XDS_AUDIO_SERVICES_EVENT                        (1<<XDSAUDIOSERVICESEVENT)
#define XDS_ASPECT_RATIO_EVENT                          (1<<XDSASPECTRATIOEVENT)
#define XDS_CGMS_EVENT                                  (1<<XDSCGMSEVENT)
#define XDS_DATE_TIME_EVENT                             (1<<XDSDATETIMEEVENT)
#define XDS_NETWORK_INFO_EVENT                          (1<<XDSNETWORKINFOEVENT)
#define XDS_WEATHER_INFO_EVENT                          (1<<XDSWEATHERINFOEVENT)


/*
 * AM_XDSContext :
 *
 */
typedef struct AM_XDSContext {
	AM_XDSInfo_t		*XDSBuffer;
        pthread_mutex_t 	lock;
	unsigned int		pinParamCount;
	unsigned int		lenParamCount;
	unsigned int		progNameCount;
	char	        	progTitle[33];
	unsigned int		progTypeCount;
	unsigned int		progRatingCount;
	unsigned char	        contentAdvisoryPrevData;
	AM_Bool_t	        contentAdvisoryWait;
	unsigned int		audioCount;
	unsigned int		csCount;
	unsigned int		aspectRatioCount;
	unsigned int		networkNameCount;
	char			networkName[33];
	unsigned int		nativeChannelCount;
	AM_XDSStationCode_t	station;
	unsigned int 		tapeDelayCount;
	unsigned int		tsidCount;
	unsigned short		tsidData;
	unsigned int		timeOfDayCount;
	unsigned int  		impulseCount;
	unsigned int  		supplementCount;
	unsigned int		localTimeZoneCount;
	unsigned int		oobCount;
	unsigned int		weatherCodeCount;
	unsigned int 		weatherMessageCount;
	char		 	message[32];
	unsigned int		compPkt1Count;
	unsigned int 		compPkt2Count;
	unsigned int		progDescCount;
	AM_XDSProgDesc_t        progDesc[8];
	unsigned int  		progDescLevel2Count;
	unsigned int		cgmsCount;
	unsigned int 		chMapCount;
	AM_XDSState_t		currentXDSState;
	AM_XDSState_t		currentXDSClass;
	AM_XDSState_t		currentXDSType;
	AM_Bool_t		startCaptureXDS;
	unsigned char		currentXDSChecksum;

} AM_XDSContext_t;


typedef struct AM_XDSSqlite {
       const char *sql_action_cmd;
       const char *sql_action_nam;
} AM_XDSSqlite_t;

enum AM_XDSSql
{
  SQL_UPDATE_STARTEND =0,
  SQL_UPDATE_RRT_RATING,
  SQL_UPDATE_EVENTNAME,
  SQL_UPDATE_DESCR,
  SQL_UPDATE_SRVTABLE,
  SQL_INSERT_EVTTABLE,
  SQL_MAX_STMT
};


#endif
