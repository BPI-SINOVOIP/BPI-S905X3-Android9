/*
 *
 * honggfuzz - reporting
 * -----------------------------------------
 *
 * Author: Robert Swiecki <swiecki@google.com>
 *
 * Copyright 2010-2015 by Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 */

#include "report.h"

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "libcommon/common.h"
#include "libcommon/log.h"
#include "libcommon/util.h"

static int reportFD = -1;

#if defined(_HF_ARCH_LINUX)
static void report_printdynFileMethod(run_t* run) {
    dprintf(reportFD, " dynFileMethod: ");
    if (run->global->dynFileMethod == 0)
        dprintf(reportFD, "NONE\n");
    else {
        if (run->global->dynFileMethod & _HF_DYNFILE_INSTR_COUNT) dprintf(reportFD, "INSTR_COUNT ");
        if (run->global->dynFileMethod & _HF_DYNFILE_BRANCH_COUNT)
            dprintf(reportFD, "BRANCH_COUNT ");
        if (run->global->dynFileMethod & _HF_DYNFILE_BTS_EDGE) dprintf(reportFD, "BTS_EDGE_COUNT ");
        if (run->global->dynFileMethod & _HF_DYNFILE_IPT_BLOCK)
            dprintf(reportFD, "IPT_BLOCK_COUNT ");

        dprintf(reportFD, "\n");
    }
}
#endif

static void report_printTargetCmd(run_t* run) {
    dprintf(reportFD, " fuzzTarget   : ");
    for (int x = 0; run->global->exe.cmdline[x]; x++) {
        dprintf(reportFD, "%s ", run->global->exe.cmdline[x]);
    }
    dprintf(reportFD, "\n");
}

void report_Report(run_t* run) {
    if (run->report[0] == '\0') {
        return;
    }

    MX_SCOPED_LOCK(&run->global->report_mutex);

    if (reportFD == -1) {
        char reportFName[PATH_MAX];
        if (run->global->reportFile == NULL) {
            snprintf(reportFName, sizeof(reportFName), "%s/%s", run->global->io.workDir,
                _HF_REPORT_FILE);
        } else {
            snprintf(reportFName, sizeof(reportFName), "%s", run->global->reportFile);
        }

        reportFD = open(reportFName, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
        if (reportFD == -1) {
            PLOG_F("Couldn't open('%s') for writing", reportFName);
        }
    }

    char localtmstr[PATH_MAX];
    util_getLocalTime("%F.%H:%M:%S", localtmstr, sizeof(localtmstr), time(NULL));

    dprintf(reportFD,
        "=====================================================================\n"
        "TIME: %s\n"
        "=====================================================================\n"
        "FUZZER ARGS:\n"
        " mutationsPerRun : %u\n"
        " externalCmd     : %s\n"
        " fuzzStdin       : %s\n"
        " timeout         : %ld (sec)\n"
        " ignoreAddr      : %p\n"
        " ASLimit         : %" PRIu64
        " (MiB)\n"
        " RSSLimit        : %" PRIu64
        " (MiB)\n"
        " DATALimit       : %" PRIu64
        " (MiB)\n"
        " targetPid       : %d\n"
        " targetCmd       : %s\n"
        " wordlistFile    : %s\n",
        localtmstr, run->global->mutationsPerRun,
        run->global->exe.externalCommand == NULL ? "NULL" : run->global->exe.externalCommand,
        run->global->exe.fuzzStdin ? "TRUE" : "FALSE", run->global->timing.tmOut,
        run->global->linux.ignoreAddr, run->global->exe.asLimit, run->global->exe.rssLimit,
        run->global->exe.dataLimit, run->global->linux.pid, run->global->linux.pidCmd,
        run->global->dictionaryFile == NULL ? "NULL" : run->global->dictionaryFile);

#if defined(_HF_ARCH_LINUX)
    report_printdynFileMethod(run);
#endif

    report_printTargetCmd(run);

    dprintf(reportFD,
        "%s"
        "=====================================================================\n",
        run->report);
}
