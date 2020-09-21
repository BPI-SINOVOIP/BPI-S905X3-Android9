/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/

#include <SystemControlClient.h>

#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <utils/Atomic.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/threads.h>
#include <unistd.h>

#include "am_debug.h"

#include "am_sys_write.h"

using namespace android;

class DeathNotifier: public IBinder::DeathRecipient
{
    public:
        DeathNotifier()
        {
        }
        void binderDied(const wp < IBinder > &who)
        {
            AM_DEBUG(1, "system_write died !");
        }
};

static SystemControlClient *mScc = new SystemControlClient();
static sp < DeathNotifier > amDeathNotifier;
static Mutex amgLock;

/**\brief read sysfs value
 * \param[in] path file name
 * \param[out] value: read file info
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_SystemControl_Read_Sysfs(const char *path, char *value)
{
    if (path == NULL || value == NULL)
    {
        AM_DEBUG(1,"[false]AM_SystemControl_Read_Sysfs path or value is null");
        return AM_FAILURE;
    }
    //AM_DEBUG(1,"AM_SystemControl_Read_Sysfs:%s",path);
    if (mScc != NULL)
    {
        const std::string stdPath(path);
        std::string stdValue(value);
        if (mScc->readSysfs(stdPath, stdValue))
        {
            strcpy(value, stdValue.c_str());
            return AM_SUCCESS;
        }
    }
    //AM_DEBUG(1,"[false]AM_SystemControl_Read_Sysfs%s,",path);
    return AM_FAILURE;
}
/**\brief read num sysfs value
 * \param[in] path file name
 * \param[in] size: need read file length
 * \param[out] value: read file info
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_SystemControl_ReadNum_Sysfs(const char *path, char *value, int size)
{
    if (path == NULL || value == NULL)
    {
        AM_DEBUG(1,"[false]AM_SystemControl_ReadNum_Sysfs path or value is null");
        return AM_FAILURE;
    }
    //AM_DEBUG(1,"AM_SystemControl_ReadNum_Sysfs:%s",path);
    if (mScc != NULL && value != NULL && access(path, 0) != -1)
    {
        const std::string stdPath(path);
        std::string stdValue(value);
        if (mScc->readSysfs(stdPath, stdValue))
        {
            if (stdValue.size() != 0)
            {
                //AM_DEBUG(1,"readSysfs ok:%s,%s,%d", path, String8(v).string(), String8(v).size());
                memset(value, 0, size);
                if (size <= stdValue.size() + 1)
                {
                    memcpy(value, stdValue.c_str(),
                           size - 1);
                    value[strlen(value)] = '\0';
                }
                else
                {
                    strcpy(value, stdValue.c_str());
                }
                return AM_SUCCESS;
            }
        }
    }
    //AM_DEBUG(1,"[false]AM_SystemControl_ReadNum_Sysfs%s,",path);
    return AM_FAILURE;
}
/**\brief write sysfs value
 * \param[in] path: file name
 * \param[in] value: write info to file
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_SystemControl_Write_Sysfs(const char *path, char *value)
{
    if (path == NULL || value == NULL)
    {
        AM_DEBUG(1,"[false]AM_SystemControl_Write_Sysfs path or value is null");
        return AM_FAILURE;
    }
    //AM_DEBUG(1,"AM_SystemControl_Write_Sysfs:%s",path);
    if (mScc != NULL)
    {
        const std::string stdPath(path);
        std::string stdValue(value);
        if (mScc->writeSysfs(stdPath, stdValue))
        {
            //AM_DEBUG(1,"writeSysfs ok");
            return AM_SUCCESS;
        }
    }
    //AM_DEBUG(1,"[false]AM_SystemControl_Write_Sysfs%s,",path);
    return AM_FAILURE;
}
