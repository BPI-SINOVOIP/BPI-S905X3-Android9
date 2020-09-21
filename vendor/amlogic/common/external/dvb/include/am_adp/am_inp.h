/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief 输入设备模块
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-05-19: create the document
 ***************************************************************************/

#ifndef _AM_INP_H
#define _AM_INP_H

#include "am_types.h"
#include <linux/input.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief 输入模块错误代码*/
enum AM_INP_ErrorCode
{
	AM_INP_ERROR_BASE=AM_ERROR_BASE(AM_MOD_INP),
	AM_INP_ERR_INVALID_DEV_NO,           /**< 无效的设备号*/
	AM_INP_ERR_BUSY,                     /**< 设备已经打开*/
	AM_INP_ERR_NOT_OPENNED,              /**< 设备还没有打开*/
	AM_INP_ERR_CANNOT_OPEN_DEV,          /**< 打开设备失败*/
	AM_INP_ERR_CANNOT_CREATE_THREAD,     /**< 无法创建线程*/
	AM_INP_ERR_TIMEOUT,                  /**< 等待输入事件超时*/
	AM_INP_ERR_NOT_SUPPORTED,            /**< 设备不支持此功能*/
	AM_INP_ERR_READ_FAILED,              /**< 读取设备文件出错*/
	AM_INP_ERR_UNKNOWN_KEY,              /**< 无效按键*/
	AM_INP_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief 输入设备开启参数*/
typedef struct
{
	AM_Bool_t    enable_thread;          /**< 始能输入事件检测线程*/
} AM_INP_OpenPara_t;

/**\brief 按键名*/
typedef enum
{
	AM_INP_KEY_UNKNOWN,
	AM_INP_KEY_UP,                       /**< 上*/
	AM_INP_KEY_DOWN,                     /**< 下*/
	AM_INP_KEY_LEFT,                     /**< 左*/
	AM_INP_KEY_RIGHT,                    /**< 右*/
	AM_INP_KEY_PGUP,                     /**< 上翻页*/
	AM_INP_KEY_PGDOWN,                   /**< 下翻页*/
	AM_INP_KEY_0,                        /**< 数字0*/
	AM_INP_KEY_1,                        /**< 数字1*/
	AM_INP_KEY_2,                        /**< 数字2*/
	AM_INP_KEY_3,                        /**< 数字3*/
	AM_INP_KEY_4,                        /**< 数字4*/
	AM_INP_KEY_5,                        /**< 数字5*/
	AM_INP_KEY_6,                        /**< 数字6*/
	AM_INP_KEY_7,                        /**< 数字7*/
	AM_INP_KEY_8,                        /**< 数字8*/
	AM_INP_KEY_9,                        /**< 数字9*/
	AM_INP_KEY_MUTE,                     /**< 静音*/
	AM_INP_KEY_VOLUP,                    /**< 音量+*/
	AM_INP_KEY_VOLDOWN,                  /**< 音量-*/
	AM_INP_KEY_CHUP,                     /**< 频道+*/
	AM_INP_KEY_CHDOWN,                   /**< 频道-*/
	AM_INP_KEY_OK,                       /**< 确定*/
	AM_INP_KEY_CANCEL,                   /**< 取消*/
	AM_INP_KEY_MENU,                     /**< 菜单*/
	AM_INP_KEY_EXIT,                     /**< 退出*/
	AM_INP_KEY_BACK,                     /**< 返回*/
	AM_INP_KEY_PLAY,                     /**< 播放*/
	AM_INP_KEY_PAUSE,                    /**< 暂停*/
	AM_INP_KEY_STOP,                     /**< 停止*/
	AM_INP_KEY_REC,                      /**< 图像*/
	AM_INP_KEY_SKIP,                     /**< 跳至下一个*/
	AM_INP_KEY_REWIND,                   /**< 重新播放当前*/
	AM_INP_KEY_POWER,                    /**< 电源*/
	AM_INP_KEY_RED,                      /**< 红*/
	AM_INP_KEY_YELLOW,                   /**< 黄*/
	AM_INP_KEY_BLUE,                     /**< 蓝*/
	AM_INP_KEY_GREEN,                    /**< 绿*/
	AM_INP_KEY_COUNT
} AM_INP_Key_t;

/**\brief 键值映射表*/
typedef struct
{
	int codes[AM_INP_KEY_COUNT];         /**< 存放每个按键的键码值*/
} AM_INP_KeyMap_t;

/**\brief 输入设备回调函数*/
typedef void (*AM_INP_EventCb_t) (int dev_no, struct input_event *evt, void *user_data);

/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief 打开一个输入设备
 * \param dev_no 输入设备号
 * \param[in] para 设备开启参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_Open(int dev_no, const AM_INP_OpenPara_t *para);

/**\brief 关闭一个输入设备
 * \param dev_no 输入设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_Close(int dev_no);

/**\brief 使能输入设备，之后输入设备产生的输入事件可以被读取
 * \param dev_no 输入设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_Enable(int dev_no);

/**\brief 禁止输入设备,禁止之后，设备不会返回输入事件
 * \param dev_no 输入设备号
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_Disable(int dev_no);

/**\brief 等待一个输入事件
 *挂起调用线程直到输入设备产生一个输入事件。如果超过超时时间没有事件，返回AM_INP_ERR_TIMEOUT。
 * \param dev_no 输入设备号
 * \param[out] evt 返回输入事件
 * \param timeout 以毫秒为单位的超时时间，<0表示永远等待，=0表示检查后不等待立即返回
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_WaitEvent(int dev_no, struct input_event *evt, int timeout);

/**\brief 向输入设备注册一个事件回调函数
 *输入设备将创建一个线程，检查输入输入事件，如果有输入事件，线程调用回调函数。
 *如果回调函数为NULL,表示取消回调函数，输入设备将结束事件检测线程。
 * \param dev_no 输入设备号
 * \param[in] cb 新注册的回调函数
 * \param[in] user_data 回调函数的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_SetCallback(int dev_no, AM_INP_EventCb_t cb, void *user_data);

/**\brief 取得输入设备已经注册事件回调函数及参数
 * \param dev_no 输入设备号
 * \param[out] cb 返回回调函数指针
 * \param[out] user_data 返回回调函数的用户参数
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_GetCallback(int dev_no, AM_INP_EventCb_t *cb, void **user_data);

/**\brief 设定输入设备的键码映射表
 * \param dev_no 输入设备号
 * \param[in] map 新的键码映射表，如果为NULL表示不使用映射表，直接返回键值
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_inp.h)
 */
extern AM_ErrorCode_t AM_INP_SetKeyMap(int dev_no, const AM_INP_KeyMap_t *map);

#ifdef __cplusplus
}
#endif

#endif

