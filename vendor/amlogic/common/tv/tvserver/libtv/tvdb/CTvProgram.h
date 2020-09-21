/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */


#if !defined(_CTVPROGRAM_H)
#define _CTVPROGRAM_H

#include "CTvDatabase.h"
#include "CTvChannel.h"
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <stdlib.h>
#include "CTvLog.h"
using namespace android;

class CTvEvent;
class CTvProgram : public LightRefBase<CTvProgram> {
public:

    static const int TYPE_UNKNOWN = 0;

    static const int TYPE_TV    =  4;

    static const int TYPE_RADIO =  2;

    static const int TYPE_ATV   =  3;

    static const int TYPE_DATA  =  5;

    static const int TYPE_DTV   = 1 ;
    /** PVR/Timeshifting playback program*/
    static const int TYPE_PLAYBACK =  6;

    static const int PROGRAM_SKIP_NO = 0;
    static const int PROGRAM_SKIP_YES = 1;
    static const int PROGRAM_SKIP_UNKOWN = 2;

public:
    class Element {
    private :
        int mpid;

    public :
        Element(int pid)
        {
            this->mpid = pid;
        }

        int getPID()
        {
            return mpid;
        }
    };



public:
    class MultiLangElement : public Element {
    private :
        String8 mlang;

    public :
        MultiLangElement(int pid, String8 lang): Element(pid)
        {
            this->mlang = lang;
        }

        String8 getLang()
        {
            return mlang;
        }
    };

public :
    class Video : public Element {
    public:
        /**MPEG1/2*/
        static const int FORMAT_MPEG12 = 0;
        /**MPEG4*/
        static const int FORMAT_MPEG4  = 1;
        /**H.264*/
        static const int FORMAT_H264   = 2;
        /**MJPEG*/
        static const int FORMAT_MJPEG  = 3;
        /**Real video*/
        static const int FORMAT_REAL   = 4;
        /**JPEG*/
        static const int FORMAT_JPEG   = 5;
        /**Microsoft VC1*/
        static const int FORMAT_VC1    = 6;
        /**AVS*/
        static const int FORMAT_AVS    = 7;
        /**YUV*/
        static const int FORMAT_YUV    = 8;
        /**H.264 MVC*/
        static const int FORMAT_H264MVC = 9;
        /**QJPEG*/
        static const int FORMAT_QJPEG  = 10;

        Video(int pid, int fmt): Element(pid)
        {
            this->mformat = fmt;
        }

        int getFormat()
        {
            return mformat;
        }
    private :
        int mformat;
    };

public :
    class Audio : public MultiLangElement {
    public  :
        /**MPEG*/
        static const int FORMAT_MPEG      = 0;
        static const int FORMAT_PCM_S16LE = 1;
        /**AAC*/
        static const int FORMAT_AAC       = 2;
        /**AC3*/
        static const int FORMAT_AC3       = 3;
        /**ALAW*/
        static const int FORMAT_ALAW      = 4;
        /**MULAW*/
        static const int FORMAT_MULAW     = 5;
        /**DTS*/
        static const int FORMAT_DTS       = 6;

        static const int FORMAT_PCM_S16BE = 7;
        /**FLAC*/
        static const int FORMAT_FLAC      = 8;
        /**COOK*/
        static const int FORMAT_COOK      = 9;
        /**PCM 8U*/
        static const int FORMAT_PCM_U8    = 10;
        /**ADPCM*/
        static const int FORMAT_ADPCM     = 11;
        /**AMR*/
        static const int FORMAT_AMR       = 12;
        /**RAAC*/
        static const int FORMAT_RAAC      = 13;
        /**WMA*/
        static const int FORMAT_WMA       = 14;
        /**WMA Pro*/
        static const int FORMAT_WMAPRO    = 15;

        static const int FORMAT_PCM_BLURAY = 16;
        /**ALAC*/
        static const int FORMAT_ALAC      = 17;
        /**Vorbis*/
        static const int FORMAT_VORBIS    = 18;

        static const int FORMAT_AAC_LATM  = 19;
        /**APE*/
        static const int FORMAT_APE       = 20;


        Audio(int pid, String8 lang, int fmt): MultiLangElement(pid, lang)
        {
            this->mformat = fmt;
        }

        int getFormat()
        {
            return mformat;
        }
    private :
        int mformat;
    };

public :
    class Subtitle : public MultiLangElement {
    public :
        /**DVB subtitle*/
        static const int TYPE_DVB_SUBTITLE = 1;

        static const int TYPE_DTV_TELETEXT = 2;

        static const int TYPE_ATV_TELETEXT = 3;

        static const int TYPE_DTV_CC = 4;

        static const int TYPE_ATV_CC = 5;



        Subtitle(int pid, String8 lang, int type, int num1, int num2): MultiLangElement(pid, lang)
        {
            compositionPage = 0;
            ancillaryPage = 0;
            magazineNo = 0;
            pageNo = 0;
            this->type = type;
            if (type == TYPE_DVB_SUBTITLE) {
                compositionPage = num1;
                ancillaryPage   = num2;
            } else if (type == TYPE_DTV_TELETEXT) {
                magazineNo = num1;
                pageNo = num2;
            }
        }


        int getType()
        {
            return type;
        }


        int getCompositionPageID()
        {
            return compositionPage;
        }


        int getAncillaryPageID()
        {
            return ancillaryPage;
        }


        int getMagazineNumber()
        {
            return magazineNo;
        }

        int getPageNumber()
        {
            return pageNo;
        }

    private :
        int compositionPage;
        int ancillaryPage;
        int magazineNo;
        int pageNo;
        int type;
    };


public :
    class Teletext : public MultiLangElement {
    public:
        Teletext(int pid, String8 lang, int mag, int page): MultiLangElement(pid, lang)
        {
            magazineNo = mag;
            pageNo = page;
        }

        int getMagazineNumber()
        {
            return magazineNo;
        }

        int getPageNumber()
        {
            return pageNo;
        }

    private :
        int magazineNo;
        int pageNo;
    };


public:

    static const int MINOR_CHECK_NONE         = 0;

    static const int MINOR_CHECK_UP           = 1;

    static const int MINOR_CHECK_DOWN         = 2;

    static const int MINOR_CHECK_NEAREST_UP   = 3;

    static const int MINOR_CHECK_NEAREST_DOWN = 4;

    int getNumber()
    {
        return major;
    }

    int getMajor()
    {
        return major;
    }

    int getMinor()
    {
        return minor;
    }

    bool isATSCMode()
    {
        return atscMode;
    }

    int getMinorCheck()
    {
        return minorCheck;
    }

private:
    int major;
    int minor;
    int minorCheck;
    bool atscMode;


public:
    CTvProgram(CTvDatabase::Cursor &c);
    CTvProgram(int channelID, int type, int num, int skipFlag);
    CTvProgram(int channelID, int type, int major, int minor, int skipFlag);
    ~CTvProgram();
    CTvProgram(int channelID, int type);

    CTvProgram();
    CTvProgram(const CTvProgram&);
    CTvProgram& operator= (const CTvProgram& cTvProgram);


    int getCurrentAudio(String8 defaultLang);
    Video *getVideo()
    {
        return mpVideo;
    }
    Audio *getAudio(int id)
    {
        if (mvAudios.size() <= 0) return NULL;
        return mvAudios[id];
    }

    int getAudioTrackSize()
    {
        return mvAudios.size();
    }
    static int selectByID(int id, CTvProgram &p);
    static CTvProgram selectByNumber(int num, int type);
    int selectByNumber(int type, int major, int minor, CTvProgram &prog, int minor_check = MINOR_CHECK_NONE);
    static int selectByNumber(int type, int num, CTvProgram &prog);
    static int selectByChannel(int channelID, int type, Vector<sp<CTvProgram> > &out);
    static int selectAll(bool no_skip, Vector<sp<CTvProgram> > &out);
    static int selectByType(int type, int skip, Vector<sp<CTvProgram> > &out);
    static int selectByChanID(int type, int skip, Vector<sp<CTvProgram> > &out);
    static Vector<CTvProgram> selectByChannel(int channelID);
    static Vector<CTvProgram>  selectByName(int name);
    void tvProgramDelByChannelID(int channelID);
    int getID()
    {
        return id;
    };
    int getSrc()
    {
        return src;
    };
    int getProgType()
    {
        return type;
    };
    int getChanOrderNum()
    {
        return chanOrderNum;
    };
    int getChanVolume()
    {
        return volume;
    };
    int getSourceId()
    {
        return sourceID;
    };
    int getServiceId()
    {
        return dvbServiceID;
    };
    int getPcrId()
    {
        return pcrID;
    };

    int getProgSkipFlag();
    int getSubtitleIndex(int progId);
    int setSubtitleIndex(int progId, int index);
    void setCurrAudioTrackIndex(int programId, int audioIndex);
    int getCurrAudioTrackIndex();

    String8 getName();
    void getCurrentSubtitle();
    void getCurrentTeletext();
    int getChannel(CTvChannel &c);
    int upDateChannel(CTvChannel &c, int std, int freq, int fineFreq);
    int updateVolComp(int progID, int volValue);
    void updateProgramName(int progId, String8 strName);
    void setSkipFlag(int progId, bool bSkipFlag);
    void setFavoriteFlag(int progId, bool bFavor);
    int getFavoriteFlag()
    {
        return favorite;
    };
    void deleteProgram(int progId);
    static int CleanAllProgramBySrvType(int srvType);
    void setLockFlag(int progId, bool bLockFlag);
    bool getLockFlag();
    void swapChanOrder(int ProgId1, int chanOrderNum1, int ProgId2, int chanOrderNum2);
    int getAudioChannel();
    static int updateAudioChannel(int progId, int ch);
    static int deleteChannelsProgram(CTvChannel &c);
    Vector<Subtitle *> getSubtitles()
    {
        return mvSubtitles;
    }
private:
    friend class LightRefBase<CTvProgram>;
    int CreateFromCursor(CTvDatabase::Cursor &c);
    int selectProgramInChannelByNumber(int channelID, int num, CTvDatabase::Cursor &c);
    int selectProgramInChannelByNumber(int channelID, int major, int minor, CTvDatabase::Cursor &c);
    CTvChannel channel;
    int id;
    int dvbServiceID;
    int type;
    String8 name;
    int channelID;
    int skip;
    int favorite;
    int volume;
    int sourceID;
    int pmtPID;
    int src;
    int audioTrack;
    int chanOrderNum;
    int currAudTrackIndex;
    bool lock;
    bool scrambled;
    Video *mpVideo;
    int pcrID;
    Vector<Audio *> mvAudios;
    Vector<Subtitle *> mvSubtitles;
    Vector<Teletext *> mvTeletexts;

};

#endif  //_CTVPROGRAM_H
