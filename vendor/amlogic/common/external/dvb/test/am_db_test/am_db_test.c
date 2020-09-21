#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package. 
 * Description:
 */
/**\file am_db_test.c
 * \brief DBase测试程序
 *
 * \author Xia Lei Peng <leipeng.xia@amlogic.com>
 * \date 2010-10-20: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 0

#include <am_debug.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "am_db.h"


/****************************************************************************
 * Macro definitions
 ***************************************************************************/

int print_result_callback(void *param, int col, char **values, char **names)
{
	int i;

	for (i=0; i<col; i++)
	{
		printf("%s\t", names[i]);
	}
	printf("\n");

	for (i=0; i<col; i++)
	{
		printf("%s\t", values[i]);
	}
	printf("\n");
	
	return 0;
}

static void dbselect_test(sqlite3 *pdb)
{
	int nid[3];
	char name[3][64];
	int row = 5;
	int i;
	int db_id;
	int freq;

	if (AM_DB_Select(pdb, "select network_id,name from net_table where db_id > '0'", &row,
					  "%d, %s:64 ", (void*)nid, (void*)name) == AM_SUCCESS)
	{
		for (i=0; i<row; i++)
		{
			printf("network id is %d, name is %s\n", nid[i], name[i]);
		}
	}
	
	row = 1;
	if (AM_DB_Select(pdb, "select freq from ts_table where ts_id='100'", &row,
					  "%d", (void*)&freq) == AM_SUCCESS)
		printf("freq is %d\n", freq);
}
 
int main(int argc, char **argv)
{
	sqlite3 *pdb;
	int go = 1;
	int rc;
	char buf[256];
	char *errmsg;
	int (*cb)(void*, int, char**, char**);

	AM_TRY(AM_DB_Init(&pdb));
	sqlite3_exec(pdb, "insert into net_table(name, network_id) values('network1', '1111')", NULL, NULL, &errmsg);
	sqlite3_exec(pdb, "insert into net_table(name, network_id) values('network2', '1112')", NULL, NULL, &errmsg);
	sqlite3_exec(pdb, "insert into net_table(name, network_id) values('network3', '1113')", NULL, NULL, &errmsg);
	sqlite3_exec(pdb, "insert into ts_table(db_net_id, ts_id, freq) values('2', '100', '474000000')", NULL, NULL, &errmsg);
	
	do{
		printf("Enter your sql cmd:\n");
		if (gets(buf))
		{
			cb = NULL;
			if (!strncmp(buf, "select", 6))
			{
				cb = print_result_callback;
			}
			else if (!strncmp(buf, "dbselect", 8))
			{
				dbselect_test(pdb);
				continue;
			}
			else if (!strncmp(buf, "quit", 4))
			{
				go=0;
				break;
			}

			if (sqlite3_exec(pdb, buf, cb, NULL, &errmsg) != SQLITE_OK)
			{
				printf("Opration failed, reseaon[%s]\n", errmsg);
			}
			else
			{
				printf("Operation OK!\n\n");
			}
		}
	}while (go);

	AM_DB_Quit(pdb);
	
	return 0;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

