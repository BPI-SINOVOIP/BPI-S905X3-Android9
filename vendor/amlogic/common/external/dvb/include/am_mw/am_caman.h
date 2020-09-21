/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_caman.h
 * \brief CA manage
 *
 * \author 
 * \date 
 ***************************************************************************/
 
#ifndef _AM_CAMAN_H_
#define _AM_CAMAN_H_

#include "am_types.h"

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
/**\brief Error code of the CA manage module*/
enum AM_CAMAN_ErrorCode
{
	AM_CAMAN_ERROR_BASE=AM_ERROR_BASE(AM_MOD_CAMAN),
	AM_CAMAN_ERROR_CA_UNKOWN,                         /**< Unknown CA*/
	AM_CAMAN_ERROR_CA_EXISTS,                         /**< CA already exists*/
	AM_CAMAN_ERROR_CA_ERROR,                          /**< CA error*/
	AM_CAMAN_ERROR_NO_MEM,                            /**< out of memory*/
	AM_CAMAN_ERROR_CANNOT_CREATE_THREAD,              /**< Thread creation failed*/
	AM_CAMAN_ERROR_BADPARAM,                          /**< Parameter error*/
	AM_CAMAN_ERROR_NOTOPEN,                           /**< CA management module is not open*/
	AM_CAMAN_ERROR_END                                
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/
 
/**\brief CA type*/
typedef enum {
	AM_CA_TYPE_CA,		/**< CA */
	AM_CA_TYPE_CI			/**< CI */
} AM_CA_Type_t;

/**\brief CA message destination identifier*/
typedef enum {
	AM_CA_MSG_DST_CA,     /**< message destination CA */
	AM_CA_MSG_DST_CAMAN,  /**< message destination CA manage */
	AM_CA_MSG_DST_APP			/**< message destination APP */
} AM_CA_Msg_Dst_t;

/**\brief CAMAN parameter of ts*/
typedef struct {
	int fend_fd;           /**< frontend device number*/
	int dmx_fd;            /**< dmx device number*/
	int pmt_timeout;       /**< pmt timeout，Unit millisecond*/
	int pmt_mon_interval;  /**< pmt monitoring detection interval，Unit millisecond*/
	int pat_timeout;       /**< pat timeout，Unit millisecond*/
	int pat_mon_interval;  /**< pat monitoring detection interval，Unit millisecond*/
	int cat_timeout;       /**< cat timeout，Unit millisecond*/
	int cat_mon_interval;  /**< cat monitoring detection interval，Unit millisecond*/
} AM_CAMAN_Ts_t;

/**\brief CAMAN open parameter*/
typedef struct {
	AM_CAMAN_Ts_t ts;
} AM_CAMAN_OpenParam_t;

/**\brief CA message structure*/
typedef struct {
	int type;             /**< Message type*/
	int dst;              /**< Message destination, see AM_CA_Msg_Dst_t*/
	unsigned char *data; /**< Message content*/
	unsigned int len;    /**< Message content len*/
} AM_CA_Msg_t;

/*
		CA				CAMAN					APP

	AM_Ca_t	ca			man register	<--	registerCA
												|
											setCallback
												|
	open,enable(1) <--	call			<--	openCA()
	
		---------------service-loop-------------------
												|
	ts_changed()		new ts event	<--	FEND_EVT_STATUS_CHANGE
						get CAT			startService(sid, caname) force-mode
						get PAT
							|
						match/get PMT
							|
	camatch()	<--	(automatch ca/force ca)
				 			|
							|
	startpmt()	<--		PMT(s)		

				.......................................
					monitor tables (service related)
					notify/resetup ca if changed
				.......................................
					
		
	stoppmt()	<--		stop			<-- stopCA(name)/stopService
	
		------------service-loop-------------------


		------------msg-loop-----------------------
												poll or callback
	msg_send	-->		in the pool      <-- getMsg(display)[poll]
										--> msg_callback()[callback]

	msg_receive  <--		call		<-- putMsg(trigger/reply)									
		------------msg-loop-----------------------
	

	close,enable(0) <--	call			<-- close_CA
						man unregister<-- unregisterCA
						  
*/

typedef struct AM_CA_s AM_CA_t;

typedef struct AM_CA_Ops_s AM_CA_Ops_t;

typedef struct AM_CA_Opt_s AM_CA_Opt_t;

/**\brief CA operation structure
* Each CA that is registered to the CA management module needs to generate
* the AM_CA_Ops_s type object. CA management module with the object to operate
* management CA
*/
struct AM_CA_Ops_s{
	/*init/term the ca*/
	int (*open)(void *arg, AM_CAMAN_Ts_t *ts);/**< open CA*/
	int (*close)(void *arg);/**< close CA*/

	/*check if the ca can work with the ca system marked as the caid*/
	int (*camatch)(void *arg, unsigned int caid);/**< check if the ca can work with the ca system marked as the caid*/

	/*caman triggers the ca when there is a ts-change event*/
	int (*ts_changed)(void *arg);/**< caman triggers the ca when there is a ts-change event*/

	/*notify the new cat*/
	int (*new_cat)(void *arg, unsigned char *cat, unsigned int size);/**< notify the new cat*/
	
	/*caman ask the ca to start/stop working on a pmt*/
	int (*start_pmt)(void *arg, int service_id, unsigned char *pmt, unsigned int size);/**< caman ask the ca to start working on a pmt*/
	int (*stop_pmt)(void *arg, int service_id);/**< caman ask the ca to stop working on a pmt*/

	/*ca can exchange messages with caman only if enable() is called with a non-zero argument.
	    ca must stop exchanging msgs with msg funcs after enable(0) is called*/
	int (*enable)(void *arg, int enable);/**< CA enable or disable，ca must stop exchanging msgs with msg funcs after enable(0) is called*/

	/*the ca will use the func_msg registered to send msgs to the upper layer through caman
		send_msg()'s return value:
			  0 - success
			-1 - wrong name
			-2 - mem fail
	*/
	/**\brief register message send function into CA manage module
	*	CA use send_msg Pointer to send message to app or CA manage module
	*	the msg member of data need continuous memory block，app or ca will free data when use end.
	*/
	int (*register_msg_send)(void *arg, char *name, int (*send_msg)(char *name, AM_CA_Msg_t *msg));/**< free message*/
	/*with which the caman can free the space occupied by the msg*/
	void (*free_msg)(void *arg, AM_CA_Msg_t *msg);/**< free message function*/
	/*msg replys will come within the arg of this callback*/
	int (*msg_receive)(void *arg, AM_CA_Msg_t *msg);/**< send message to CA manage module,CA manage module Process message in this function*/
};
/**\brief CA structure*/
struct AM_CA_s{
	/*ca type in AM_CA_Type_t*/
	AM_CA_Type_t type;/**< ca type in AM_CA_Type_t*/
	/*ca ops*/
	AM_CA_Ops_t ops;/**< CA operation */
	/*ca private args*/
	void *arg;/**< ca private args */
	void (*arg_destroy)(void *arg);/**< ca free private args function */
};
/**\brief CA auto operation structure*/
struct AM_CA_Opt_s{
	/*if the ca will be checked for the auto-match*/
	int auto_disable;/**< ca auto disable*/
};

/*CAMAN open/close*/

/**\brief Open CA manage module
 * \param [in] para open parameter
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_Open(AM_CAMAN_OpenParam_t *para);

/**\brief Close CA manage module
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_Close(void);

/**\brief Pause CA manage module
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_Pause(void);

/**\brief Resume CA manage module
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_Resume(void);

/*CA open/close*/

/**\brief Open registed CA
 * \param [in] name CA name
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_openCA(char *name);

/**\brief Close registed CA
 * \param [in] name CA name
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_closeCA(char *name);

/**\brief Start Descramble Service
 * \param [in] service_id Serviceid
 * \param [in] caname     select ca by name
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_startService(int service_id, char *caname);

/**\brief Stop Descramble Service
 * \param [in] service_id Serviceid
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_stopService(int service_id);

/**\brief Stop CA
 * \param [in] caname   stop CA name
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_stopCA(char *caname);

/*get/free Msgs from CAMAN/CA
    get -> use -> free
*/

/**\brief Get CA Message initiative
 * \param [out]  caname 	The CA name who send message.
 * \param [in,out] msg    the pointer of store message pointer，need free by AM_CAMAN_freeMsg
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_getMsg(char *caname, AM_CA_Msg_t **msg);

/**\brief free message pointer
 * \param [in] msg the pointer of store message pointer，get by AM_CAMAN_getMsg()
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_freeMsg(AM_CA_Msg_t *msg);

/*send Msg to CAMAN/CA, it's the caller's responsibility to free the Msg*/

/**\brief send message
 * \param [in] caname send message to CA name,if name is null,this message will broadcast
 * \param [in] msg    send message pointer
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_putMsg(char *caname, AM_CA_Msg_t *msg);

/*register/unregister CAs*/

/**\brief register CA into CA manage module
 * \param [in] caname register CA name
 * \param [in] ca     register CA struct obj
 * \param [in] opt    register parameter
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_registerCA(char *caname, AM_CA_t *ca, AM_CA_Opt_t *opt);

/**\brief unregister CA from CA manage module
 * \param [in] caname unregister CA name
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_unregisterCA(char *caname);

/**\brief Set CA message callback function，get CA message by callback type
 * \param [in] caname ca name
 * \param [in] cb  process message callback function pointer
 * \retval AM_SUCCESS On success
 * \return Error code
 */
AM_ErrorCode_t AM_CAMAN_setCallback(char *caname, int (*cb)(char *name, AM_CA_Msg_t *msg));

#ifdef __cplusplus
}
#endif

#endif

