/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_db.h
 * \brief Database module
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-20: create the document
 ***************************************************************************/

#ifndef _AM_DB_H
#define _AM_DB_H

#include <am_types.h>
#include <sqlite3.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/**\brief Maximum TS number in one source*/
#define AM_DB_MAX_TS_CNT_PER_SRC 100

/**\brief Maximum service number in one source*/
#define AM_DB_MAX_SRV_CNT_PER_SRC 1000

/**\brief Maximum service name length*/
#define AM_DB_MAX_SRV_NAME_LEN 64

#define AM_DB_HANDLE_PREPARE(hdb)	\
	AM_MACRO_BEGIN \
		AM_ErrorCode_t private_db_err = \
			AM_DB_GetHandle(&hdb); \
		assert((private_db_err==AM_SUCCESS) && hdb); \
	AM_MACRO_END

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/**\brief Error code of the databasemodule*/
enum AM_DB_ErrorCode
{
	AM_DB_ERROR_BASE=AM_ERROR_BASE(AM_MOD_DB),
	AM_DB_ERR_OPEN_DB_FAILED,    		/**< Open the database failed*/
	AM_DB_ERR_CREATE_TABLE_FAILED,    	/**< Create table failed*/
	AM_DB_ERR_NO_MEM,                       /**< Not enough memory*/
	AM_DB_ERR_INVALID_PARAM,                /**< Invalid parameter*/
	AM_DB_ERR_SELECT_FAILED,                /**< Select failed*/
	AM_DB_ERR_END
};

/****************************************************************************
 * Type definitions
 ***************************************************************************/


/****************************************************************************
 * Function prototypes  
 ***************************************************************************/

/**\brief Database initialize
 * \param [out] handle Return the database handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_Init(sqlite3 **handle);

/**\brief Database initialize
 * \param [in]  path   Database file path
 * \param [out] handle Return the database handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_Init2(char *path, sqlite3 **handle);

/**\brief Release the database handle
 * \param [in] handle The database handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_Quit(sqlite3 *handle);

/**\brief Run a select statement
 * \param [in] handle Database handle
 * \param [in] sql SQL select statement
 * \param [in,out] max_row  Maximum row count, return the real row count
 * \param [in] fmt Result pattern string
 * Each field is related to the record field in \a sql.
 * For example, "%d %s:256 %f":
 * "%d" means the field is a integer number.
 * "%s" means the field is a string, "256" after ":" means the length of the string's character buffer..
 * "%f" means the field is a double precision number.
 * e.g.1 select the service's audio PID and name which index is 1.
 * \code
 *	int aud_pid;
 *	char name[256];
 *	int row = 1;
 * 	AM_DB_Select(pdb, "select aud1_pid,name from srv_table where db_id='1'", &row,
 *					  "%d, %s:256", (void*)&aud_pid, (void*)name);
 * \endcode
 * e.g.2 Select the services' video PID and name which service_id > 1.
 * \code
 *	int aud_pid[300];
 *	char name[300][256];
 *	int row = 300;
 *	AM_DB_Select(pdb, "select aud1_pid,name from srv_table where service_id > '1'", &row,
 *					  "%d, %s:256",(void*)&aud_pid, (void*)name);
 * \endcode
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_Select(sqlite3 *handle, const char *sql, int *max_row, const char *fmt, ...);

/**\brief Create tables in the database
 * \param [in] handle The database handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_CreateTables(sqlite3 *handle);

/**\brief Set the database's file path and database handle
 * \param [in] path The file path.
 * \param [in] defhandle Database handle created outside
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_Setup(char *path, sqlite3 *defhandle);

/**\brief Release the database
 * In multithread mode (SQLITE_THREADSAFE=2), each thread used database should
 * invoke this function to release the database.
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_UnSetup(void);

/**\brief Get the database handle
 * \param [out] handle Return the database handle
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_GetHandle(sqlite3 **handle);

/**\brief Get SQL statement object
 * If the statement is not used, allocate a new statement object.
 * Or return the old allocated statement object if it is used.
 * \param [out] stmt Return the statement object
 * \param [in]  name The name of the statement
 * \param [in]  sql SQL statement string
 * \param [in]  reset_if_exist Reallocate the statement object if it is already used
 * \retval AM_SUCCESS On success
 * \return Error code
 */
extern AM_ErrorCode_t AM_DB_GetSTMT(sqlite3_stmt **stmt, const char *name, const char *sql, int reset_if_exist);


#ifdef __cplusplus
}
#endif

#endif

