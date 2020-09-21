/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "CPQdb"

#include "CPQdb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <utils/Log.h>
#include <utils/String8.h>

#define ID_FIELD            "TableID"
#define CM_LEVEL_NAME       "CMLevel"
#define LEVEL_NAME          "Level"
#define CVBS_NAME_ID        "TVOUT_CVBS"
#define TABLE_NAME_ID       "TableName"

CPQdb::CPQdb()
{
    memset(pq_bri_data, 0, sizeof(pq_bri_data));
    memset(pq_con_data, 0, sizeof(pq_con_data));
    memset(pq_sat_data, 0, sizeof(pq_sat_data));
    memset(pq_hue_data, 0, sizeof(pq_hue_data));
    memset(pq_sharpness0_reg_data, 0, sizeof(pq_sharpness0_reg_data));
    memset(pq_sharpness1_reg_data, 0, sizeof(pq_sharpness1_reg_data));
}

CPQdb::~CPQdb()
{
}

int CPQdb::openPqDB(const char *db_path)
{
    SYS_LOGD("openPqDB path = %s", db_path);
    int rval;

    if (access(db_path, 0) < 0) {
        SYS_LOGE("PQ_DB don't exist!\n");
        return -1;
    }

    closeDb();
    rval = openDb(db_path);
    if (rval == 0) {
        String8 PQ_ToolVersion, PQ_DBVersion, PQ_DBGenerateTime, val;
        if (PQ_GetPqVersion(PQ_ToolVersion, PQ_DBVersion, PQ_DBGenerateTime)) {
            val = PQ_ToolVersion + " " + PQ_DBVersion + " " + PQ_DBGenerateTime;
            SYS_LOGD("db-code match mask is %d\n", PQ_DB_CODE_MATCH_MASK);
            if (PQ_DBVersion.isEmpty()) {
                mDbMatchType = MATCH_TYPE_NO_DBVERSION;
            } else {
                unsigned int DbVersionSelectValue = atoi(PQ_DBVersion.c_str());
                if (DbVersionSelectValue == PQ_DB_CODE_MATCH_MASK) {
                    mDbMatchType = MATCH_TYPE_MATCH;
                } else if (DbVersionSelectValue > PQ_DB_CODE_MATCH_MASK) {
                    mDbMatchType = MATCH_TYPE_OLDCODE_NEWDB;
                } else{
                    mDbMatchType = MATCH_TYPE_NEWCODE_OLDDB;
                }
            }
            SYS_LOGD("db-code match type is %d.\n", mDbMatchType);
        } else {
            val = "Get PQ_DB Verion failure!!!";
        }
        SYS_LOGD("%s = %s\n", "PQ.db.version", val.string());
    }

    return rval;
}

int CPQdb::reopenDB(const char *db_path)
{
    int  rval = openDb(db_path);
    return rval;
}

int CPQdb::getRegValues(const char *table_name, am_regs_t *regs)
{
    CSqlite::Cursor c_reg_list;
    int rval = -1;
    int index_am_reg = 0;
    char sqlmaster[256];
    if (table_name == NULL || !strlen(table_name)) {
        SYS_LOGE("%s, table_name is null\n", __FUNCTION__);
        return rval;
    }

    getSqlParams(__FUNCTION__, sqlmaster, "select RegType, RegAddr, RegMask, RegValue from %s;", table_name);
    this->select(sqlmaster, c_reg_list);
    int count = c_reg_list.getCount();
    if (count < 0 ) {
        SYS_LOGE("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        return rval;
    } else if (count > REGS_MAX_NUMBER) {
        SYS_LOGE("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        regs->length = 0;
        return rval;
    }

    if (c_reg_list.moveToFirst()) {
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
            regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
            regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
            regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            index_am_reg++;
        } while (c_reg_list.moveToNext());
        regs->length = index_am_reg;
        rval = 0;
    } else {
        SYS_LOGE ("Don't have table in %s !\n", table_name);
        regs->length = 0;
    }

    return rval;
}

int CPQdb::getRegValuesByValue(const char *name, const char *f_name, const char *f2_name,
                                 const int val, const int val2, am_regs_t *regs)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256];
    int rval = -1;

    if ((strlen(f2_name) == 0) && (val2 == 0)) {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                     val);
    } else {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);
    }

    rval = this->select(sqlmaster, c_reg_list);
    int count = c_reg_list.getCount();

    if (count < 0 ) {
        SYS_LOGE("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        return -1;
    } else if (count > REGS_MAX_NUMBER) {
        SYS_LOGE("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        regs->length = 0;
        return -1;
    }

    int index_am_reg = 0;
    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
            regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
            regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
            regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            index_am_reg++;
        } while (c_reg_list.moveToNext());
        regs->length = index_am_reg;
    } else {
        regs->length = 0;
        rval = -1;
    }

    SYS_LOGD("%s, length = %d", __FUNCTION__, regs->length);
    return rval;
}

int CPQdb::getRegValuesByValue_long(const char *name, const char *f_name, const char *f2_name,
                                      const int val, const int val2, am_regs_t *regs, am_regs_t *regs_1)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256];
    int rval = -1;

    if ((strlen(f2_name) == 0) && (val2 == 0)) {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                     val);
    } else {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);
    }

    rval = this->select(sqlmaster, c_reg_list);

    int index_am_reg = 0;
    int count = c_reg_list.getCount();
    if (count < 0) {
        SYS_LOGD("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        regs_1->length = 0;
        return -1;
    } else if (count > 1024) {
        SYS_LOGD("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        regs->length = 0;
        regs_1->length = 0;
        return -1;
    }
    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            if (index_am_reg < 512) {
                regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
                regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
                regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
                regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            } else if (index_am_reg >= 512 && index_am_reg < 1024) {
                regs_1->am_reg[index_am_reg - 512].type = c_reg_list.getUInt(index_type);
                regs_1->am_reg[index_am_reg - 512].addr = c_reg_list.getUInt(index_addr);
                regs_1->am_reg[index_am_reg - 512].mask = c_reg_list.getUInt(index_mask);
                regs_1->am_reg[index_am_reg - 512].val = c_reg_list.getUInt(index_val);
            } else {
            }
            index_am_reg++;
        } while (c_reg_list.moveToNext());

        if (index_am_reg < 512) {
            regs->length = index_am_reg;
        } else if (index_am_reg >= 512 && index_am_reg < 1024) {
            regs->length = 512;
            regs_1->length = index_am_reg - 512;
        }
    } else {
        SYS_LOGE("%s, Select value error!\n", __FUNCTION__);
        regs->length = 0;
        regs_1->length = 0;
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetBlackExtensionParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralBlackBlueTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralBlackBlueTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValues(TableName.string(), regs);
    } else {
        SYS_LOGE("GeneralBlackBlueTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetSharpness0FixedParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralSharpness0FixedTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness0FixedTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValues(TableName.string(), regs);
    } else {
        SYS_LOGE("GeneralSharpness0FixedTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_SetSharpness0VariableParams(source_input_param_t source_input_param)
{
    int rval = -1;
    if (CheckHdrStatus("GeneralSharpness0VariableTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness0VariableTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = loadSharpnessData(TableName.string(), 0);
    } else {
        SYS_LOGE("%s: GeneralSharpness0VariableTable don't have this table!\n", __FUNCTION__);
    }

    return rval;
}

int CPQdb::PQ_GetSharpness1FixedParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralSharpness1FixedTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness1FixedTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValues(TableName.string(), regs);
    } else {
        SYS_LOGE("GeneralSharpness1FixedTable don't have table!!\n");
    }

    return rval;
}

int CPQdb::PQ_SetSharpness1VariableParams(source_input_param_t source_input_param)
{
    int rval = -1;
    if (CheckHdrStatus("GeneralSharpness1VariableTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralSharpness1VariableTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = loadSharpnessData(TableName.string(), 1);
    } else {
        SYS_LOGE("%s: GeneralSharpness1VariableTable don't have this table!\n", __FUNCTION__);
    }

    return rval;
}

int CPQdb::PQ_GetCM2Params(vpp_color_management2_t basemode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;

    if (CheckHdrStatus("GeneralCM2Table"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralCM2Table", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue(TableName.string(), CM_LEVEL_NAME, "", (int) basemode, 0, regs);
    } else {
        SYS_LOGE("GeneralCM2Table select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetXVYCCParams(vpp_xvycc_mode_t xvycc_mode, source_input_param_t source_input_param, am_regs_t *regs,
                               am_regs_t *regs_1)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralXVYCCTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getRegValuesByValue_long(TableName.string(), LEVEL_NAME, "", (int) xvycc_mode, 0, regs, regs_1);
    } else {
        SYS_LOGE("GeneralXVYCCTable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetDIParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralDITable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.string(), "", "", 0, 0, regs);
    } else {
        SYS_LOGE("GeneralDITable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetMCDIParams(vpp_mcdi_mode_t mcdi_mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralMCDITable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.string(), LEVEL_NAME, "", (int) mcdi_mode, 0, regs);
    } else {
        SYS_LOGE("GeneralMCDITable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetDeblockParams(vpp_deblock_mode_t deb_mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralDeblockTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.string(), LEVEL_NAME, "", (int) deb_mode, 0, regs);
    } else {
        SYS_LOGE("GeneralDeblockTable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetNR2Params(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param, am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralNR2Table", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.string(), LEVEL_NAME, "", (int) nr_mode, 0, regs);
    } else {
        SYS_LOGE("GeneralNR2Table select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_GetDemoSquitoParams(source_input_param_t source_input_param,  am_regs_t *regs)
{
    int rval = -1;
    String8 TableName = GetTableName("GeneralDemosquitoTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        rval = getDIRegValuesByValue(TableName.string(), "", "", 0, 0, regs);
    } else {
        SYS_LOGE("GeneralDemosquitoTable select error!!\n");
    }

    return rval;

}

int CPQdb::getDIRegValuesByValue(const char *name, const char *f_name, const char *f2_name,
                                                   const int val, const int val2, am_regs_t *regs)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256];
    int rval = -1;

    if ((strlen(f2_name) == 0) && (val2 == 0)) {
        if ((strlen(f_name) == 0) && (val == 0)) {
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegType, RegAddr, RegMask, RegValue from %s ;", name);
        } else {
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                         val);
        }
    } else {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);
    }

    rval = this->select(sqlmaster, c_reg_list);
    int count = c_reg_list.getCount();
    if (count < 0) {
        SYS_LOGE("%s, select value error!\n", __FUNCTION__);
        return -1;
    } else if (count > REGS_MAX_NUMBER) {
        SYS_LOGE("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        return -1;
    }

    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;
        int index_addr = 1;
        int index_mask = 2;
        int index_val = 3;
        do {
            regs->am_reg[regs->length].type = c_reg_list.getUInt(index_type);
            regs->am_reg[regs->length].addr = c_reg_list.getUInt(index_addr);
            regs->am_reg[regs->length].mask = c_reg_list.getUInt(index_mask);
            regs->am_reg[regs->length].val = c_reg_list.getUInt(index_val);
            regs->length++;
        } while (c_reg_list.moveToNext());
    } else {
        SYS_LOGE("%s, select value error!\n", __FUNCTION__);
        rval = -1;
    }

    SYS_LOGD("%s, length = %d", __FUNCTION__, regs->length);
    return rval;
}

int CPQdb::PQ_GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode, source_input_param_t source_input_param,
                                                   tcon_rgb_ogo_t *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];

    int rval = -1;
    //default
    params->en = 1;
    params->r_pre_offset = 0;
    params->g_pre_offset = 0;
    params->b_pre_offset = 0;
    params->r_gain = 1024;
    params->g_gain = 1024;
    params->b_gain = 1024;
    params->r_post_offset = 0;
    params->g_post_offset = 0;
    params->b_post_offset = 0;

    String8 TableName = GetTableName("GeneralWhiteBalanceTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "select Enable, R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset  from %s where "
            "Level = %d and def = 0;", TableName.string(), (int) Tempmode);

        rval = this->select(sqlmaster, c);

        if (c.moveToFirst()) {
            params->en = c.getInt(0);
            params->r_pre_offset = c.getInt(1);
            params->g_pre_offset = c.getInt(2);
            params->b_pre_offset = c.getInt(3);
            params->r_gain = c.getInt(4);
            params->g_gain = c.getInt(5);
            params->b_gain = c.getInt(6);
            params->r_post_offset = c.getInt(7);
            params->g_post_offset = c.getInt(8);
            params->b_post_offset = c.getInt(9);
        }
    } else {
        SYS_LOGE("GeneralWhiteBalanceTable select error!!\n");
    }

    return rval;
}

int CPQdb::PQ_SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode,source_input_param_t source_input_param,
                                                   tcon_rgb_ogo_t params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    char sql[512];

    int rval = -1;
    String8 TableName = GetTableName("GeneralWhiteBalanceTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(
            __FUNCTION__,
            sql,
            "update %s set Enable = %d, "
            "R_Pre_Offset = %d, G_Pre_Offset = %d, B_Pre_Offset = %d, R_Gain = %d, G_Gain = %d, B_Gain = %d, "
            "R_Post_Offset = %d, G_Post_Offset = %d, B_Post_Offset = %d  where Level = %d and def = 0;",
            TableName.string(), params.en, params.r_pre_offset,
            params.g_pre_offset, params.b_pre_offset, params.r_gain, params.g_gain,
            params.b_gain, params.r_post_offset, params.g_post_offset, params.b_post_offset,
            Tempmode);

        if (this->exeSql(sql)) {
            rval = 0;
        } else {
            SYS_LOGE("%s, update error!\n", __FUNCTION__);
            rval = -1;
        }
    } else {
        SYS_LOGD("%s, GeneralWhiteBalanceTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_ResetAllColorTemperatureParams(void)
{
    CSqlite::Cursor c;
    char sqlmaster[512];

    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select distinct TableName from GeneralWhiteBalanceTable ;");

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;
        do { //delete
            getSqlParams(
                __FUNCTION__,
                sqlmaster,
                "delete from %s where def = 0;"
                "insert into %s( Level , Enable , R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset, def ) "
                "select Level, Enable, R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset, 0 from %s where def = 1;",
                c.getString(index_TableName).string(), c.getString(index_TableName).string(),
                c.getString(index_TableName).string());
            if (this->exeSql(sqlmaster)) {
                rval = 0;
            } else {
                SYS_LOGE("%s, Delete values error!\n", __FUNCTION__);
                rval = -1;
            }
        } while (c.moveToNext());
    } else {
        SYS_LOGE("%s, GeneralWhiteBalanceTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetDNLPParams(source_input_param_t source_input_param, Dynamic_contrst_status_t mode, ve_dnlp_curve_param_t *newParams)
{
    CSqlite::Cursor c;
    CSqlite::Cursor c1;
    char sqlmaster[256];
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    unsigned int index = 0;
    int rval = -1;

    memset(newParams, 0, sizeof(ve_dnlp_curve_param_s));

    if (CheckHdrStatus("GeneralDNLPTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralDNLPTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        { // for param
            index = 0;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum < 1000 and "
                        "level = %d;",
                        TableName.string(), mode);

            rval = this->select(sqlmaster, c1);
            if (c1.moveToFirst()) {
                index = 0;
                do {
                    newParams->param[index] = c1.getInt(0);
                    index++;
                    if (index >= sizeof(newParams->param)/sizeof(unsigned int)) {
                        break;
                    }
                } while (c1.moveToNext());
            }
        }
        { // for ve_dnlp_scurv_low
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level =  %d;",
                        TableName.string(), scurv_low, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_dnlp_scurv_low is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_low[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_low)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
        { // for scurv_mid1
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), scurv_mid1, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_dnlp_scurv_mid1 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_mid1[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_mid1)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for scurv_mid2
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), scurv_mid2, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_dnlp_scurv_mid2 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_mid2[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_mid2)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for scurv_hgh1
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), scurv_hgh1, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_dnlp_scurv_hgh1 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_hgh1[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_hgh1)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for scurv_hgh2
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), scurv_hgh2, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_dnlp_scurv_hgh2 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_dnlp_scurv_hgh2[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_dnlp_scurv_hgh2)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for curv_var_lut49
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), curv_var_lut49, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_gain_var_lut49 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_gain_var_lut49[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_gain_var_lut49)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for curv_wext_gain
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), curv_wext_gain, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_wext_gain is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_wext_gain[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_wext_gain)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for adp_thrd
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), adp_thrd, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_adp_thrd is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_adp_thrd[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_adp_thrd)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_blk_boost_12
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), reg_blk_boost_12, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_reg_blk_boost_12 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_blk_boost_12[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_blk_boost_12)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_adp_ofset_20
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), reg_adp_ofset_20, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_reg_adp_ofset_20 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_adp_ofset_20[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_adp_ofset_20)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_mono_protect
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), reg_mono_protect, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_reg_mono_protect is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_mono_protect[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_mono_protect)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { // for reg_trend_wht_expand_lut8
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), reg_trend_wht_expand_lut8, mode);

            rval = this->select(sqlmaster, c1);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c1.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s - ve_reg_trend_wht_expand_lut8 is %s+++++++++++++++++", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                newParams->ve_reg_trend_wht_expand_lut8[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(newParams->ve_reg_trend_wht_expand_lut8)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
    } else {
        SYS_LOGE("%s, GeneralDNLPTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_GetLocalContrastNodeParams(source_input_param_t source_input_param, local_contrast_mode_t mode, ve_lc_curve_parm_t *Params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    char buf[512];
    char *buffer = NULL;
    char *aa = NULL;
    char *aa_save[100];
    unsigned int index = 0;
    int rval = -1;

    if (CheckHdrStatus("GeneralLocalContrastNodeTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralLocalContrastNodeTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        { //for param
            index = 0;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum > 256 and "
                        "level = %d;",
                        TableName.string(), mode);

            rval = this->select(sqlmaster, c);
            if (c.moveToFirst()) {
                index = 0;
                do {
                    Params->param[index] = c.getInt(0);
                    index++;
                    if (index >= sizeof(Params->param)/sizeof(unsigned int)) {
                        break;
                    }
                } while (c.moveToNext());
            }
        }
        { //for ve_lc_saturation
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level =  %d;",
                        TableName.string(), LC_SATURATION_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c.getString(index).string(), sizeof(buf));
            //SYS_LOGD("%s: ve_lc_saturation is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_saturation[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_saturation)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { //for ve_lc_yminval_lmt
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), LC_YMINVAL_LMT_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s: ve_lc_yminval_lmt is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_yminval_lmt[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_yminval_lmt)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { //for ve_lc_ypkbv_ymaxval_lmt
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), LC_YPKBV_YMAXVAL_LMT_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s: ve_lc_ypkbv_ymaxval_lmt is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_ypkbv_ymaxval_lmt[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_ypkbv_ymaxval_lmt)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
         { //for ve_lc_ypkbv_ratio
            index = 0;
            aa = NULL;
            getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                        "regnum = %d and "
                        "level = %d;",
                        TableName.string(), LC_YPKBV_RATIO_NUM, mode);

            rval = this->select(sqlmaster, c);
            memset(buf, 0, sizeof(buf));
            strncpy(buf, c.getString(index).string(), sizeof(buf));
            //SYS_LOGD ("%s: ve_lc_ypkbv_ratio is %s\n", __FUNCTION__, buf);
            buffer = buf;
            while ((aa_save[index] = strtok_r(buffer, " ", &aa)) != NULL) {
                Params->ve_lc_ypkbv_ratio[index] = atoi(aa_save[index]);
                index ++;
                if (index >= sizeof(Params->ve_lc_ypkbv_ratio)/sizeof(unsigned int)) {
                    break;
                }
                buffer = NULL;
            }
         }
    } else {
        SYS_LOGE("%s, GeneralLocalContrastNodeTable don't have this table!\n", __FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_GetLocalContrastRegParams(source_input_param_t source_input_param, local_contrast_mode_t mode, am_regs_t *regs)
{
    int ret = -1;

    if (CheckHdrStatus("GeneralLocalContrastRegTable"))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName("GeneralLocalContrastRegTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        ret = getRegValuesByValue(TableName.string(), LEVEL_NAME, "", (int) mode, 0, regs);
    } else {
        SYS_LOGE("GeneralLocalContrastRegTable select error!!\n");
    }

    return ret;
}

int CPQdb::PQ_SetDNLPGains(source_input_param_t source_input_param, Dynamic_contrst_status_t level, int final_gain)
{
    char sqlmaster[256];
    int final_gain_reg_num = 46;
    int rval = -1;

    String8 TableName = GetTableName("GeneralDNLPTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "update  %s set value = %d where "
                      "regnum = %d and "
                      "level = %d;",
                      TableName.string(), final_gain, final_gain_reg_num, level);
        rval = this->exeSql(sqlmaster);
    } else {
        SYS_LOGD("%s: GeneralDNLPTable don't have this table!\n", __FUNCTION__);
    }

    return rval;
}

int CPQdb::PQ_GetDNLPGains(source_input_param_t source_input_param, Dynamic_contrst_status_t level)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int final_gain = -1;
    int final_gain_reg_num = 46;

    String8 TableName = GetTableName("GeneralDNLPTable", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "select value from %s where "
                      "regnum = %d and "
                      "level = %d;",
                      TableName.string(), final_gain_reg_num, level);
        this->select(sqlmaster, c);

        if (c.moveToFirst()) {
            final_gain = c.getInt(0);
        }
    } else {
        SYS_LOGD("%s: GeneralDNLPTable don't have this table!\n", __FUNCTION__);
    }

    SYS_LOGD("PQ_GetDNLPGains, get final_gain: %d", final_gain);
    return final_gain;
}

int CPQdb::PQ_GetBEParams(source_input_param_t source_input_param, int addr, am_regs_t *regs)
{
     int rval = -1;

     String8 TableName = GetTableName("GeneralBlackBlueTable", source_input_param);
     if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
         rval = getRegValuesByValue(TableName.string(), "RegAddr", "", addr, 0, regs);
     } else {
         SYS_LOGE("%s: GeneralBlackBlueTable don't have this table!\n", __FUNCTION__);
     }

     return rval;
}

int CPQdb::PQ_SetBEParams(source_input_param_t source_input_param, int addr, unsigned int reg_val)
{
     int rval = -1;

     String8 TableName = GetTableName("GeneralBlackBlueTable", source_input_param);
     if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
         String8 cmd = String8("update ") + String8::format("%s", TableName.string()) +
                       String8(" set RegValue = ") + String8::format("%d", reg_val) +
                       String8(" where RegAddr = ") + String8::format("%d", addr);
         rval = this->exeSql(cmd.string());
     } else {
         SYS_LOGE("%s: GeneralBlackBlueTable don't have this table!\n", __FUNCTION__);
     }
     return rval;
}

int CPQdb::PQ_SetRGBCMYFcolor(source_input_param_t source_input_param, int data_Rank ,int val)
{
    int rval = -1;
    char sql[256];
    String8 tableName;
    tv_source_input_t source_input = source_input_param.source_input;

    if (source_input == SOURCE_TV) {
        tableName = String8("CMS_ATV");
    } else if ((source_input == SOURCE_AV1) || (source_input == SOURCE_AV2)) {
        tableName = String8("CMS_AV");
    } else if ((source_input >= SOURCE_HDMI1) && (source_input <= SOURCE_HDMI4)) {
        tableName = String8("CMS_HDMI");
    } else if (source_input >= SOURCE_DTV) {
        tableName = String8("CMS_DTV");
    } else if (source_input >= SOURCE_MPEG) {
        tableName = String8("CMS_MPEG_HD");
    }

    if (mHdrStatus) {
        tableName = String8("CMS_HDR");
    }

    getSqlParams(__FUNCTION__,sql,
                "update %s set Value = %d where Rank = %d;",
                tableName.string(), val, data_Rank);

    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s: SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetRGBCMYFcolor(source_input_param_t source_input_param, int data_Rank)
{
    int rval = -1;;
    CSqlite::Cursor c;
    char sqlmaster[256];
    String8 tableName;
    tv_source_input_t source_input = source_input_param.source_input;
    if (source_input == SOURCE_TV) {
        tableName = String8("CMS_ATV");
    } else if ((source_input == SOURCE_AV1) || (source_input == SOURCE_AV2)) {
        tableName = String8("CMS_AV");
    } else if ((source_input >= SOURCE_HDMI1) && (source_input <= SOURCE_HDMI4)) {
        tableName = String8("CMS_HDMI");
    } else if (source_input >= SOURCE_DTV) {
        tableName = String8("CMS_DTV");
    } else if (source_input >= SOURCE_MPEG) {
        tableName = String8("CMS_MPEG_HD");
    }

    if (mHdrStatus) {
        tableName = String8("CMS_HDR");
    }

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select Value from %s where "
                 "Rank = %d ", tableName.string(), data_Rank);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        rval = c.getInt(0);
    } else {
        SYS_LOGE("%s error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_SetNoLineAllBrightnessParams(tv_source_input_t source_input, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_BRIGHTNESS, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllBrightnessParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_BRIGHTNESS, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllBrightnessParams Error %d\n", rval);
    }
    return rval;

}

int CPQdb::PQ_GetBrightnessParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;
    GetNonlinearMapping(TVPQ_DATA_BRIGHTNESS, source_input_param.source_input, level, &val);
    *params = CaculateLevelParam(pq_bri_data, bri_nodes, val);
    return 0;

}

int CPQdb::PQ_SetBrightnessParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllContrastParams(tv_source_input_t source_input, int osd0, int osd25,
        int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_CONTRAST, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllContrastParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_CONTRAST, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllContrastParams Error %d\n", rval);
    }
    return rval;
}

int CPQdb::PQ_GetContrastParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_CONTRAST, source_input_param.source_input, level, &val);
    *params = CaculateLevelParam(pq_con_data, con_nodes, val);
    return 0;
}

int CPQdb::PQ_SetContrastParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllSaturationParams(tv_source_input_t source_input, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_SATURATION, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllSaturationParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_SATURATION, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllSaturationParams Error %d\n", rval);
    }
    return rval;
}
int CPQdb::PQ_GetSaturationParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_SATURATION, source_input_param.source_input, level, &val);
    *params = CaculateLevelParam(pq_sat_data, sat_nodes, val);
    return 0;
}

int CPQdb::PQ_SetSaturationParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllHueParams(tv_source_input_t source_input, int osd0, int osd25,
                                      int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_HUE, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllHueParams(tv_source_input_t source_input, int *osd0, int *osd25,
                                      int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_HUE, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllHueParams Error %d\n", rval);
    }
    return rval;
}
int CPQdb::PQ_GetHueParams(source_input_param_t source_input_param, int level, int *params)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_HUE, source_input_param.source_input, level, &val);
    *params = CaculateLevelParam(pq_hue_data, hue_nodes, val);
    return 0;
}

int CPQdb::PQ_SetHueParams(source_input_param_t source_input_param, int level __unused, int params __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllSharpnessParams(tv_source_input_t source_input, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllSharpnessParams(tv_source_input_t source_input, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_SHARPNESS, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllSharpnessParams Error %d\n", rval);
    }
    return rval;
}
int CPQdb::PQ_GetSharpness0Params(source_input_param_t source_input_param, int level, am_regs_t *regs)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_input_param.source_input, level, &val);
    *regs = CaculateLevelRegsParam(pq_sharpness0_reg_data, val, 0);
    return 0;
}

int CPQdb::PQ_GetSharpness1Params(source_input_param_t source_input_param, int level, am_regs_t *regs)
{
    int val = 0;

    GetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_input_param.source_input, level, &val);
    *regs = CaculateLevelRegsParam(pq_sharpness1_reg_data, val, 1);
    return 0;
}

int CPQdb::PQ_GetPLLParams(source_input_param_t source_input_param, am_regs_t *regs)
{
    int ret = -1;
    int i = 0;

    ret = getRegValuesByValue("ADC_Settings", "Port", "Format", source_input_param.source_input, source_input_param.sig_fmt, regs);
#ifdef  CPQDB_DEBUG
    if (ret == 0) {
        for (i = 0; i < regs->length; i++) {
            SYS_LOGD("%s, =================================================", "TV");
            SYS_LOGD("%s, regData.am_reg[%d].type = %d", "TV", i, regs->am_reg[i].type);
            SYS_LOGD("%s, regData.am_reg[%d].addr = %d", "TV", i, regs->am_reg[i].addr);
            SYS_LOGD("%s, regData.am_reg[%d].mask = %d", "TV", i, regs->am_reg[i].mask);
            SYS_LOGD("%s, regData.am_reg[%d].val  = %d", "TV", i, regs->am_reg[i].val);
        }
    }
#endif
    if (regs->am_reg[0].val == 0 && regs->am_reg[1].val == 0 && regs->am_reg[2].val == 0
            && regs->am_reg[3].val == 0) {
        SYS_LOGE("%s,db's value is all zeros, that's not OK!!!\n", "TV");
        return -1;
    }
    return ret;
}

int CPQdb::PQ_SetSharpnessParams(source_input_param_t source_input_param, int level __unused, am_regs_t regs __unused)
{
    return 0;
}

int CPQdb::PQ_SetNoLineAllVolumeParams(tv_source_input_t source_input, int osd0, int osd25,
        int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_VOLUME, source_input, osd0, osd25, osd50, osd75, osd100);
}

int CPQdb::PQ_GetNoLineAllVolumeParams(tv_source_input_t source_input, int *osd0, int *osd25,
        int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
     int rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_VOLUME, source_input, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        SYS_LOGE("PQ_GetNoLineAllSharpnessParams Error %d\n", rval);
    }
    return rval;
}

int CPQdb::PQ_ResetAllNoLineParams(void)
{
    int rval = 0;
    char sqlmaster[256];

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from NonlinearMapping; "
        "insert into NonlinearMapping(TVIN_PORT, Item_ID, Level, Value) select TVIN_PORT, Item_ID, Level, Value from NonlinearMapping_Default;");

    if (this->exeSql(sqlmaster)) {
        rval = 0;
    } else {
        rval = -1;
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
    }

    return rval;
}
int CPQdb::PQ_GetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param, int reg_addr)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int reg_val = -1;

    String8 TableName = GetTableName("GeneralNR2Table", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "select RegValue from %s where RegAddr = %d and Level = %d;",
                     TableName.string(), reg_addr, nr_mode);

        this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            reg_val = c.getInt(0);
        }
    } else {
        SYS_LOGD("%s: GeneralNR2Table don't have this table!\n", __FUNCTION__);
    }

    return reg_val;
}

int CPQdb::PQ_SetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode, source_input_param_t source_input_param , int reg_addr, int value)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int err = -1;

    String8 TableName = GetTableName("GeneralNR2Table", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "update %s set RegValue = %d where "
                 "RegAddr = %u and "
                 "Level = %d;",
                 TableName.string(), value, reg_addr, nr_mode);
        err = this->exeSql(sqlmaster);
    } else {
        SYS_LOGD("%s: GeneralNR2Table don't have this table!\n", __FUNCTION__);
    }

    return err;
}

int CPQdb::PQ_GetCVD2Param(source_input_param_t source_input_param, int reg_addr, int param_type, int reg_mask)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;
    int reg_val = -1;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI)
        source_input_param.sig_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;

    String8 TableName = GetTableName("GeneralCVD2Table", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "select RegValue from %s where "
                     "RegAddr = %u and "
                     "RegMask = %u;",
                     TableName.string(), reg_addr, reg_mask);

        rval = this->select(sqlmaster, c);
        if (rval < 0) {
            return rval;
        }

        if (c.moveToFirst()) {
            reg_val = c.getInt(0);
        }
    } else {
        SYS_LOGE("%s: GeneralCVD2Table don't have this table!\n", __FUNCTION__);
    }

    SYS_LOGD("%s, addr:%d, sourde_input:%d, sig_fmt: %d, get value is %u",
              __FUNCTION__, reg_addr, source_input_param.source_input, source_input_param.sig_fmt, reg_val);

    return reg_val;
}

int CPQdb::PQ_SetCVD2Param(source_input_param_t source_input_param, int reg_addr,
                                 int value, int param_type, int reg_mask)
{
    char sqlmaster[256];
    int rval = -1;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI)
        source_input_param.sig_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
    SYS_LOGD("%s:, param_type is %d, sig_fmt is %d, source is %d, addr is %d, reg_mask is %u\n",
              __FUNCTION__, param_type, source_input_param.sig_fmt, source_input_param.source_input, reg_addr, reg_mask);

    String8 TableName = GetTableName("GeneralCVD2Table", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(__FUNCTION__, sqlmaster, "update %s set RegValue = %d where "
                     "RegAddr = %u and "
                     "RegMask = %u;",
                     TableName.string(), value, reg_addr, reg_mask);
        rval = this->exeSql(sqlmaster);
    } else {
        SYS_LOGE("%s: GeneralCVD2Table don't have this table!\n", __FUNCTION__);
    }

    return rval;
}


int CPQdb::PQ_GetCVD2Params(source_input_param_t source_input_param, am_regs_t *regs)
{
    int ret = -1;
    String8 TableName = GetTableName("GeneralCVD2Table", source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        ret = getRegValues(TableName.string(), regs);
        if (regs->am_reg[0].val == 0 && regs->am_reg[1].val == 0 && regs->am_reg[2].val == 0
                && regs->am_reg[3].val == 0) {
            SYS_LOGE("%s: db's value is all zeros, that's not OK!!!\n", __FUNCTION__);
            return -1;
        }
    } else {
        SYS_LOGE("%s: GeneralCVD2Table don't have this table!\n", __FUNCTION__);
    }
    return ret;
}

int CPQdb::PQ_GetSharpnessCTIParams(source_input_param_t source_input_param, int reg_addr,
                                              int param_type, int reg_mask)
{
    CSqlite::Cursor c, c1;
    char sqlmaster[256];
    int err = -1;
    unsigned int reg_val = 0;
    char table_name[128];
    unsigned int sr0_reg_val = 0;
    unsigned int sr1_reg_val = 0;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI) {
        return PQ_GetCVD2Param(source_input_param, reg_addr, param_type, reg_mask);
    }

    getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);

    err = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        source_input_param.sig_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);
        getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);
        err = this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        do {
            SYS_LOGD("%s: get table is %s\n", __FUNCTION__, c.getString(0).string());
            SYS_LOGD("%s, addr is %d, reg mask is: %u\n", __FUNCTION__, reg_addr, reg_mask);
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegValue from %s where NodeNumber = %d and RegAddr = %d and RegMask = %u;",
                         c.getString(0).string(), 1, reg_addr, reg_mask);
            err = this->select(sqlmaster, c1);

            if (c1.moveToFirst()) {
                do {
                    if (param_type >= SR0_CTI_GAIN0 && param_type <= SR0_CTI_GAIN3
                        && !strncmp("Sharpness_0", c.getString(0).string(), 11)) {//SR0

                        sr0_reg_val = c1.getUInt(0);
                        SYS_LOGD("%s, addr is %d, reg_get sr0_reg_val: %u, %u\n", __FUNCTION__,
                                                         reg_addr, c1.getUInt(0), sr0_reg_val);
                        return sr0_reg_val;
                    } else if (param_type >= SR1_CTI_GAIN0 && param_type <= SR1_CTI_GAIN3
                        && !strncmp("Sharpness_1", c.getString(0).string(), 11)) {//SR1

                        sr1_reg_val = c1.getUInt(0);
                        SYS_LOGD("%s, addr is %d, get reg_val: %u, %u\n", __FUNCTION__,
                                                         reg_addr, c1.getUInt(0), sr1_reg_val);
                        return sr1_reg_val;
                    }
                } while (c1.moveToNext());
            }
        } while (c.moveToNext());
    }

    SYS_LOGE("%s, get value from db error", __FUNCTION__);
    return err;
}

int CPQdb::PQ_SetSharpnessCTIParams(source_input_param_t source_input_param, int reg_addr,
                                              int value, int param_type, int reg_mask)
{
    CSqlite::Cursor c, c1;
    char sqlmaster[256];
    int err = -1;

    if (param_type == CVD_YC_DELAY || param_type == DECODE_CTI) {
        return PQ_SetCVD2Param(source_input_param, reg_addr, value, param_type, reg_mask);
    }

    getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);

    err = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        source_input_param.sig_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);
        getSqlParams(__FUNCTION__,sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d and "
                 "%s = %d;",
                 "GeneralSharpnessG9Table", source_input_param.source_input,
                 source_input_param.sig_fmt, source_input_param.trans_fmt, ID_FIELD, 1);
        err = this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        do {
            SYS_LOGD("%s: get table is %s\n", __FUNCTION__, c.getString(0).string());
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegValue from %s where NodeNumber = %d and RegAddr = %d;",
                         c.getString(0).string(), 1, reg_addr);
            err = this->select(sqlmaster, c1);

            if (c1.moveToFirst()) {
                do {
                    if (param_type >= SR0_CTI_GAIN0 && param_type <= SR0_CTI_GAIN3
                        && !strncmp("Sharpness_0", c.getString(0).string(), 11)) {//SR0
                        SYS_LOGD("%s, addr is %d, last sr0_reg_val: %u, try to set %u\n",
                                     __FUNCTION__, reg_addr, c1.getUInt(0), value);
                        getSqlParams(__FUNCTION__, sqlmaster,
                                 "update %s set RegValue = %u where NodeNumber = %d and RegAddr = %d "
                                 "and RegMask = %u;",
                                 c.getString(0).string(), value, 1, reg_addr, reg_mask);
                        err |= this->exeSql(sqlmaster);
                        return err;
                    } else if (param_type >= SR1_CTI_GAIN0 && param_type <= SR1_CTI_GAIN3
                        && !strncmp("Sharpness_1", c.getString(0).string(), 11)) {//SR1
                        SYS_LOGD("%s, addr is %d, last sr1_reg_val: %u, try to set %u\n",
                                     __FUNCTION__, reg_addr, c1.getUInt(0), value);
                        getSqlParams(__FUNCTION__, sqlmaster,
                                 "update %s set RegValue = %u where NodeNumber = %d and RegAddr = %d "
                                 "and RegMask = %u;",
                                 c.getString(0).string(), value, 1, reg_addr, reg_mask);
                        err |= this->exeSql(sqlmaster);
                        return err;
                    }
                } while (c1.moveToNext());
            }
        } while (c.moveToNext());
    }

    return err;
}

const char *CPQdb::getSharpnessTableName(source_input_param_t source_input_param, int isHd)
{
    switch (source_input_param.source_input) {
        case SOURCE_TV: {
          if (!isHd) {
              return "Sharpness_0_ATV_Fixed";
          }
          return "Sharpness_1_ATV_Fixed";
        }
        case SOURCE_AV1:
        case SOURCE_AV2:{
          if (source_input_param.sig_fmt == 0x601 || source_input_param.sig_fmt == 0x602) {
              if (!isHd) {
                  return "Sharpness_0_AV_NTSC_Fixed";
              }
              return "Sharpness_1_AV_NTSC_Fixed";
          } else {
              if (!isHd) {
                  return "Sharpness_0_AV_PAL_Fixed";
              }
              return "Sharpness_1_AV_PAL_Fixed";
          }
        }
        case SOURCE_HDMI1:
        case SOURCE_HDMI2:
        case SOURCE_HDMI3:
        case SOURCE_HDMI4: {
          if (!isHd) {
              return "Sharpness_0_HDMI_SD_Fixed";
          }
          return "Sharpness_1_HDMI_HD_Fixed";
        }
        case SOURCE_MPEG: {
          if (!isHd) {
              return "Sharpness_0_MPEG_SD_Fixed";
          }
          return "Sharpness_1_MPEG_HD_Fixed";
        }
        case SOURCE_DTV: {
          if (!isHd) {
              return "Sharpness_0_DTV_SD_Fixed";
          }
          return "Sharpness_1_DTV_HD_Fixed";
        }
        default: {
          break;
        }
    }
    return NULL;
}


int CPQdb::PQ_GetSharpnessAdvancedParams(source_input_param_t source_input_param, int reg_addr, int isHd)
{
  int reg_val = 0;
  am_regs_t regs;
  memset(&regs, 0, sizeof(am_regs_t));

  reg_val = getSharpnessRegValues("GeneralCommonTable", source_input_param, &regs, reg_addr, isHd);
  SYS_LOGD("%s, get value is %u\n", __FUNCTION__, reg_val);

  return reg_val;
}

int CPQdb::getSharpnessRegValues(const char *table_name, source_input_param_t source_input_param,
                                         am_regs_t *regs, int reg_addr, int isHd)
{
    CSqlite::Cursor c_tablelist, c_reg_list;
    int index_am_reg = 0;
    char sqlmaster[256];
    tvin_sig_fmt_t signal = source_input_param.sig_fmt;

    if (table_name == NULL || !strlen(table_name)) {
        SYS_LOGE("%s, table_name is null\n", __FUNCTION__);
        return index_am_reg;
    }

    getSqlParams(__FUNCTION__, sqlmaster,
               "select TableName from %s where "
               "TVIN_PORT = %d and "
               "TVIN_SIG_FMT = %d and "
               "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
    this->select(sqlmaster, c_tablelist);

    if (c_tablelist.getCount() <= 0) {
        signal = TVIN_SIG_FMT_NULL;
        c_tablelist.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);

        getSqlParams(__FUNCTION__, sqlmaster,
                   "select TableName from %s where "
                   "TVIN_PORT = %d and "
                   "TVIN_SIG_FMT = %d and "
                   "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
        this->select(sqlmaster, c_tablelist);
    }

    if (c_tablelist.moveToFirst()) { //for table list
        do {
            getSqlParams(__FUNCTION__, sqlmaster,
                       "select RegType, RegAddr, RegMask, RegValue from %s;",
                       c_tablelist.getString(0).string());
            this->select(sqlmaster, c_reg_list);
            SYS_LOGD("%s, addr id 0x%x, [%d]table name is %s-----\n",
                    __FUNCTION__, reg_addr, index_am_reg, c_tablelist.getString(0).string());
            if (c_reg_list.moveToFirst()) { //reg list for each table
                int index_type = 0;
                int index_addr = 1;
                int index_mask = 2;
                int index_val = 3;
                do {
                    if ((strcmp(c_tablelist.getString(0).string(), getSharpnessTableName(source_input_param, isHd)) == 0)
                        && (unsigned long int)reg_addr == c_reg_list.getUInt(index_addr)) {
                        regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
                        regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
                        regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
                        regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);

                        SYS_LOGD("getSharpnessRegValues, addr is 0x%x, [%d]get value is %u\n",
                                 reg_addr, index_am_reg, regs->am_reg[index_am_reg].val);
                        return regs->am_reg[index_am_reg].val;
                    }

                    index_am_reg++;
                } while (c_reg_list.moveToNext());
            }
        } while (c_tablelist.moveToNext());
        regs->length = index_am_reg;
      }else {
          regs->length = 0;
          SYS_LOGE ("Don't have table in %s !\n",table_name);
    }
    return 0;
}

int CPQdb::PQ_SetSharpnessAdvancedParams(source_input_param_t source_input_param, int reg_addr, int value, int isHd)
{
  CSqlite::Cursor c_tablelist, c_reg_list;
  int err = -1;
  int index_am_reg = 0;
  char sqlmaster[256];
  const char *table_name = "GeneralCommonTable";
  tvin_sig_fmt_t signal = source_input_param.sig_fmt;

  getSqlParams(__FUNCTION__, sqlmaster,
               "select TableName from %s where "
               "TVIN_PORT = %d and "
               "TVIN_SIG_FMT = %d and "
               "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
  this->select(sqlmaster, c_tablelist);

  if (c_tablelist.getCount() <= 0) {
      signal = TVIN_SIG_FMT_NULL;
      c_tablelist.close();
      SYS_LOGD ("%s - Load default", __FUNCTION__);

      getSqlParams(__FUNCTION__, sqlmaster,
                   "select TableName from %s where "
                   "TVIN_PORT = %d and "
                   "TVIN_SIG_FMT = %d and "
                   "TVIN_TRANS_FMT = %d ;", table_name, source_input_param.source_input, signal, source_input_param.trans_fmt);
      err = this->select(sqlmaster, c_tablelist);
  }

  if (c_tablelist.moveToFirst()) { //for table list
      do {
          getSqlParams(__FUNCTION__, sqlmaster,
                       "select RegType, RegAddr, RegMask, RegValue from %s;",
                       c_tablelist.getString(0).string());
          err = this->select(sqlmaster, c_reg_list);
          SYS_LOGD("%s, addr id 0x%x, [%d]table name is %s-----\n",
                    __FUNCTION__, reg_addr, index_am_reg, c_tablelist.getString(0).string());
          if (c_reg_list.moveToFirst()) { //reg list for each table
              int index_type = 0;
              int index_addr = 1;
              int index_mask = 2;
              int index_val = 3;
              do {
                  if ((strcmp(c_tablelist.getString(0).string(), getSharpnessTableName(source_input_param, isHd)) == 0)
                      && (unsigned long int)reg_addr == c_reg_list.getUInt(index_addr)) {

                      SYS_LOGD("PQ_SetSharpnessAdvancedParams, addr is 0x%x, [%d]last reg value is %u, try to set %u\n",
                                 reg_addr, index_am_reg, c_reg_list.getUInt(index_val), value);
                      getSqlParams(__FUNCTION__,sqlmaster,
                                   "update %s set RegValue = %d where RegAddr = %u;",
                                   c_tablelist.getString(0).string(), value, reg_addr);
                      err = this->exeSql(sqlmaster);
                      return err;
                  }

                  index_am_reg++;
              } while (c_reg_list.moveToNext());
          }
      } while (c_tablelist.moveToNext());
  }else {
      SYS_LOGE ("Don't have table in %s !\n",table_name);
  }

  return err;
}

int CPQdb::PQ_GetOverscanParams(source_input_param_t source_input_param, vpp_display_mode_t dmode, tvin_cutwin_t *cutwin_t)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;
    char table_name[30];

    cutwin_t->hs = 0;
    cutwin_t->he = 0;
    cutwin_t->vs = 0;
    cutwin_t->ve = 0;
    tv_source_input_t source_input = source_input_param.source_input;
    tvin_sig_fmt_t fmt = source_input_param.sig_fmt;
    tvin_trans_fmt_t trans_fmt = source_input_param.trans_fmt;

    memset(table_name, 0, sizeof(table_name));
    switch ( dmode ) {
        case VPP_DISPLAY_MODE_169 :
            strcpy(table_name, "OVERSCAN_16_9");
            break;
        case VPP_DISPLAY_MODE_PERSON :
            strcpy(table_name, "OVERSCAN_PERSON");
            break;
        case VPP_DISPLAY_MODE_MOVIE :
            strcpy(table_name, "OVERSCAN_MOVIE");
            break;
        case VPP_DISPLAY_MODE_CAPTION :
            strcpy(table_name, "OVERSCAN_CAPTION");
            break;
        case VPP_DISPLAY_MODE_MODE43 :
            strcpy(table_name, "OVERSCAN_4_3");
            break;
        case VPP_DISPLAY_MODE_FULL :
            strcpy(table_name, "OVERSCAN_FULL");
            break;
        case VPP_DISPLAY_MODE_NORMAL :
            strcpy(table_name, "OVERSCAN_NORMAL");
            break;
        case VPP_DISPLAY_MODE_NOSCALEUP :
            strcpy(table_name, "OVERSCAN_NOSCALEUP");
            break;
        case VPP_DISPLAY_MODE_CROP_FULL :
            strcpy(table_name, "OVERSCAN_CROP_FULL");
            break;
        case VPP_DISPLAY_MODE_CROP :
            strcpy(table_name, "OVERSCAN_CROP");
            break;
        case VPP_DISPLAY_MODE_ZOOM :
            strcpy(table_name, "OVERSCAN_ZOOM");
            break;
        default :
            strcpy(table_name, "OVERSCAN_NORMAL");
            break;
    }
    getSqlParams(__FUNCTION__, sqlmaster, "select hs, he, vs, ve from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", table_name, source_input, fmt, trans_fmt);

    rval = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);

        getSqlParams(__FUNCTION__, sqlmaster, "select hs, he, vs, ve from %s where "
                                              "TVIN_PORT = %d and "
                                              "TVIN_SIG_FMT = %d and "
                                              "TVIN_TRANS_FMT = %d ;", table_name, source_input, fmt, trans_fmt);
        this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        cutwin_t->hs = c.getInt(0);
        cutwin_t->he = c.getInt(1);
        cutwin_t->vs = c.getInt(2);
        cutwin_t->ve = c.getInt(3);
    }
    return rval;
}
int CPQdb::PQ_SetOverscanParams(source_input_param_t source_input_param, tvin_cutwin_t cutwin_t)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;
    tv_source_input_t source_input = source_input_param.source_input;
    tvin_sig_fmt_t fmt = source_input_param.sig_fmt;
    tvin_trans_fmt_t trans_fmt = source_input_param.trans_fmt;

    getSqlParams(__FUNCTION__, sqlmaster,
        "select * from OVERSCAN where TVIN_PORT = %d and TVIN_SIG_FMT = %d and TVIN_TRANS_FMT = %d;",
        source_input, fmt, trans_fmt);

    rval = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGE ("%s - Load default", __FUNCTION__);

        getSqlParams(__FUNCTION__, sqlmaster,
                    "select * from OVERSCAN where TVIN_PORT = %d and TVIN_SIG_FMT = %d and TVIN_TRANS_FMT = %d;",
                    source_input, fmt, trans_fmt);
        this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        getSqlParams(__FUNCTION__, sqlmaster,
            "update OVERSCAN set hs = %d, he = %d, vs = %d, ve = %d where TVIN_PORT = %d and TVIN_SIG_FMT = %d and TVIN_TRANS_FMT = %d;",
            cutwin_t.hs, cutwin_t.he, cutwin_t.vs, cutwin_t.ve, source_input, fmt, trans_fmt);
    } else {
        getSqlParams(__FUNCTION__, sqlmaster,
            "Insert into OVERSCAN(TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, hs, he, vs, ve) values(%d, %d, %d ,%d ,%d, %d, %d);",
            source_input, fmt, trans_fmt, cutwin_t.hs, cutwin_t.he, cutwin_t.vs, cutwin_t.ve);
    }

    if (this->exeSql(sqlmaster)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}
int CPQdb::PQ_ResetAllOverscanParams(void)
{
    int rval;
    char sqlmaster[256];

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from OVERSCAN; insert into OVERSCAN(TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, hs, he, vs, ve) select TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, hs, he, vs, ve from OVERSCAN_default;");
    if (this->exeSql(sqlmaster)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

bool CPQdb::PQ_GetPqVersion(String8& ToolVersion, String8& ProjectVersion, String8& GenerateTime)
{
    bool ret = false;
    CSqlite::Cursor c;
    char sqlmaster[256];

    getSqlParams(__FUNCTION__, sqlmaster,"select ToolVersion,ProjectVersion,GenerateTime from PQ_VersionTable;");

    int rval = this->select(sqlmaster, c);

    if (!rval && c.getCount() > 0) {
        ToolVersion = c.getString(0);
        ProjectVersion = c.getString(1);
        GenerateTime = c.getString(2);
        ret = true;
    }

    return ret;
}

int CPQdb::PQ_GetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode,
                                vpp_pq_para_t *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];

    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR from Picture_Mode where "
                 "TVIN_PORT = %d and "
                 "Mode = %d ;", source_input, pq_mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        params->brightness = c.getInt(0);
        params->contrast = c.getInt(1);
        params->saturation = c.getInt(2);
        params->hue = c.getInt(3);
        params->sharpness = c.getInt(4);
        params->backlight = c.getInt(5);
        params->nr = c.getInt(6);
    } else {
        SYS_LOGE("%s error!\n",__FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_SetPQModeParams(tv_source_input_t source_input, vpp_picture_mode_t pq_mode, vpp_pq_para_t *params)
{
    int rval;
    char sql[256];

    getSqlParams(__FUNCTION__, sql,
        "update Picture_Mode set Brightness = %d, Contrast = %d, Saturation = %d, Hue = %d, Sharpness = %d, Backlight = %d, NR= %d "
        " where TVIN_PORT = %d and Mode = %d;", params->brightness, params->contrast,
        params->saturation, params->hue, params->sharpness, params->backlight, params->nr,
        source_input, pq_mode);
    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_SetPQModeParamsByName(const char *name, tv_source_input_t source_input,
                                      vpp_picture_mode_t pq_mode, vpp_pq_para_t *params)
{
    int rval;
    char sql[256];

    getSqlParams(__FUNCTION__, sql,
                 "insert into %s(TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR)"
                 " values(%d,%d,%d,%d,%d,%d,%d,%d,%d);", name, source_input, pq_mode,
                 params->brightness, params->contrast, params->saturation, params->hue,
                 params->sharpness, params->backlight, params->nr);

    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_ResetAllPQModeParams(void)
{
    int rval;
    char sqlmaster[256];

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from Picture_Mode; insert into Picture_Mode(TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR) select TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR from picture_mode_default;");

    if (this->exeSql(sqlmaster)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::PQ_GetGammaSpecialTable(vpp_gamma_curve_t gamma_curve, const char *f_name,
                                     tcon_gamma_table_t *gamma_value)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    if (gamma_curve == VPP_GAMMA_CURVE_DEFAULT) {
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_ALL", f_name);
    } else {
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_%d", f_name, gamma_curve);
    }

    rval = this->select(sqlmaster, c);
    if (c.moveToFirst()) {
        int index = 0;
        do {
            gamma_value->data[index] = c.getInt(0);
            index++;
        } while (c.moveToNext());
    } else {
        SYS_LOGE("%s, select %s error!\n", __FUNCTION__, f_name);
        rval = -1;
    }
    return rval;
}

int CPQdb::PQ_GetGammaTableR(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_r)
{
    return PQ_GetGammaTable(panel_id, source_input_param, "Red", gamma_r);
}

int CPQdb::PQ_GetGammaTableG(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_g)
{
    return PQ_GetGammaTable(panel_id, source_input_param, "Green", gamma_g);
}

int CPQdb::PQ_GetGammaTableB(int panel_id, source_input_param_t source_input_param, tcon_gamma_table_t *gamma_b)
{
    return PQ_GetGammaTable(panel_id, source_input_param, "Blue", gamma_b);
}

int CPQdb::PQ_GetGammaTable(int panel_id, source_input_param_t source_input_param, const char *f_name, tcon_gamma_table_t *val)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralGammaTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d;", source_input_param.source_input, source_input_param.sig_fmt);

    rval = this->select(sqlmaster, c);
    if (c.moveToFirst()) {
        int index_TableName = 0;
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from %s;", f_name,
                     c.getString(index_TableName).string());

        rval = this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            int index = 0;
            do {
                val->data[index] = c.getInt(0);
                index++;
            } while (c.moveToNext());
        }
    }
    return rval;
}

int CPQdb::PQ_GetVGAAjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t *adjparam)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    adjparam->clk_step = 0;
    adjparam->phase = 0;
    adjparam->hpos_step = 0;
    adjparam->vpos_step = 0;
    adjparam->vga_in_clean = 0;

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "select Clk, Phase, HPos, VPos, Vga_in_clean from VGA_AutoParams where TVIN_SIG_FMT = %d",
        vga_fmt);

    rval = this->select(sqlmaster, c);

    if (c.getCount() <= 0) {
        vga_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);

        getSqlParams(
                    __FUNCTION__,
                    sqlmaster,
                    "select Clk, Phase, HPos, VPos, Vga_in_clean from VGA_AutoParams where TVIN_SIG_FMT = %d",
                    vga_fmt);
        this->select(sqlmaster, c);
    }

    if (c.moveToFirst()) {
        adjparam->clk_step = c.getInt(0);
        adjparam->phase = c.getInt(1);
        adjparam->hpos_step = c.getInt(2);
        adjparam->vpos_step = c.getInt(3);
        adjparam->vga_in_clean = c.getInt(4);
    }
    return rval;
}

int CPQdb::PQ_SetVGAAjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t adjparam)
{
    CSqlite::Cursor c;
    char sql[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sql, "select * from VGA_AutoParams where TVIN_SIG_FMT = %d;",
                 vga_fmt);

    rval = this->select(sql, c);

    if (c.getCount() <= 0) {
        vga_fmt = TVIN_SIG_FMT_NULL;
        c.close();
        SYS_LOGD ("%s - Load default", __FUNCTION__);

        getSqlParams(
                    __FUNCTION__,
                    sql,
                    "select * from VGA_AutoParams where TVIN_SIG_FMT = %d;",
                    vga_fmt);
        this->select(sql, c);
    }

    if (c.moveToFirst()) {
        getSqlParams(
            __FUNCTION__,
            sql,
            "Insert into VGA_AutoParams(TVIN_SIG_FMT, Clk, Phase, HPos, VPos, Vga_in_clean) values(%d, %d, %d ,%d ,%d, %d);",
            vga_fmt, adjparam.clk_step, adjparam.phase, adjparam.hpos_step, adjparam.vpos_step,
            adjparam.vga_in_clean);
    } else {
        getSqlParams(
            __FUNCTION__,
            sql,
            "update VGA_AutoParams set Clk = %d, Phase = %d, HPos = %d, VPos = %d, Vga_in_clean = %d where TVIN_SIG_FMT = %d;",
            adjparam.clk_step, adjparam.phase, adjparam.hpos_step, adjparam.vpos_step,
            adjparam.vga_in_clean, vga_fmt);
    }
    if (this->exeSql(sql)) {
        rval = 0;
    } else {
        SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
        rval = -1;
    }

    return rval;
}

String8 CPQdb::GetTableName(const char *GeneralTableName, source_input_param_t source_input_param)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int ret = -1;
    switch (mDbMatchType) {
    case MATCH_TYPE_NO_DBVERSION:
        if (CheckIdExistInDb(CVBS_NAME_ID, "GeneralSharpness0FixedTable")) {
            if ((mOutPutType >= OUTPUT_TYPE_HDMI_4K) && (mOutPutType <= OUTPUT_TYPE_MAX)) {
                getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                             "TVIN_PORT = %d and "
                             "TVIN_SIG_FMT = %d and "
                             "TVIN_TRANS_FMT = %d and "
                             "TVOUT_CVBS = %d ;", GeneralTableName, source_input_param.source_input,
                             source_input_param.sig_fmt, source_input_param.trans_fmt, OUTPUT_TYPE_LVDS);
            } else {
                getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                             "TVIN_PORT = %d and "
                             "TVIN_SIG_FMT = %d and "
                             "TVIN_TRANS_FMT = %d and "
                             "TVOUT_CVBS = %d ;", GeneralTableName, source_input_param.source_input,
                             source_input_param.sig_fmt, source_input_param.trans_fmt, mOutPutType);
            }
        } else {
            getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                         "TVIN_PORT = %d and "
                         "TVIN_SIG_FMT = %d and "
                         "TVIN_TRANS_FMT = %d ;", GeneralTableName, source_input_param.source_input,
                         source_input_param.sig_fmt, source_input_param.trans_fmt);
        }
        ret = 0;
        break;
    case MATCH_TYPE_NEWCODE_OLDDB:
        SYS_LOGE("%s: new systemcontrol don't match old pq.db!\n", __FUNCTION__);
        break;
    case MATCH_TYPE_OLDCODE_NEWDB:
        SYS_LOGE("%s: old pq.db don't match new systemcontrol!\n", __FUNCTION__);
        break;
    case MATCH_TYPE_MATCH:
    default:
        if ((strcmp(GeneralTableName, "GeneralSharpness0FixedTable") == 0)
            || (strcmp(GeneralTableName, "GeneralSharpness0VariableTable") == 0)) {
            if (mOutPutType == OUTPUT_TYPE_LVDS) {//TV
                getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                             "TVIN_PORT = %d and "
                             "TVIN_SIG_FMT = %d and "
                             "TVIN_TRANS_FMT = %d ;", GeneralTableName, source_input_param.source_input,
                             source_input_param.sig_fmt, source_input_param.trans_fmt);
            } else {//HDMI OUTPUT
                getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                             "TVOUT_CVBS = %d ;", GeneralTableName, mOutPutType);
            }
        } else {
            getSqlParams(__FUNCTION__, sqlmaster, "select TableName from %s where "
                         "TVIN_PORT = %d and "
                         "TVIN_SIG_FMT = %d and "
                         "TVIN_TRANS_FMT = %d and "
                         "TVOUT_CVBS = %d ;", GeneralTableName, source_input_param.source_input,
                         source_input_param.sig_fmt, source_input_param.trans_fmt, OUTPUT_TYPE_LVDS);
        }
        ret = 0;
        break;
    }

    if (ret < 0) {
        return String8("");
    } else {
        ret = this->select(sqlmaster, c);
        if (ret == 0) {
            if (c.moveToFirst()) {
                SYS_LOGD("table name is %s!\n", c.getString(0).string());
                return c.getString(0);
            } else {
                SYS_LOGE("%s don't have this table!\n", GeneralTableName);
                return String8("");
            }
        } else {
            SYS_LOGE("%s: select action error!\n", __FUNCTION__);
            return String8("");
        }
    }
}


int CPQdb::CaculateLevelParam(tvpq_data_t *pq_data, int nodes, int level)
{
    int i;

    for (i = 0; i < nodes; i++) {
        if (level < pq_data[i].IndexValue) {
            break;
        }
    }

    if (i == 0) {
        return pq_data[i].RegValue;
    } else if (i == nodes) {
        return pq_data[i - 1].RegValue;
    } else {
        return pq_data[i - 1].RegValue + (level - pq_data[i - 1].IndexValue) * pq_data[i - 1].step;
    }
}

am_regs_t CPQdb::CaculateLevelRegsParam(tvpq_sharpness_regs_t *pq_regs, int level, int sharpness_number)
{
    int i;
    am_regs_t regs;
    memset(&regs, 0, sizeof(am_regs_t));
    int *pq_nodes = NULL;
    if (sharpness_number == 1) {//sharpness1
        pq_nodes = &sha1_nodes;
    } else if (sharpness_number == 0){//sharpness0
        pq_nodes = &sha0_nodes;
    } else {
        SYS_LOGE("%s: sharpness_number invalid!\n", __FUNCTION__);
        return regs;
    }

    for (i = 0; i < *pq_nodes; i++) {
        if (level < pq_regs[i].reg_data[0].IndexValue) {
            break;
        }
    }

    if (i == 0) {
        regs.length = pq_regs[i].length;
        for (int j = 0; j < pq_regs[i].length; j++) {
            regs.am_reg[j].type = pq_regs[i].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i].reg_data[j].Value.val;
        }

    } else if (i == *pq_nodes) {
        regs.length = pq_regs[i - 1].length;
        for (int j = 0; j < pq_regs[i - 1].length; j++) {
            regs.am_reg[j].type = pq_regs[i - 1].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i - 1].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i - 1].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i - 1].reg_data[j].Value.val;
        }
    } else {
        regs.length = pq_regs[i - 1].length;
        for (int j = 0; j < pq_regs[i - 1].length; j++) {
            regs.am_reg[j].type = pq_regs[i - 1].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i - 1].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i - 1].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i - 1].reg_data[j].Value.val + (level
                                 - pq_regs[i - 1].reg_data[j].IndexValue) * pq_regs[i - 1].reg_data[j].step;
            regs.am_reg[j].val &= regs.am_reg[j].mask;
        }
    }

    return MergeSameAddrVal(regs);
}

am_regs_t CPQdb::MergeSameAddrVal(am_regs_t regs)
{
    am_regs_t tmp_reg;
    unsigned int i = 0, k = 0;
    for (i=0;i<regs.length;i++) {
        if (regs.am_reg[i].addr == regs.am_reg[i+1].addr) {
            regs.am_reg[i+1].mask |=  regs.am_reg[i].mask;
            regs.am_reg[i+1].val |=  regs.am_reg[i].val;
        } else {
            tmp_reg.am_reg[k].type = regs.am_reg[i].type;
            tmp_reg.am_reg[k].addr = regs.am_reg[i].addr;
            tmp_reg.am_reg[k].mask = regs.am_reg[i].mask;
            tmp_reg.am_reg[k].val = regs.am_reg[i].val;
            k++;
        }
    }
    tmp_reg.length = k;

    /*SYS_LOGD("###length = %d\n", tmp_reg.length);
    for (i=0;i<tmp_reg.length;i++) {
        SYS_LOGD("###type: %u, addr: %u, mask: %u, val: %u\n", tmp_reg.am_reg[i].type, tmp_reg.am_reg[i].addr, tmp_reg.am_reg[i].mask, tmp_reg.am_reg[i].val);
    }*/

    return tmp_reg;
}

int CPQdb::GetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_t source_input, int level,
                                 int *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select Value from NonlinearMapping where "
                 "TVIN_PORT = %d and "
                 "Item_ID = %d and "
                 "Level = %d ;", source_input, data_type, level);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        *params = c.getInt(0);
    }
    return rval;
}

int CPQdb::GetNonlinearMappingByOSDFac(tvpq_data_type_t data_type, tv_source_input_t source_input, int *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select Value from NonlinearMapping where "
                 "TVIN_PORT = %d and "
                 "Item_ID = %d and ("
                 "Level = 0 or Level = 25 or Level = 50 or Level = 75 or Level = 100);", source_input, data_type);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        params[0] = c.getInt(0);
        params[1] = c.getInt(1);
        params[2] = c.getInt(2);
        params[3] = c.getInt(3);
        params[4] = c.getInt(4);
    }
    return rval;
}

int CPQdb::SetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_t source_input,
                                 int osd0, int osd25, int osd50, int osd75, int osd100)
{
    int rval;
    char *err = NULL;
    int osdvalue[101];
    double step[4];
    char sql[256];

    step[0] = (osd25 - osd0) / 25.0;
    step[1] = (osd50 - osd25) / 25.0;
    step[2] = (osd75 - osd50) / 25.0;
    step[3] = (osd100 - osd75) / 25.0;

    for (int i = 0; i <= 100; i++) {
        if (i == 0) {
            osdvalue[i] = osd0;
        } else if ((i > 0) && (i <= 25)) {
            osdvalue[i] = osd0 + (int) (i * step[0]);
        } else if ((i > 25) && (i <= 50)) {
            osdvalue[i] = osd25 + (int) ((i - 25) * step[1]);
        } else if ((i > 50) && (i <= 75)) {
            osdvalue[i] = osd50 + (int) ((i - 50) * step[2]);
        } else if ((i > 75) && (i <= 100)) {
            osdvalue[i] = osd75 + (int) ((i - 75) * step[3]);
        }
        getSqlParams(
            __FUNCTION__,
            sql,
            "update NonLinearMapping set Value = %d where TVIN_PORT = %d and Item_ID = %d and Level = %d ;",
            osdvalue[i], source_input, data_type, i);
        if (this->exeSql(sql)) {
            rval = 0;
        } else {
            SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
            rval = -1;
        }
    }

    return rval;
}

int CPQdb::SetNonlinearMappingByName(const char *name, tvpq_data_type_t data_type,
                                       tv_source_input_t source_input, int osd0, int osd25, int osd50, int osd75, int osd100)
{
    int rval;
    char *err = NULL;
    int osdvalue[101];
    double step[4];
    char sql[256];

    step[0] = (osd25 - osd0) / 25.0;
    step[1] = (osd50 - osd25) / 25.0;
    step[2] = (osd75 - osd50) / 25.0;
    step[3] = (osd100 - osd75) / 25.0;

    for (int i = 0; i <= 100; i++) {
        if (i == 0) {
            osdvalue[i] = osd0;
        } else if ((i > 0) && (i <= 25)) {
            osdvalue[i] = osd0 + (int) (i * step[0]);
        } else if ((i > 25) && (i <= 50)) {
            osdvalue[i] = osd25 + (int) ((i - 25) * step[1]);
        } else if ((i > 50) && (i <= 75)) {
            osdvalue[i] = osd50 + (int) ((i - 50) * step[2]);
        } else if ((i > 75) && (i <= 100)) {
            osdvalue[i] = osd75 + (int) ((i - 75) * step[3]);
        }
        memset(sql, '\0', 256);
        getSqlParams(__FUNCTION__, sql,
                     "insert into %s(TVIN_PORT, Item_ID, Level, Value) values(%d,%d,%d,%d);", name,
                     source_input, data_type, i, osdvalue[i]);
        if (this->exeSql(sql)) {
            rval = 0;
        } else {
            SYS_LOGE("%s--SQL error!\n",__FUNCTION__);
            rval = -1;
        }
    }

    return rval;
}

int CPQdb::loadSharpnessData(const char *table_name, int sharpness_number)
{
    CSqlite::Cursor c;
    int rval;
    int *pq_nodes = NULL;
    char sqlmaster[256];

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select TotalNode, NodeNumber, RegType, RegAddr, RegMask,"
                 "IndexValue, RegValue, StepUp from %s order by NodeNumber asc;",
                 table_name);
    rval = this->select(sqlmaster, c);
    int length = 0;
    int index = 0;

    if (sharpness_number == 1) {//for Sharpness_1
        pq_nodes = &sha1_nodes;
        if (c.moveToFirst()) {
            *pq_nodes = c.getInt(0);
            length = c.getCount() / (*pq_nodes);
            for (int i = 0; i < *pq_nodes; i++) {
                pq_sharpness1_reg_data[i].length = length;
            }
            do {
                pq_sharpness1_reg_data[index / length].reg_data[index % length].TotalNode
                    = c.getInt(0);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].NodeValue
                    = c.getInt(1);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.type
                    = c.getUInt(2);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.addr
                    = c.getUInt(3);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.mask
                    = c.getUInt(4);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].IndexValue
                    = c.getInt(5);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].Value.val
                    = c.getUInt(6);
                pq_sharpness1_reg_data[index / length].reg_data[index % length].step = c.getF(7);
                index++;
            } while (c.moveToNext());
        }else {
            SYS_LOGE("%s: select sharpness1 value error!\n", __FUNCTION__);
            rval = -1;
        }
    } else if (sharpness_number == 0) {//for Sharpness_0
        pq_nodes = &sha0_nodes;
        if (c.moveToFirst()) {
            *pq_nodes = c.getInt(0);//TotalNode?
            length = c.getCount() / (*pq_nodes);
            for (int i = 0; i < *pq_nodes; i++) {
                pq_sharpness0_reg_data[i].length = length;
            }
            do {
                pq_sharpness0_reg_data[index / length].reg_data[index % length].TotalNode
                    = c.getInt(0);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].NodeValue
                    = c.getInt(1);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.type
                    = c.getUInt(2);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.addr
                    = c.getUInt(3);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.mask
                    = c.getUInt(4);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].IndexValue
                    = c.getInt(5);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].Value.val
                    = c.getUInt(6);
                pq_sharpness0_reg_data[index / length].reg_data[index % length].step = c.getF(7);
                index++;
            } while (c.moveToNext());
        }else {
            SYS_LOGE("%s: select sharpness0 value error!\n", __FUNCTION__);
            rval = -1;
        }
    }else {
        SYS_LOGE("%s: sharpness_number invalid!\n", __FUNCTION__);
        rval = -1;
    }

    return rval;
}

int CPQdb::LoadVppBasicParam(tvpq_data_type_t data_type, source_input_param_t source_input_param)
{
    CSqlite::Cursor c;
    int rval = -1;
    char sqlmaster[256];
    char table_name[128];
    tvpq_data_t *pq_data = NULL;
    int *pq_nodes = NULL;
    String8 tableName[] = {String8("GeneralBrightnessTable"), String8("GeneralContrastTable"),
                           String8("GeneralSaturationTable"), String8("GeneralHueTable")};

    switch (data_type) {
    case TVPQ_DATA_BRIGHTNESS:
        pq_data = pq_bri_data;
        pq_nodes = &bri_nodes;
        break;
    case TVPQ_DATA_CONTRAST:
        pq_data = pq_con_data;
        pq_nodes = &con_nodes;
        break;
    case TVPQ_DATA_SATURATION:
        pq_data = pq_sat_data;
        pq_nodes = &sat_nodes;
        break;
    case TVPQ_DATA_HUE:
        pq_data = pq_hue_data;
        pq_nodes = &hue_nodes;
        break;
    default:
        return rval;
    }

    if (CheckHdrStatus(tableName[data_type]))
        source_input_param.sig_fmt = TVIN_SIG_FMT_HDMI_HDR;

    String8 TableName = GetTableName(tableName[data_type].string(), source_input_param);
    if ((TableName.string() != NULL) && (TableName.length() != 0) ) {
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "select TotalNode, NodeNumber, IndexValue, RegValue, StepUp from %s order by NodeNumber asc;",
            TableName.string());

        rval = this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            int index = 0;
            do {
                pq_data[index].TotalNode = c.getInt(0);
                pq_data[index].NodeValue = c.getInt(1);
                pq_data[index].IndexValue = c.getInt(2);
                pq_data[index].RegValue = c.getInt(3);
                pq_data[index].step = c.getF(4);
                index++;
            } while (c.moveToNext());
            *pq_nodes = index;
        }else {
            SYS_LOGE("%s: select value error!\n", __FUNCTION__);
        }
    } else {
        SYS_LOGE("%s: %s don't have this table!\n", __FUNCTION__, tableName[data_type].string());
    }
    return rval;
}

const char *Pmode_name[6] = { "Picture_Mode", "Picture_Mode_Default", "NonlinearMapping",
                              "NonlinearMapping_Default", "VGA_AutoParams", "OVERSCAN"
                            };

void CPQdb::initialTable(int type)
{
    vpp_pq_para_t pmode_default;

    pmode_default.backlight = 100;
    pmode_default.brightness = 50;
    pmode_default.contrast = 50;
    pmode_default.hue = 50;
    pmode_default.nr = 0;
    pmode_default.saturation = 50;
    pmode_default.sharpness = 50;

    switch (type) {
    case TYPE_PMode:
    case TYPE_PMode_Default:
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 4; j++) {
                PQ_SetPQModeParamsByName(Pmode_name[type], (tv_source_input_t) i,
                                         (vpp_picture_mode_t) j, &pmode_default);
            }
        }
        break;
    case TYPE_Nonlinear:
    case TYPE_NonLinear_Default:
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 5; j++) {
                SetNonlinearMappingByName(Pmode_name[type], (tvpq_data_type_t) j,
                                          (tv_source_input_t) i, 0, (int) 255 / 4.0, (int) 255 * 2 / 4.0,
                                          (int) 255 * 3 / 4.0, 255);
            }
        }
        break;
    case TYPE_VGA_AUTO:
        break;
    case TYPE_OVERSCAN:
        break;
    }
}

int CPQdb::PQ_GetPhaseArray(am_phase_t *am_phase)
{
    CSqlite::Cursor c;
    int iOutRet = 0;
    char sqlmaster[256];
    getSqlParams(__FUNCTION__, sqlmaster, "select Phase from Phase order by Format ASC; ");

    this->select(sqlmaster, c);
    int nums = 0;
    am_phase->length = c.getCount();
    if (c.moveToFirst()) {
        do {
            am_phase->phase[nums] = c.getInt(0);
            nums++;
        } while (c.moveToNext());
    }

    return nums;
}

bool CPQdb::PQ_GetLDIM_Regs(vpu_ldim_param_s *vpu_ldim_param)
{
    CSqlite::Cursor c;
    bool ret = true;
    int i = 0;
    int ldimMemsSize = sizeof (vpu_ldim_param_s) / sizeof (int);

    SYS_LOGD ("%s, entering...\n", __FUNCTION__);
    SYS_LOGD ("ldimMemsSize = %d\n", ldimMemsSize);

    if (vpu_ldim_param != NULL) {
        int* temp = reinterpret_cast<int*>(vpu_ldim_param);

        if (this->select("select value from LDIM_1; ", c) != -1 ) {
            int retNums = c.getCount();

            SYS_LOGD ("retNums = %d\n", retNums);

            if ( retNums > 0 && retNums == ldimMemsSize ) {
                do {

                    temp[i] = c.getUInt(0);
                    SYS_LOGD ("%d - %d\n", i + 1, temp[i]);

                    i++;
                }while (c.moveToNext());
            }
            else {
                SYS_LOGV ("DataBase not match vpu_ldim_param_s\n");
                ret = false;
            }
        }
        else {
            SYS_LOGV ("select value from LDIM_1; failure\n");
            ret = false;
        }
    }

    return ret;
}

bool CPQdb::CheckHdrStatus(const char *tableName)
{
    bool ret = false;
    char sqlmaster[256];
    CSqlite::Cursor tempCursor;

    if (mHdrStatus) {
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select TableName from %s where TVIN_SIG_FMT = %d;",
                     tableName, TVIN_SIG_FMT_HDMI_HDR);

        this->select(sqlmaster, tempCursor);
        if (tempCursor.getCount() > 0) {
            ret = true;
        }
    }

    return ret;
}

int CPQdb::GetFileAttrIntValue(const char *fp, int flag)
{
    int fd = -1, ret = -1;
    int temp = -1;
    char temp_str[32];

    memset(temp_str, 0, 32);

    fd = open(fp, flag);

    if (fd <= 0) {
        SYS_LOGE("open %s ERROR(%s)!!\n", fp, strerror(errno));
        return -1;
    }

    if (read(fd, temp_str, sizeof(temp_str)) > 0) {
        if (sscanf(temp_str, "%d", &temp) >= 0) {
            SYS_LOGD("get %s value =%d!\n", fp, temp);
            close(fd);
            return temp;
        } else {
            SYS_LOGE("get %s value error(%s)\n", fp, strerror(errno));
            close(fd);
            return -1;
        }
    }

    close(fd);
    return -1;
}

bool CPQdb::CheckCVBSParamValidStatus()
{
    bool ret = false;
    char sqlmaster[256];
    CSqlite::Cursor tempCursor;

    if (CheckIdExistInDb(CVBS_NAME_ID, "GeneralNR2Table")) {
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from GeneralNR2Table where "
                     "TVIN_PORT = %d and "
                     "TVIN_SIG_FMT = %d and "
                     "TVIN_TRANS_FMT = %d and "
                     "TVOUT_CVBS = %d;", TABLE_NAME_ID, SOURCE_MPEG,
                     TVIN_SIG_FMT_HDMI_1920X1080P_60HZ, TVIN_TFMT_2D, 1);
        if ((this->select(sqlmaster, tempCursor) == 0) && (tempCursor.moveToFirst())) {
            ret = true;
        } else {
            SYS_LOGD("%s: new db, but don't have cvbs param!\n", __FUNCTION__);
            ret = false;
        }
    } else {
        SYS_LOGD("%s: old db, don't have cvbs param!\n", __FUNCTION__);
        ret = false;
    }
    return ret;
}

bool CPQdb::CheckIdExistInDb(const char *Id, const char *TableName)
{
    bool ret = false;
    char sqlmaster[256];
    CSqlite::Cursor tempCursor;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select sql from sqlite_master where type = 'table' and tbl_name = '%s';", TableName);

    int retVal = this->select(sqlmaster, tempCursor);
    if ((retVal == 0) && (tempCursor.moveToFirst())) {
        if (strstr(tempCursor.getString(0).c_str(), Id) != NULL) {
            ret = true;
        } else {
            ret = false;
        }
    } else {
        SYS_LOGE("%s: error!\n", __FUNCTION__);
        ret = false;
    }

    /*if (ret) {
        SYS_LOGE("%s: %s exist in %s!\n", __FUNCTION__, Id, TableName);
    } else {
        SYS_LOGE("%s: %s don't exist in %s!\n", __FUNCTION__, Id, TableName);
    }*/

    return ret;
}

