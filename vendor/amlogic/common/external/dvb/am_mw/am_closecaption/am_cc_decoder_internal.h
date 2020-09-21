/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _AM_CC_DECODER_INTERNAL_H_
#define _AM_CC_DECODER_INTERNAL_H_

#include <am_types.h>


/*
** add for NTSC vbi E608 close caption
*/
/*
 * Closed Caption Events
 *
 * Used for Notifying the Application
 *
 */
typedef enum AM_CCEvent {
        CCEVENT_RESERVED = 0,
        /*
         * XDS - Extended Data Support
         *
         */
        XDSCONTROLCODE,
        XDSDATA,
        /*
         * EIA-708 Events/Commands
         */
        ENDOFTEXT,
        FORMFEED,
        CARRIAGERETURN,
        HCR,
        PRINTABLETEXT,
        PRINTABLETEXTEXTENDED1,
        EXTENDED1,
        P16,
        SETWINDOW0,
        SETWINDOW1,
        SETWINDOW2,
        SETWINDOW3,
        SETWINDOW4,
        SETWINDOW5,
        SETWINDOW6,
        SETWINDOW7,
        DEFINEWINDOW0,
        DEFINEWINDOW1,
        DEFINEWINDOW2,
        DEFINEWINDOW3,
        DEFINEWINDOW4,
        DEFINEWINDOW5,
        DEFINEWINDOW6,
        DEFINEWINDOW7,
        DELETEWINDOWS,
        DISPLAYWINDOWS,
        HIDEWINDOWS,
        CLEARWINDOWS,
        TOGGLEWINDOWS,
        SETWINDOWATTRIBUTES,
        SETPENATTRIBUTES,
        SETPENCOLOR,
         SETPENLOCATION,
        DELAY,
        DELAYCANCEL,
        RESET,
        COMMANDPARAMETERS,
        CCLOGO,
        /*
         * EIA -608 events/commands
         */
        /* First Level Commands */
        UNDECODED608DATA,
        MISCCONTROLCODE,
        MIDROWCONTROLCODE,
        PREAMBLEADDRESSCODE,
        EXTENDEDCODE,
        /* Miscellanous Control Codes */
        PRIMARYLINE21CODE,
        SECONDARYLINE21CODE,
        RESUMECAPTIONLOADING,
        BACKSPACE,
        DELETETOEOR,
        ROLLUP2ROWS,
        ROLLUP3ROWS,
        ROLLUP4ROWS,
        FLASHON,
        RESUMEDIRECTCAPTIONING,
        TEXTRESTART,
        RESUMETEXTDISPLAY,
        ERASEDISPLAYEDMEMORY,
        ERASENONDISPLAYEDMEMORY,
        ENDOFCAPTION,
        TABOFFSET1,
        TABOFFSET2,
        TABOFFSET3,
         /* MIDROW CODES */
        WHITENOUNDERLINE,
        WHITEUNDERLINE,
        GREENNOUNDERLINE,
        GREENUNDERLINE,
        BLUENOUNDERLINE,
        BLUEUNDERLINE,
        CYANNOUNDERLINE,
        CYANUNDERLINE,
        REDNOUNDERLINE,
        REDUNDERLINE,
        YELLOWNOUNDERLINE,
        YELLOWUNDERLINE,
        MAGENTANOUNDERLINE,
        MAGENTAUNDERLINE,
        ITALICSNOUNDERLINE,
        ITALICSUNDERLINE,
        /* Pramble Address Codes */
        WHITEINDENT0NOUNDERLINE,
        WHITEINDENT0UNDERLINE,
        WHITEINDENT4NOUNDERLINE,
        WHITEINDENT4UNDERLINE,
        WHITEINDENT8NOUNDERLINE,
        WHITEINDENT8UNDERLINE,
        WHITEINDENT12NOUNDERLINE,
        WHITEINDENT12UNDERLINE,
        WHITEINDENT16NOUNDERLINE,
        WHITEINDENT16UNDERLINE,
        WHITEINDENT20NOUNDERLINE,
        WHITEINDENT20UNDERLINE,
        WHITEINDENT24NOUNDERLINE,
        WHITEINDENT24UNDERLINE,
        WHITEINDENT28NOUNDERLINE,
        WHITEINDENT28UNDERLINE,
        ROW1,
        ROW2,
        ROW3,
        ROW4,
        ROW5,
        ROW6,
        ROW7,
        ROW8,
        ROW9,
        ROW10,
        ROW11,
        ROW12,
        ROW13,
        ROW14,
        ROW15,
        /* Background/Foreground Attribute Commands */
        IGNORECODE,
        BGWHITEOPAQUE,
        BGWHITESEMITRANSPARENT,
        BGGREENOPAQUE,
        BGGREENSEMITRANSPARENT,
        BGBLUEOPAQUE,
        BGBLUESEMITRANSPARENT,
        BGCYANOPAQUE,
        BGCYANSEMITRANSPARENT,
        BGREDOPAQUE,
        BGREDSEMITRANSPARENT,
        BGYELLOWOPAQUE,
        BGYELLOWSEMITRANSPARENT,
        BGMAGENTAOPAQUE,
        BGMAGENTASEMITRANSPARENT,
        BGBLACKOPAQUE,
        BGBLACKSEMITRANSPARENT,
        BGTRANSPARENT,
        FGBLACK,
        FGBLACKUNDERLINE,
         /* Character Set to be chosen */
        STANDARDNORMALFONT,
        STANDARDDOUBLEFONT,
        PRIVATEFIRSTSET,
        PRIVATESECONDSET,
        CHINESESTANDARDSET,
        KOREANSTANDARDSET,
        REGISTEREDSET,
        /* NTSC closed caption events*/
        NTSCPARSE,
        /* 708 service events */
        SERVICEPARSE,
        DECODERDELAYEND,
        /* CC Graphics Events*/
        UPDATEDISPLAY,
        ENABLECCEVENT, // SDF: for reset.
        DISCARDCCDATA,
        DISCARDPREVCMD,
        CLEARDISPLAY,
        CCTASKDELETE,
        CCREINITIALIZE,
        CCREINITDONE,


        /* additonal eia 608 event */
        ADDITIONAL608DATA,
        /* Luma pam data */
        LUMAPAMDATA,
        /* Special Reset Condition */
        SEQERRORRESET,
        /* Non Real Time Video Data */
        NONREALTIMEVIDEODATA,
        /* Special Printable Character */
        SPECIALPRINTABLETEXT,
        SPECIALPRINTABLETEXT1, /* no backspace */
        /* Some illegal/Unkown code */
        ILLEGALCODE
} AM_CCEvent_t;

/** Closed Caption Service Type */
typedef enum AM_ServiceType{
    INVALIDSERVICE = 0,                 /*  Illegal service type                                             */
    PRIMARYCAPTIONSERVICE,              /*  Supports captions in primary language accompanying program audio */
    SECONDARYLANGUAGESERVICE1,          /*  Supports captions translated to secondary language               */
    SECONDARYLANGUAGESERVICE2,          /*  not preassigned, depends upon discretion of caption provider     */
    SECONDARYLANGUAGESERVICE3,
    SECONDARYLANGUAGESERVICE4,
    SECONDARYLANGUAGESERVICE5,
    /*  enum values 7 - 63 are reserved for Extended Caption Services    */
    PRIMARYLINE21DATASERVICE1 = 64,     /*  Field1 Data Services Channel 1  */
    /*  includes CC1 = Primary Synchronous Caption Service                  */
    /*  T1  = First Text Service  */
    PRIMARYLINE21DATASERVICE2,          /*  Field1 Data Services Channel 2  */
    /*  includes CC2 = Special Non Synch use captions */
    /*  T2 = Second Text Services */
    SECONDARYLINE21DATASERVICE1,        /*  Field2 Data Services Channel 1  */
    /*  includes CC3 = Secondary Synchronous Caption Services   */
    /*  T3  = Third text service  */
    SECONDARYLINE21DATASERVICE2,        /*  Field2 Data Services Channel 2  */
    /*  includes CC4 = Special non synch use captions */
    /*  T4 = Fourth Text Service  */
    EXTENDEDDATASERVICEONLY,            /*  Extended Data Service XDS only  */
    RESERVEDSERVICE = 1000              /*  Reserve Service Type            */
} AM_ServiceType_t;

typedef enum AM_CCDataChannel{
    ILLEGALCHANNEL,
    CC_CHANNEL1,
    CC_CHANNEL2
} AM_CCDataChannel_t;

typedef struct AM_CCUserData {
    AM_CCEvent_t            mainCommand;    /* Used for EIA608 */
    AM_CCEvent_t            level1Event;    /* Used for decoded EIA608 */
    AM_CCEvent_t            level2Event;    /* Used for decoded EIA608 */
    AM_CCDataChannel_t      channel;
    int	                    buf1;
    int                     buf2;
    AM_ServiceType_t        serviceType;
} AM_CCUserData_t;

#endif     /*_AM_CC_DECODER_INTERNAL_H_ */

