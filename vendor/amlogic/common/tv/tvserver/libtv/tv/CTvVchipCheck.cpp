/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CTvVchipCheck"

#include "CTvVchipCheck.h"
CTvVchipCheck:: CTvVchipCheck()
{
    m_request_pause_detect = false;
    mDetectState = 0;
}

CTvVchipCheck:: ~CTvVchipCheck()
{
}

bool CTvVchipCheck::CheckProgramBlock(int id)
{
    bool lock = false;
    CTvProgram prog;
    CTvEvent ev;
    int ret = 0;

    ret = CTvProgram::selectByID(id, prog);
    if (ret != 0) return false;

    int type = prog.getProgType();

    if (type == CTvProgram::TYPE_ATV) {
        ret = ev.getATVProgEvent(prog.getSrc(), prog.getID(), ev);
    } else {
        //long epgtime = mDmTime.getTime();
        //ret = ev.getProgPresentEvent(prog.getSrc(),prog.getSourceId(), epgtime, ev);
    }
    if (ret == 0) {
        if (prog.isATSCMode()) {
            // ATSC V-Chip
            Vector<CTvDimension::VChipRating *> definedRatings = ev.getVChipRatings();
            for (int i = 0; i < (int)definedRatings.size(); i++) {
                CTvDimension dm;
                if (dm.isBlocked(dm, definedRatings[i])) {
                    lock = true;
                    {
                        /*CurvchipDimension = dm.getName();
                        CurvchipAbbrev = dm.getAbbrev(definedRatings[i]->getValue());
                        CurvchipText= dm.getText(definedRatings[i]->getValue());
                        LOGD("%s, %d Dimension:%s, Abbrev: %s, idx:%d","TV",__LINE__,CurvchipDimension.string(),
                            CurvchipAbbrev.string(),definedRatings[i]->getValue());*/
                    }
                    break;
                }
            }
        }
    } else {
        LOGD("Present event of playing program not received yet, will unblock this program.");
    }

    return lock;
}

void *CTvVchipCheck::VchipCheckingThread (void *arg __unused)
{
    /*CTv *pt = static_cast<CTv *> ( arg );

    if ( !pt->IsVchipEnable() ) {
        return NULL;
    }

    while ( pt->mvchip_running ) {
        bool lock = 0;
        String8 curdm;
        String8 curabbrev;
        tvin_info_t siginfo = pt->Tv_GetCurrentSignalInfo();

        //if ( TVIN_SIG_STATUS_STABLE == siginfo.status ) {
            lock = pt->mTvVchip.CheckProgramBlock ( pt->getDTVProgramID() );
            curdm = pt->mTvVchip.getCurdimension();
            curabbrev = pt->mTvVchip.getCurAbbr();

            if ( ( lock != pt->mlastlockstatus ) || ( pt->mlastdm != curdm ) || ( pt->mlastabbrev != curabbrev ) ) {
                pt->mlastlockstatus = lock;
                pt->mlastdm = curdm;
                pt->mlastabbrev = curabbrev;
                BlockEvent evt;

                if ( lock ) {
                    evt.programBlockType = 0;
                    evt.block_status = 1;
                    evt.vchipDimension = curdm;
                    evt.vchipAbbrev = curdm;
                    LOGD ( "%s, %d block the program by type %s, %s", "TV", __LINE__, curdm.string(), curabbrev.string() );
                } else {
                    LOGD ( "unblock the program" );
                    evt.programBlockType = 0;
                    evt.block_status = 0;
                }

                pt->sendTvEvent ( evt );
                pt->Programblock ( lock );
            }

            usleep ( 1000 * 1000 );
        //} else {
            //usleep ( 500 * 1000 );
        //}
    }*/

    return NULL;
}

int CTvVchipCheck::stopVChipCheck()
{
    AutoMutex _l( mLock );
    LOGD ( "stopVChipCheck() and exit thread" );
    requestExit();
    return 0;
}

int CTvVchipCheck::pauseVChipCheck()
{
    AutoMutex _l( mLock );
    LOGD ( "pauseVChipCheck() set request pause flag, when flag true, thread loop go pause on condition" );
    m_request_pause_detect = true;
    return 0;
}

int CTvVchipCheck::requestAndWaitPauseVChipCheck()
{
    AutoMutex _l( mLock );
    LOGD ( "requestAndWaitPauseVChipCheck(),first set pause flag to true, and wait when loop run to pause code segment" );
    m_request_pause_detect = true;

    if ( mDetectState == STATE_RUNNING ) {
        mRequestPauseCondition.wait ( mLock );
    }

    return 0;
}

int CTvVchipCheck::resumeVChipCheck()
{
    AutoMutex _l( mLock );
    LOGD ( "resumeVChipCheck() first set flag false, and signal to paused condition, let run loop" );
    m_request_pause_detect = false;
    mDetectPauseCondition.signal();
    return 0;
}

bool  CTvVchipCheck::threadLoop()
{
    while ( !exitPending() ) { //requietexit() or requietexitWait() not call
        while ( m_request_pause_detect ) {
            mRequestPauseCondition.broadcast();
            mLock.lock();
            mDetectState = STATE_PAUSE;
            mDetectPauseCondition.wait ( mLock ); //first unlock,when return,lock again,so need,call unlock
            mDetectState = STATE_RUNNING;
            mLock.unlock();
        }
        //loop codes

        if ( !m_request_pause_detect ) { //not request pause, sleep 1s which loop
            usleep ( 1000 * 1000 );
        }
    }
    //exit
    mDetectState = STATE_STOPED;
    //return true, run again, return false,not run.
    return false;
}
