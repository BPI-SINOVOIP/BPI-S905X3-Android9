#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file am_db.c
 * \brief 数据库模块
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-20: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <am_debug.h>
#include <am_util.h>
#include "am_db_internal.h"
#include <am_db.h>
#include <am_mem.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
 
/*计算一个fileds所有字符串的总长度*/
#define DB_CAL_FIELD_STRING_LEN(_f, _s, _l)\
	AM_MACRO_BEGIN\
		int _i;\
		_l = 0;\
		for (_i=0; _i<(_s); _i++){\
			(_l) += strlen((_f)[_i]);\
			(_l) += 1;/* +1 存储 ",",最后一个field不加",",用于存储结束符*/\
		}\
	AM_MACRO_END

/*生成创建一个表的sql字符串*/
#define DB_GEN_TABLE_SQL_STRING(_t, _b)\
	AM_MACRO_BEGIN\
		int _i;\
		(_b)[0] = 0;\
		strcat(_b, "create table if not exists ");\
		strcat(_b, _t.table_name);\
		strcat(_b, "(");\
		for (_i=0; _i<_t.get_size(); _i++){\
			strcat(_b, _t.fields[_i]);\
			if (_i != (_t.get_size() - 1))\
				strcat(_b, ",");\
		}\
		strcat(_b, ")");\
	AM_MACRO_END

/**\brief 增加一个计算某个表的字段个数的函数*/
#define DEFINE_GET_FIELD_COUNT_FUNC(_f)\
static AM_INLINE int db_get_##_f##_cnt(void)\
{\
	return AM_ARRAY_SIZE(_f);\
}

/**\brief busy timeout in ms*/
#define DB_BUSY_TIMEOUT 5000

/****************************************************************************
 * Static data
 ***************************************************************************/
 
/**\brief network table 字段定义*/
static const char *net_fields[] = 
{
#include "network.fld"
};

/**\brief ts table 字段定义*/
static const char *ts_fields[] = 
{
#include "ts.fld"
};

/**\brief service table 字段定义*/
static const char *srv_fields[] = 
{
#include "service.fld"
};

/**\brief event table 字段定义*/
static const char *evt_fields[] = 
{
#include "event.fld"
};

/**\brief booking table 字段定义*/
static const char *booking_fields[] = 
{
#include "booking.fld"
};

/**\brief group table 字段定义*/
static const char *grp_fields[] = 
{
#include "group.fld"
};

/**\brief group map table 字段定义*/
static const char *grp_map_fields[] = 
{
#include "group_map.fld"
};

/**\brief dimension table 字段定义*/
static const char *dimension_fields[] = 
{
#include "dimension.fld"
};

/**\brief satellite parameter table 字段定义*/
static const char *sat_para_fields[] = 
{
#include "satellite_para.fld"
};

/**\brief region table */
static const char *region_fields[] = 
{
#include "region.fld"
};

/*各表自动获取字段数目函数定义*/
DEFINE_GET_FIELD_COUNT_FUNC(net_fields)
DEFINE_GET_FIELD_COUNT_FUNC(ts_fields)
DEFINE_GET_FIELD_COUNT_FUNC(srv_fields)
DEFINE_GET_FIELD_COUNT_FUNC(evt_fields)
DEFINE_GET_FIELD_COUNT_FUNC(booking_fields)
DEFINE_GET_FIELD_COUNT_FUNC(grp_fields)
DEFINE_GET_FIELD_COUNT_FUNC(grp_map_fields)
DEFINE_GET_FIELD_COUNT_FUNC(dimension_fields)
DEFINE_GET_FIELD_COUNT_FUNC(sat_para_fields)
DEFINE_GET_FIELD_COUNT_FUNC(region_fields)

/**\brief 所有的表定义*/
static AM_DB_Table_t db_tables[] = 
{
	{"net_table", net_fields, db_get_net_fields_cnt},
	{"ts_table",  ts_fields,  db_get_ts_fields_cnt},
	{"srv_table", srv_fields, db_get_srv_fields_cnt},
	{"evt_table", evt_fields, db_get_evt_fields_cnt},
	{"booking_table", booking_fields, db_get_booking_fields_cnt},
	{"grp_table", grp_fields, db_get_grp_fields_cnt},
	{"grp_map_table", grp_map_fields, db_get_grp_map_fields_cnt},
	{"dimension_table", dimension_fields, db_get_dimension_fields_cnt},
	{"sat_para_table", sat_para_fields, db_get_sat_para_fields_cnt},
	{"region_table", region_fields, db_get_region_fields_cnt},
};


/*list util -----------------------------------------------------------*/
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif

#define prefetch(x) (x)

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

struct list_head {
	struct list_head *next, *prev;
};
#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
#define list_for_each(pos, head) \
	for (pos = (head)->next; prefetch(pos->next), pos != (head); \
        	pos = pos->next)
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     prefetch(pos->member.next), &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))	     
/*---------------------------------------------------------------*/


typedef struct {
	struct list_head head;
	
	char *name;
	sqlite3_stmt *stmt;
} stmt_t;

typedef struct {
	/*db handle*/
	sqlite3 *db;

	/*stmt list*/
	struct list_head stmts;
}dbinfo_t;


static int multithread;

/**\brief 数据库文件路径*/
static char *dbpath;

/**\brief 默认数据库句柄*/
static dbinfo_t *dbdef;
//static sqlite3 *dbdef;
/**\brief 是否本地打开*/
static int dbdb_local;

/**\brief 各线程的db key*/
static pthread_key_t dbkey;
static int dbkey_enable;

/**\brief 全局锁*/
static pthread_mutex_t dblock;


/**\brief 分析数据类型列表，并生成相应结构*/
static AM_ErrorCode_t db_select_parse_types(const char *fmt, AM_DB_TableSelect_t *ps)
{
	char *p = (char*)fmt;
	char *td, *sd;
	
	assert(fmt && ps);
	
	ps->col = 0;
	while (*p)
	{
		/*去空格*/
		if (*p == ' ')
		{
			p++;
			continue;
		}
		/*分析'%'*/
		if (*p != '%')
		{
			AM_DEBUG(1, "DBase Select: fmt error, '%%' expected,but found '%c'", *p);
			return AM_DB_ERR_INVALID_PARAM;
		}
		/*column 是否超出*/
		if (ps->col >= MAX_SELECT_COLMUNS)
		{
			AM_DEBUG(1, "DBase Select: too many columns!");
			return AM_DB_ERR_INVALID_PARAM;
		}
		/*分析类型字符*/
		p++;
		switch (*p)
		{
			case 'd':
				ps->types[ps->col] = AM_DB_INT;
				ps->sizes[ps->col++] = sizeof(int);
				break;
			case 'f':
				ps->types[ps->col] = AM_DB_FLOAT;
				ps->sizes[ps->col++] = sizeof(double);
				break;
			case 's':
				ps->types[ps->col] = AM_DB_STRING;
				/*解析字符串数组长度*/
				if (*(++p) != ':')
				{
					AM_DEBUG(1, "DBase Select: fmt error, ':' expected,but found '%c'", *p);
					return AM_DB_ERR_INVALID_PARAM;
				}
				if (! isdigit(*(++p)))
				{
					AM_DEBUG(1, "DBase Select: fmt error, digit expected,but found '%c'", *p);
					return AM_DB_ERR_INVALID_PARAM;
				}

				/*取出数字*/
				td = p;
				while (*p && isdigit(*p))
					p++;
					
				sd = (char*)malloc(p - td + 1);
				if (sd == NULL)
				{
					AM_DEBUG(1, "DBase Select: no memory!");
					return AM_DB_ERR_NO_MEM;
				}
				strncpy(sd, td, p - td);
				sd[p - td] = '\0';
				/*AM_DEBUG(1, "Get string array size %d", atoi(sd));*/
				ps->sizes[ps->col] = atoi(sd);
				free(sd);
				
				/*没有指定字符串数组大小?*/
				if (ps->sizes[ps->col] <= 0)
				{
					AM_DEBUG(1, "DBase Select: invalid %%s array size %d", ps->sizes[ps->col - 1]);
					return AM_DB_ERR_INVALID_PARAM;
				}
				ps->col++;
				p--;
				break;
			default:
				AM_DEBUG(1, "DBase Select: fmt error, '[s][d][f]' expected,but found '%c'", *p);
				return AM_DB_ERR_INVALID_PARAM;
				break;
		}
		p++;
		/*查找分隔符','*/
		while (*p && *p == ' ')
			p++;
		
		if (*p && *p++ != ',')
		{
			AM_DEBUG(1, "DBase Select: fmt error, ',' expected,but found '%c'", *p);
			return AM_DB_ERR_INVALID_PARAM;
		}
	}

	/*没有指定任何数据类型*/
	if (ps->col == 0)
	{
		AM_DEBUG(1, "DBase Select: fmt error, no types specified");
		return AM_DB_ERR_INVALID_PARAM;
	}

	return AM_SUCCESS;
}

/**\brief 根据表的fileds定义计算并分配一个合适大小的buf用于存储sql字符串*/
static char *db_get_sql_string_buf(void)
{
	int temp, max, i;

	max = 0;
	for (i=0; i<(int)AM_ARRAY_SIZE(db_tables); i++)
	{
		DB_CAL_FIELD_STRING_LEN(db_tables[i].fields, db_tables[i].get_size(), temp);
		temp += strlen(db_tables[i].table_name);
		max = AM_MAX(max, temp);
	}

	max += strlen("create table if not exsits ()");

	/*AM_DEBUG(1, "buf len %d", max);*/
	return (char*)malloc(max);
}

static sqlite3 *db_open_db(const char *path)
{
	sqlite3 *hdb = NULL;
	/*打开数据库*/
	AM_DEBUG(1, "Dbase: Opening DBase[%s] for thread[%ld]...", path, pthread_self());
	if (sqlite3_open(path, &hdb) != SQLITE_OK)
	{
		AM_DEBUG(1, "DBase:Cannot open DB %s!", path);
		return NULL;
	}
	if (sqlite3_busy_timeout(hdb, DB_BUSY_TIMEOUT) != SQLITE_OK)
	{
		AM_DEBUG(1, "DBase: [Warning] Set DB busy timeout(%dms) failed!", DB_BUSY_TIMEOUT);
	}
	return hdb;
}

static dbinfo_t * db_get_dbinfo_multithread(const char *path)
{
	dbinfo_t *db = NULL;
	sqlite3 *hdb = NULL;

	db = (dbinfo_t *)pthread_getspecific(dbkey);
	if(!db)
	{
		int rc;
		
		hdb = db_open_db(path);
		if(!hdb)
			return NULL;

		db = malloc(sizeof(dbinfo_t));
		assert(db);
		memset(db, 0, sizeof(dbinfo_t));
		db->db = hdb;
		INIT_LIST_HEAD(&db->stmts);
		rc = pthread_setspecific(dbkey, (void*)db);
		if(rc)
		{
			AM_DEBUG(1, "DBase: key set: %s", strerror(rc));
			sqlite3_close(hdb);
			free(db);
			return NULL;
		}
	}
	return db;
}

static AM_ErrorCode_t db_get_stmt(sqlite3_stmt **stmt, dbinfo_t *dbinfo, const char *name, const char *sql, int reset_if_exist)
{
	stmt_t *st = NULL;
	AM_ErrorCode_t err = AM_SUCCESS;

	*stmt = NULL;

	/*looking for stmt exsits already.*/
	list_for_each_entry(st, &dbinfo->stmts, head)
	{
		if(0 == strcmp(st->name, name)) {
			if(reset_if_exist)
			{
				sqlite3_stmt *sttmp;
				if(sqlite3_prepare(dbinfo->db, sql, strlen(sql), &sttmp, NULL) == SQLITE_OK)
					err = AM_FAILURE;
				else
				{
					sqlite3_finalize(st->stmt);
					st->stmt = sttmp;
				}
			}
			*stmt = st->stmt;
			return err;
		}
	}

	/*not found, create one*/
	AM_DEBUG(1, "Dbase: stmt[%s] not exist, prepare a new one.", name);
	st = malloc(sizeof(stmt_t));
	assert(st);
	memset(st, 0, sizeof(stmt_t));
	if(sqlite3_prepare(dbinfo->db, sql, strlen(sql), &st->stmt, NULL) != SQLITE_OK)
	{
		free(st);
		st = NULL;
		return AM_FAILURE;
	}
	
	if(name)
		st->name = strdup(name);

	list_add_tail(&st->head, &dbinfo->stmts);

	*stmt = st->stmt;

	return err;
}

static int db_put_stmt(sqlite3_stmt *sqst, dbinfo_t *dbinfo)
{
	stmt_t *pos, *ptmp;

	list_for_each_entry_safe(pos, ptmp, &dbinfo->stmts, head)
	{
		if(pos->stmt == sqst) {
			sqlite3_finalize(pos->stmt);
			if(pos->name)
				free(pos->name);
			list_del(&pos->head);
			free(pos);
			return 0;
		}
	}
	return -1;
}

static void db_clear_dbinfo(dbinfo_t *dbinfo, int freedb)
{
	stmt_t *pos, *ptmp;
	list_for_each_entry_safe(pos, ptmp, &dbinfo->stmts, head)
	{
		sqlite3_finalize(pos->stmt);
		if(pos->name)
			free(pos->name);
		list_del(&pos->head);
		free(pos);
	}

	if(freedb)
		AM_DB_Quit(dbinfo->db);
}

static void dbkey_destructor(void *p)
{
	AM_DEBUG(1, "Dbase: Closing DBase[%p] for thread[%ld]...", p, pthread_self());
	if(p)
	{
		db_clear_dbinfo((dbinfo_t*)p, 1);
		free(p);
	}
}


/****************************************************************************
 * API functions
 ***************************************************************************/
 
/**\brief 初始化数据库模块
 * \param [out] handle 返回数据库操作句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_Init(sqlite3 **handle)
{
	assert(handle);
	
	/*打开数据库*/
	AM_DEBUG(1, "Opening DBase...");
	if (sqlite3_open(":memory:", handle) != SQLITE_OK)
	{
		AM_DEBUG(1, "DBase:Cannot open DB in memory!");
		return AM_DB_ERR_OPEN_DB_FAILED;
	}

	AM_TRY(AM_DB_CreateTables(*handle));
	
	AM_DEBUG(1, "DBase handle %p", *handle);
	return AM_SUCCESS;
}

/**\brief 初始化数据库模块
 * \param [in]  path   数据库文件路径
 * \param [out] handle 返回数据库操作句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_Init2(char *path, sqlite3 **handle)
{
	assert(handle);

	*handle = NULL;
	
	if(!path)
		return AM_DB_Init(handle);

	/*打开数据库*/
	AM_DEBUG(1, "Opening DBase [%s]...", path);
	if (sqlite3_open(path, handle) != SQLITE_OK)
	{
		AM_DEBUG(1, "DBase:Cannot open DB %s!", path);
		return AM_DB_ERR_OPEN_DB_FAILED;
	}

	AM_TRY(AM_DB_CreateTables(*handle));
	
	AM_DEBUG(1, "DBase handle %p", *handle);

	return AM_SUCCESS;
}

/**\brief 终止数据库模块
 * param [in] handle AM_DB_Init()中返回的handle
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_Quit(sqlite3 *handle)
{
	if (handle != NULL)
	{
		AM_DEBUG(1, "Closing DBase(handle=%p)...", handle);
		sqlite3_close(handle);
	}

	return AM_SUCCESS;
}

/**\brief 从数据库取数据
 * param [in] handle AM_DB_Init()中返回的handle
 * param [in] sql 用于查询的sql语句,只支持select语句
 * param [in out]max_row  输入指定最大组数，输出返回查询到的组数
 * param [in] fmt 需要查询的数据类型字符串，格式形如"%d, %s:64, %f",
 *				  其中,顺序必须与select语句中的要查询的字段对应,%d对应int, %s对应字符串,:后指定字符串buf大小,%f对应double
 * param 可变参数 参数个数与要查询的字段个数相同，参数类型均为void *
 *
 * e.g.1 从数据库中查询service的db_id为1的service的aud_pid和name(单组记录)
 *	int aud_pid;
 	char name[256];
 	int row = 1;
  	AM_DB_Select(pdb, "select aud1_pid,name from srv_table where db_id='1'", &row,
 					  "%d, %s:256", (void*)&aud_pid, (void*)name);
 *
 * e.g.2 从数据库中查找service_id大于1的所有service的vid_pid和name(多组记录)
 *	int aud_pid[300];
 	char name[300][256];
 	int row = 300;
 	AM_DB_Select(pdb, "select aud1_pid,name from srv_table where service_id > '1'", &row,
 					  "%d, %s:256",(void*)&aud_pid, (void*)name);
 *
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_Select(sqlite3 *handle, const char *sql, int *max_row, const char *fmt, ...)
{
	AM_DB_TableSelect_t ts;
	va_list ap;
	void *vp;
	int va_cnt = 0;
	int col, row, i, j, cur;
	char **db_result = NULL;
	char *errmsg;
	
	assert(handle && sql && fmt && max_row);

	if (strncmp(sql, "select", 6) || *max_row <= 0)
		return AM_DB_ERR_INVALID_PARAM;

	/*取数据类型列表*/
	AM_TRY(db_select_parse_types(fmt, &ts));

	/*分析存储列表*/
	va_start(ap, fmt);
	while ((vp = va_arg(ap, void*)) != NULL && va_cnt < ts.col)
	{
		ts.dat[va_cnt++] = vp;
	}
	va_end(ap);

	/*检查类型列表和参数列表个数是否匹配*/
	if (va_cnt != ts.col)
	{
		AM_DEBUG(1, "DBase Select:fmt error, dismatch the arg list");
		return AM_DB_ERR_INVALID_PARAM;
	}

	/*从数据库查询数据*/
	if (sqlite3_get_table(handle, sql, &db_result, &row, &col, &errmsg) != SQLITE_OK)
	{
		AM_DEBUG(1, "DBase Select:get table failed, reason [%s], sql [%s]", errmsg, sql);
		if (db_result != NULL)
			sqlite3_free_table(db_result);
		return AM_DB_ERR_SELECT_FAILED;
	}

	/*sql语句中指定的column数与fmt字符串中指定的个数不匹配*/
	if (col != ts.col && col != 0)
	{
		AM_DEBUG(1, "DBase Select:sql columns != columns described in fmt");
		sqlite3_free_table(db_result);
		return AM_DB_ERR_INVALID_PARAM;
	}

	if (row < *max_row)
		*max_row = row;

	row = *max_row;

	/*按用户指定的数据类型存储数据*/
	cur = col;
	for (i=0; i<row; i++)
	{
		for (j=0; j<col; j++)
		{
			switch (ts.types[j])
			{
				case AM_DB_INT:
					*(int*)((char*)ts.dat[j] + i * ts.sizes[j]) = db_result[cur] ? atoi(db_result[cur]) : 0;
					break;
				case AM_DB_FLOAT:
					*(double*)((char*)ts.dat[j] + i * ts.sizes[j]) = db_result[cur] ? atof(db_result[cur]) : 0.0;
					break;
				case AM_DB_STRING:
					if (db_result[cur])
					{
						strncpy((char*)ts.dat[j] + i * ts.sizes[j], db_result[cur], ts.sizes[j] - 1);
						*((char*)ts.dat[j] + (i + 1) * ts.sizes[j] - 1) = '\0';
					}
					else
					{
						*((char*)ts.dat[j] + i * ts.sizes[j]) = '\0';
					}
					break;
				default:
					/*This will never occur*/
					break;
			}

			cur++;
		}
	}

	sqlite3_free_table(db_result);
	return AM_SUCCESS;
}
	
/**\brief 在一个指定的数据库中创建表
 * param [in] handle sqlite3数据库句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_CreateTables(sqlite3 *handle)
{
	char *buf, *errmsg;
	int i;

	assert(handle);
	
	/*获取存储sql字符串的buf*/
	buf = db_get_sql_string_buf();
	if (buf == NULL)
	{
		AM_DEBUG(1, "DBase:Cannot create DB tables, no enough memory");
		return AM_DB_ERR_NO_MEM;
	}
	
	/*依次创建表*/
	for (i=0; i<(int)AM_ARRAY_SIZE(db_tables); i++)
	{
		AM_DEBUG(1, "Creating table [%s]", db_tables[i].table_name);
	
		/*生成sql 字符串*/
		DB_GEN_TABLE_SQL_STRING(db_tables[i], buf);

		AM_DEBUG(2, "sql string:[%s]", buf);
		/*创建表*/
		if (sqlite3_exec(handle, buf, NULL, NULL, &errmsg) != SQLITE_OK)
		{
			AM_DEBUG(1, "DBase:Cannot create table, reason [%s]", errmsg);
		}
	}

	free(buf);

	return AM_SUCCESS;
}

/**\brief 设置数据库文件路径和默认数据库句柄
 * param [in] path 数据库文件路径
 * param [in] defhandle 默认数据库句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_Setup(char *path, sqlite3 *defhandle)
{
	int ret;
	pthread_mutexattr_t mta;	

	assert(path || defhandle);
	//showboz, sqlite3_threadsafe() only get complite-time thread flags,can not get run-time thread flags.
	//tvapi(tvserver) run-time thread flags is serial.so del it	
	multithread = 
		(sqlite3_threadsafe()==SQLITE_CONFIG_MULTITHREAD)?1:0;
	//multithread = 0;//tvserver use serial
	if(dbpath)
	{
		AM_DEBUG(1, "DBase:Setup Already");
		return AM_FAILURE;
	}
	
	if(multithread && (!path))
	{
		AM_DEBUG(1, "DBase:Database file path is necessary as sqlite3 works with MULTITHREAD configuration");
		return AM_FAILURE;
	}

	if(multithread)
	{
		ret = pthread_key_create(&dbkey, dbkey_destructor);
		if(ret!=0)
		{
			AM_DEBUG(1, "DBase key create: %s", strerror(ret));
			return AM_FAILURE;
		}
		dbkey_enable =  1;
	}
	else
	{
		dbdef = malloc(sizeof(dbinfo_t));
		assert(dbdef);
		memset(dbdef, 0, sizeof(dbinfo_t));
		dbdef->db = defhandle;
		INIT_LIST_HEAD(&dbdef->stmts);
	}
			
	pthread_mutexattr_init(&mta);
	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&dblock, &mta);
	pthread_mutexattr_destroy(&mta);

	if(path)
		dbpath = strdup(path);

	AM_DEBUG(1, "DB SETUP : path[%s], defhandle[%p], multithread[%d]", dbpath, dbdef, multithread);
	
	return AM_SUCCESS;
}

/**\brief 释放数据库配置
		  注意：在sqlite3多线程配置状态下（SQLITE_THREADSAFE=2），
		        需在所有包含数据库操作的线程退出后执行
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_UnSetup(void)
{
	pthread_mutex_lock(&dblock);

	if(dbpath)
	{
		free(dbpath);
		dbpath = NULL;
	}

	if(dbkey_enable)
	{
		pthread_key_delete(dbkey);
		dbkey_enable = 0;
	}

	if(dbdef)
	{
		db_clear_dbinfo(dbdef, dbdb_local?1:0);
		free(dbdef);
		dbdef = NULL;
		dbdb_local = 0;
	}

	pthread_mutex_unlock(&dblock);

	return AM_SUCCESS;
}

/**\brief 获得sqlite3数据库句柄
 * param [out] handle 数据库句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_GetHandle(sqlite3 **handle)
{
	dbinfo_t *db = NULL;
	sqlite3 *hdb = NULL;
	AM_ErrorCode_t err = AM_SUCCESS;
	char *path = dbpath;
	
	assert(handle);

	*handle = NULL;
	
	pthread_mutex_lock(&dblock);

	if(multithread)
	{
		db = db_get_dbinfo_multithread(path);
		if(db)
			hdb = db->db;
	}
	else
	{
		if(!dbdef->db)
		{
			AM_DEBUG(1, "Dbase: Opening DBase[%s] for default...", path);
			dbdef->db = db_open_db(path);
			if (dbdef->db)
				dbdb_local = 1;
			else
				err = AM_DB_ERR_OPEN_DB_FAILED;
		}
		hdb = dbdef->db;
	}

	pthread_mutex_unlock(&dblock);

	*handle = hdb;
	return err;
}

/**\brief 获得sqlite3 SQL Statement句柄
 * param [out] stmt SQL Statement句柄
 * param [in]  name statement名称，方便再次获取
 * param [in]  sql  生成statement的sql语句
 *                  如果statement不存在，则使用此sql语句生成statement
 * param [in]  reset_if_exist 重新生成statement
 *                  如果statement存在，则重新生成该statement。
 *                  当数据库文件改变导致statement出错时使用
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_GetSTMT(sqlite3_stmt **stmt, const char *name, const char *sql, int reset_if_exist)
{
	dbinfo_t *db = NULL;
	sqlite3 *hdb = NULL;
	sqlite3_stmt *sqst = NULL;
	AM_ErrorCode_t err = AM_FAILURE;
	
	assert(stmt && name);

	*stmt = NULL;
	
	pthread_mutex_lock(&dblock);

	if(multithread)
	{
		db = db_get_dbinfo_multithread(dbpath);
		if(db)
			err = db_get_stmt(&sqst, db, name, sql, reset_if_exist);
	}
	else
	{
		err = db_get_stmt(&sqst, dbdef, name, sql, reset_if_exist);
	}

	pthread_mutex_unlock(&dblock);

	*stmt = sqst;

	return err;
}

/**\brief 释放sqlite3数据库句柄
 *        只能用于释放通过AM_DB_GetHandle()获得的handle
 *        sqlite库编译配置为多线程时，必须与对应的AM_DB_GetHandle()在同一线程中使用
 *        -==非公开函数，谨慎使用==-
 * param [in] handle 数据库句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_PutHandle(sqlite3 *handle)
{
	dbinfo_t *db = NULL;
	AM_ErrorCode_t err = AM_SUCCESS;
	
	assert(handle);

	UNUSED(handle);

	pthread_mutex_lock(&dblock);

	if(multithread)
	{
		db = (dbinfo_t *)pthread_getspecific(dbkey);
		if(db)
			db_clear_dbinfo(db, 1);
		else
			err = AM_DB_ERR_INVALID_PARAM;
	}
	else
	{
		db_clear_dbinfo(dbdef, dbdb_local?1:0);
		dbdb_local = 0;
	}

	pthread_mutex_unlock(&dblock);

	return err;
}

/**\brief 释放sqlite3 SQL Statement句柄
 *        只能用于释放通过AM_DB_GetSTMT()获得的handle
 *        sqlite库编译配置为多线程时，必须与对应的AM_DB_GetSTMT()在同一线程中使用
 *        -==非公开函数，谨慎使用==-
 * param [in] stmt Statement句柄
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码(见am_db.h)
 */
AM_ErrorCode_t AM_DB_PutSTMT(sqlite3_stmt *stmt)
{
	dbinfo_t *db = NULL;
	sqlite3 *hdb = NULL;
	sqlite3_stmt *sqst = NULL;
	int err = -1;
	
	assert(stmt);

	pthread_mutex_lock(&dblock);

	if(multithread)
	{
		db = (dbinfo_t *)pthread_getspecific(dbkey);
		if(db)
		{
			err = db_put_stmt(stmt, db);
		}
	}
	else
	{
		err = db_put_stmt(stmt, dbdef);
	}

	pthread_mutex_unlock(&dblock);

	return (err==0)? AM_SUCCESS : AM_FAILURE;
}

