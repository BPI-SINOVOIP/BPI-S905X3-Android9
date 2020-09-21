/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */


#ifndef COVERSCANDB_H_
#define COVERSCANDB_H_

#include <cutils/properties.h>
#include <CSqlite.h>
#include "PQType.h"
#include "CPQLog.h"

#ifdef TV_DEBUG_PQ_ENABLE
#define DEBUG_FLAG 1
#else
#define DEBUG_FLAG 0
#endif

#ifdef PROP_DEBUG_PQ
#undef PROP_DEBUG_PQ
#endif
#define PROP_DEBUG_PQ "tv.debug.pq.enable"

#ifdef getSqlParams
#undef getSqlParams
#endif
#define getSqlParams(func, buffer, args...) \
    do{\
        sprintf(buffer, ##args);\
        if (DEBUG_FLAG) {\
            char value[PROPERTY_VALUE_MAX];\
            memset(value, '\0', PROPERTY_VALUE_MAX);\
            property_get(PROP_DEBUG_PQ, value, "0");\
            if(!strcmp(value, "1")){\
                SYS_LOGD("getSqlParams for %s\n", func);\
                SYS_LOGD("%s = %s\n",#buffer, buffer);\
            }\
        }\
    }while(0)


class COverScandb: public CSqlite {
public:
    COverScandb();
    ~COverScandb();
    int openOverScanDB(const char *);
    bool GetOverScanDbVersion(String8& ToolVersion, String8& ProjectVersion, String8& GenerateTime);
    int PQ_GetOverscanParams(source_input_param_t source_input_param, vpp_display_mode_t dmode, tvin_cutwin_t *cutwin_t);
    int PQ_SetOverscanParams(source_input_param_t source_input_param, tvin_cutwin_t cutwin_t);
    int PQ_ResetAllOverscanParams(void);
    int PQ_GetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    int PQ_SetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    int PQ_SetPQModeParamsByName(const char *name, tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params);
    int PQ_ResetAllPQModeParams(void);
};
#endif
