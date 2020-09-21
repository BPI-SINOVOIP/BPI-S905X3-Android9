/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _BANDWIDTH_CONTROLLER_H
#define _BANDWIDTH_CONTROLLER_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <utils/RWLock.h>

#include "NetdConstants.h"

class BandwidthController {
public:
    android::RWLock lock;

    BandwidthController();

    int setupIptablesHooks();
    static bool getBpfStatsStatus();

    int enableBandwidthControl(bool force);
    int disableBandwidthControl();
    int enableDataSaver(bool enable);

    int setInterfaceSharedQuota(const std::string& iface, int64_t bytes);
    int getInterfaceSharedQuota(int64_t *bytes);
    int removeInterfaceSharedQuota(const std::string& iface);

    int setInterfaceQuota(const std::string& iface, int64_t bytes);
    int getInterfaceQuota(const std::string& iface, int64_t* bytes);
    int removeInterfaceQuota(const std::string& iface);

    int addNaughtyApps(int numUids, char *appUids[]);
    int removeNaughtyApps(int numUids, char *appUids[]);
    int addNiceApps(int numUids, char *appUids[]);
    int removeNiceApps(int numUids, char *appUids[]);

    int setGlobalAlert(int64_t bytes);
    int removeGlobalAlert();
    int setGlobalAlertInForwardChain();
    int removeGlobalAlertInForwardChain();

    int setSharedAlert(int64_t bytes);
    int removeSharedAlert();

    int setInterfaceAlert(const std::string& iface, int64_t bytes);
    int removeInterfaceAlert(const std::string& iface);

    static const char LOCAL_INPUT[];
    static const char LOCAL_FORWARD[];
    static const char LOCAL_OUTPUT[];
    static const char LOCAL_RAW_PREROUTING[];
    static const char LOCAL_MANGLE_POSTROUTING[];

  private:
    struct QuotaInfo {
        int64_t quota;
        int64_t alert;
    };

    enum IptIpVer { IptIpV4, IptIpV6 };
    enum IptFullOp { IptFullOpInsert, IptFullOpDelete, IptFullOpAppend };
    enum IptJumpOp { IptJumpReject, IptJumpReturn, IptJumpNoAdd };
    enum IptOp { IptOpInsert, IptOpDelete };
    enum QuotaType { QuotaUnique, QuotaShared };
    enum RunCmdErrHandling { RunCmdFailureBad, RunCmdFailureOk };
#if LOG_NDEBUG
    enum IptFailureLog { IptFailShow, IptFailHide };
#else
    enum IptFailureLog { IptFailShow, IptFailHide = IptFailShow };
#endif

    std::string makeDataSaverCommand(IptablesTarget target, bool enable);

    int manipulateSpecialApps(const std::vector<std::string>& appStrUids, const std::string& chain,
                              IptJumpOp jumpHandling, IptOp appOp);

    int runIptablesAlertCmd(IptOp op, const std::string& alertName, int64_t bytes);
    int runIptablesAlertFwdCmd(IptOp op, const std::string& alertName, int64_t bytes);

    int updateQuota(const std::string& alertName, int64_t bytes);

    int setCostlyAlert(const std::string& costName, int64_t bytes, int64_t* alertBytes);
    int removeCostlyAlert(const std::string& costName, int64_t* alertBytes);

    /*
     * Attempt to find the bw_costly_* tables that need flushing,
     * and flush them.
     * If doClean then remove the tables also.
     * Deals with both ip4 and ip6 tables.
     */
    void flushExistingCostlyTables(bool doClean);
    static void parseAndFlushCostlyTables(const std::string& ruleList, bool doRemove);

    /*
     * Attempt to flush our tables.
     * If doClean then remove them also.
     * Deals with both ip4 and ip6 tables.
     */
    void flushCleanTables(bool doClean);

    // For testing.
    friend class BandwidthControllerTest;
    static int (*execFunction)(int, char **, int *, bool, bool);
    static FILE *(*popenFunction)(const char *, const char *);
    static int (*iptablesRestoreFunction)(IptablesTarget, const std::string&, std::string *);

    static const char *opToString(IptOp op);
    static const char *jumpToString(IptJumpOp jumpHandling);

    int64_t mSharedQuotaBytes = 0;
    int64_t mSharedAlertBytes = 0;
    int64_t mGlobalAlertBytes = 0;
    /*
     * This tracks the number of tethers setup.
     * The FORWARD chain is updated in the following cases:
     *  - The 1st time a globalAlert is setup and there are tethers setup.
     *  - Anytime a globalAlert is removed and there are tethers setup.
     *  - The 1st tether is setup and there is a globalAlert active.
     *  - The last tether is removed and there is a globalAlert active.
     */
    int mGlobalAlertTetherCount = 0;

    std::map<std::string, QuotaInfo> mQuotaIfaces;
    std::set<std::string> mSharedQuotaIfaces;
};

#endif
