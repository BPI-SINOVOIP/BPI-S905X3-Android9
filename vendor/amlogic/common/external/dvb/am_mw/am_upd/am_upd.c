/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#define AM_DEBUG_LEVEL 1

#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "am_time.h"
#include "am_dmx.h"
#include "am_si.h"
#include "am_debug.h"
#include "am_upd.h"
#include "am_cond.h"


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
static inline int list_is_singular(const struct list_head *head)
{
	return !list_empty(head) && (head->next == head->prev);
}
static inline void list_move(struct list_head *list, struct list_head *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)
#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)	
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

/*table util--------------------------------------------------------*/
enum {
	TABLE_DONE,
	TABLE_TIMEOUT
};

typedef struct {
	struct list_head head;

	unsigned char sec_num;
	unsigned char last_sec_num;
	unsigned char *buf;
	unsigned int len;
}table_secbuf_t;

struct table_s;
typedef struct table_s table_t;

struct table_s{
	int type;

	int dmx;

	AM_SI_Handle_t si;

	int fid;

	struct dmx_sct_filter_params filter;
	int buf_size;
	uint16_t	ext;
	uint8_t		ver;
	uint8_t		sec;
	uint8_t		last;
	uint8_t		mask[32];
	uint16_t    sec_len_max;
	
	uint8_t     force_collect;
	uint16_t    user1;
	uint16_t    user2;

	int status;
#define table_stopped 0
#define table_started 1

	int monitor;
	struct timespec end_time;
	int timeout;
	int data_arrive_time;
	int repeat_distance;

	pthread_mutex_t     lock;
	struct list_head secbufs;

	int (*done)(int stat, table_t *table);
	void *args;

	int (*user_handler)(int dev_no, int fid, const uint8_t *data, int len, void *user_data);
};

typedef struct 
{
	uint8_t		table_id;			/**< table_id*/
	uint8_t		syntax_indicator;		/**< section_syntax_indicator*/
	uint8_t		private_indicator;		/**< private_indicator*/
	uint16_t	length;				/**< section_length*/

	union {
		struct {
			uint16_t	extension;			/**< table_id_extension*/
									/**< transport_stream_id for a
						                                             PAT section */
			uint8_t		version;			/**< version_number*/
			uint8_t		cur_next_indicator;		/**< current_next_indicator*/
		}u1;
		struct {
			uint16_t    user1;
			uint16_t    user2;
		}u2;
	}u;
	uint8_t		sec_num;			/**< section_number*/
	uint8_t		last_sec_num;			/**< last_section_number*/
}section_header_t;


#define LIST_FOR_EACH(l, v) for ((v)=(l); (v)!=NULL; (v)=(v)->p_next)

#define ADD_TO_LIST(_t, _l)\
	AM_MACRO_BEGIN\
		(_t)->p_next = (_l);\
		_l = _t;\
	AM_MACRO_END

#define RELEASE_TABLE_FROM_LIST(_t, _l, _table)\
	AM_MACRO_BEGIN\
		_t *tmp, *next;\
		tmp = (_l);\
		while (tmp){\
			next = tmp->p_next;\
			AM_SI_ReleaseSection((_table)->si, tmp->i_table_id, (void*)tmp);\
			tmp = next;\
		}\
		(_l) = NULL;\
	AM_MACRO_END


#define BIT_MASK(b) (1 << ((b) % 8))
#define BIT_SLOT(b) ((b) / 8)
#define BIT_SET(a, b) ((a)[BIT_SLOT(b)] |= BIT_MASK(b))
#define BIT_CLEAR(a, b) ((a)[BIT_SLOT(b)] &= ~BIT_MASK(b))
#define BIT_TEST(a, b) ((a)[BIT_SLOT(b)] & BIT_MASK(b))

static int table_open(table_t *table);
static int table_start(table_t *table);
static int table_stop(table_t *table);
static int table_close(table_t *table);


static inline int tablectl_test_recved(table_t *table, section_header_t *header)
{	
	return header->syntax_indicator?
		((table->ext == header->u.u1.extension) && 
		(table->ver == header->u.u1.version) && 
		(table->last == header->last_sec_num) && 
		!BIT_TEST(table->mask, header->sec_num) )
		:
		((table->user1 == header->u.u2.user1) && 
		(table->user2 == header->u.u2.user2) && 
		(table->last == header->last_sec_num) && 
		!BIT_TEST(table->mask, header->sec_num) &&
		(table->ver != 0xff));
}
static inline int tablectl_test_complete(table_t *table)
{
	static uint8_t test_array[32] = {0};
	return !((table->ver != 0xff) &&
		memcmp(table->mask, test_array, sizeof(test_array)));
}
static int tablectl_test_section(table_t *table, section_header_t *header)
{
	int changed;
	/*changed, reset ctl*/
	if(header->syntax_indicator)
		changed = (table->ver != header->u.u1.version ||\
			(table->ext != header->u.u1.extension) ||\
			table->last != header->last_sec_num)? 1 : 0;
	else
		changed = (table->user1 != header->u.u2.user1 ||\
			(table->user2 != header->u.u2.user2) ||\
			table->last != header->last_sec_num)? 1 : 0;

	if (table->ver != 0xff && changed)
	{
		memset(table->mask, 0, sizeof(table->mask));
		table->ver = 0xff;
		return -1;
	}
	return 0;
}
static int tablectl_mark_section(table_t *table, section_header_t *header)
{
	if (table->ver == 0xff)
	{
		int i;
		if(header->syntax_indicator) {
			table->ver = header->u.u1.version;	
			table->ext = header->u.u1.extension;
		} else {
			table->ver = 0;
			table->user1 = header->u.u2.user1;
			table->user2 = header->u.u2.user2;
		}
		table->last = header->last_sec_num;
		for (i=0; i<(table->last+1); i++)
			BIT_SET(table->mask, i);
	}

	BIT_CLEAR(table->mask, header->sec_num);

	return AM_SUCCESS;
}

static int get_section_header(uint8_t *buf, uint16_t len, section_header_t *sec_header)
{
	if (len < 8)
		return -1;

	assert(buf && sec_header);

	sec_header->table_id = buf[0];
	sec_header->syntax_indicator = (buf[1] & 0x80) >> 7;
	sec_header->private_indicator = (buf[1] & 0x40) >> 6;
	sec_header->length = (((uint16_t)(buf[1] & 0x0f)) << 8) | (uint16_t)buf[2];
	if(sec_header->syntax_indicator) {
		sec_header->u.u1.extension = ((uint16_t)buf[3] << 8) | (uint16_t)buf[4];
		sec_header->u.u1.version = (buf[5] & 0x3e) >> 1;
		sec_header->u.u1.cur_next_indicator = buf[5] & 0x1;
	} else {
		sec_header->u.u2.user1 = ((uint16_t)buf[3] << 4) | ((uint16_t)buf[4]>>4);
		sec_header->u.u2.user2 = ((uint16_t)(buf[4]&0xf) << 8) | (uint16_t)buf[5];
	}
	sec_header->sec_num = buf[6];
	sec_header->last_sec_num = buf[7];

	return 0;
}

static void section_handler(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	table_t *table = (table_t *)user_data;
	section_header_t header;
	table_secbuf_t *secbuf;
	
	if (!table || !data || (table->fid==-1))
		return;

	pthread_mutex_lock(&table->lock);

	if(table->user_handler)
	{
		if(table->user_handler(dev_no, fid, data, len, user_data))
			return;
	}

	if (get_section_header((uint8_t*)data, len, &header) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Scan: section header error");
		goto end;
	}

	//log_x(">>> %d of %d len:%d\n", header.sec_num, header.last_sec_num, header.length);
	//printf(">>> %d of %d len:%d\n", header.sec_num, header.last_sec_num, header.length);
	//printf(">>> syntax:%d part: %d of %d\n", header.syntax_indicator, header.u.u2.user1, header.u.u2.user2);
	if(tablectl_test_section(table, &header)!=0)
	{
		AM_DEBUG(2, "ver changed.\n");
		table_secbuf_t *sec, *n;
		list_for_each_entry_safe(sec, n, &table->secbufs, head)
		{
			list_del(&sec->head);
			free(sec->buf);
			free(sec);
		}
	}

	if(table->sec_len_max < header.length)
		table->sec_len_max = header.length;

	if(!tablectl_test_recved(table, &header))
	{
		//printf("add sec\n");
		secbuf = malloc(sizeof(table_secbuf_t));
		if(!secbuf)
			goto end;

		secbuf->buf = malloc(len);
		if(!secbuf->buf)
		{
			free(secbuf);
			goto end;
		}
		memcpy(secbuf->buf, data, len);
		secbuf->len = len;
		secbuf->sec_num = header.sec_num;
		secbuf->last_sec_num = header.last_sec_num;
		list_add(&secbuf->head, &table->secbufs);

		tablectl_mark_section(table, &header);
	}

	if (tablectl_test_complete(table))
	{
		AM_DEBUG(4, "table[%d] Done!", table->type);
		table->done(TABLE_DONE, table);
	}
end:
	pthread_mutex_unlock(&table->lock);

	return;
}

static int table_open(table_t *table)
{
	pthread_mutexattr_t mta;

	assert(table);

	if (table->fid != -1)
		return 0;

	pthread_mutexattr_init(&mta);
//	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&table->lock, &mta);
	pthread_mutexattr_destroy(&mta);

	INIT_LIST_HEAD(&table->secbufs);

	if(AM_SI_Create(&table->si) != AM_SUCCESS)
		return -1;

	if (AM_DMX_AllocateFilter(table->dmx, &table->fid) != AM_SUCCESS)
		goto fail;

	if(AM_DMX_SetCallback(table->dmx, table->fid, section_handler, (void*)table) != AM_SUCCESS)
		goto fail;

	table->status = table_stopped;
	return 0;

fail:
	if(table->fid!=-1)
	{
		AM_DMX_FreeFilter(table->dmx, table->fid);
		table->fid = -1;
	}
	if(table->si)
	{
		AM_SI_Destroy(table->si);
		table->si = 0;
	}

	return -1;
}

static int table_start(table_t *table)
{
	table_secbuf_t *secbuf, *n;

	assert(table);

	if (table->fid == -1)
		return -1;

	table_stop(table);

	list_for_each_entry_safe(secbuf, n, &table->secbufs, head)
	{
		list_del(&secbuf->head);
		free(secbuf->buf);
		free(secbuf);
	}

	AM_TIME_GetTimeSpecTimeout(table->timeout, &table->end_time);

	if(AM_DMX_SetSecFilter(table->dmx, table->fid, &table->filter) != AM_SUCCESS)
		return -1;
	if(table->buf_size 
		&& (AM_DMX_SetBufferSize(table->dmx, table->fid, table->buf_size) != AM_SUCCESS))
		return -1;
	if(AM_DMX_StartFilter(table->dmx, table->fid) != AM_SUCCESS)
		return -1;

	table->status = table_started;

	return 0;	
}

static int table_stop(table_t *table)
{
	assert(table);

	if (table->fid == -1)
		return -1;

	AM_DMX_StopFilter(table->dmx, table->fid);
	table->status = table_stopped;
	table->ver = 0xff;

	AM_DMX_Sync(table->dmx);

	return 0;
}

static int table_close(table_t *table)
{
	table_secbuf_t *secbuf, *n;

	assert(table);

	if (table->fid == -1)
		return 0;

	table_stop(table);

	AM_DMX_FreeFilter(table->dmx, table->fid);
	table->fid = -1;

	list_for_each_entry_safe(secbuf, n, &table->secbufs, head)
	{
		list_del(&secbuf->head);
		free(secbuf->buf);
		free(secbuf);
	}

	if(table->si)
	{
		AM_SI_Destroy(table->si);
		table->si = 0;
	}
	
	pthread_mutex_destroy(&table->lock);

	return 0;
}
/*---------------------------------------------------------------*/

typedef struct tsupd_s {
	unsigned int magic;

	pthread_mutex_t     lock;
	pthread_cond_t      cond;
	pthread_t          	thread;
	int                 quit;

	int (*cb)(unsigned char* pdata, unsigned int len, void *user);
	void* cb_args;

	int dmx;
	
	int stat;
#define stat_idle    0
#define stat_stopped 1
#define stat_started 2
	
	int tsevt;
#define tsevt_new_table       0x01
#define tsevt_mon_table       0x10
#define tsevt_mon_table_start 0x100

	unsigned short pid;
	unsigned char tid;
	unsigned short ext;

	table_t table;
	struct timespec table_next_time;
	unsigned int table_timeout;
	unsigned int table_check_interval;

	unsigned char    table_ver;

}tsupd_t;

typedef struct tsupdm_s tsupdm_t;

struct tsupdm_s {

	tsupd_t upd;

#define _UPDM_MAGIC	0x13141615

	int use_ext_nit;

	dvbpsi_nit_t *psinit;
	dvbpsi_nit_t *psinit_monitor;
};

typedef struct tsupdd_s tsupdd_t;

struct tsupdd_s {

	tsupd_t upd;

#define _UPDD_MAGIC	0x13141516

	void *pdl_data;
	unsigned int dl_data_len;
};

static tsupdm_t tsupdm;


static int nit_done(int status, table_t *table)
{
	tsupdm_t *updm = (tsupdm_t *)table->args;

	if(status==TABLE_DONE)
	{
		table_secbuf_t *secbuf;
		
		list_for_each_entry(secbuf, &table->secbufs, head)
		{
			dvbpsi_nit_t *p_table;

			if (AM_SI_DecodeSection(table->si, table->filter.pid, (uint8_t*)secbuf->buf, secbuf->len, (void**)&p_table) == AM_SUCCESS)
			{
				p_table->p_next = NULL;
				if(table->monitor)
					ADD_TO_LIST(p_table, updm->psinit_monitor);
				else
					ADD_TO_LIST(p_table, updm->psinit);
			}else
			{
				AM_DEBUG(1, "Decode Section failed");
			}
		}

	}
	else if(status==TABLE_TIMEOUT)
	{
		if(table->monitor)
			AM_DEBUG(3, "nit monitor timeout!");
		else
			AM_DEBUG(1, "nit timeout!");

		table->sec_len_max = 0;
	}

	updm->upd.tsevt |= tsevt_new_table;
	pthread_cond_signal(&updm->upd.cond);

	table_stop(table);

	return 0;
}

static int updm_restart_nit(tsupdm_t *updm, int monitor)
{
	table_t *nit = &updm->upd.table;

	updm->upd.tsevt &= ~(tsevt_new_table|tsevt_mon_table|tsevt_mon_table_start);

	if(!monitor)
	{
		AM_DEBUG(1, "-->|start table nit: monitor[%d] pid[%x] tid[%x] ext[%x]", 
			monitor, updm->upd.pid, updm->upd.tid, updm->upd.ext);

		updm->upd.table_ver = 0xff;
		
		table_stop(nit);
		RELEASE_TABLE_FROM_LIST(dvbpsi_nit_t, updm->psinit, nit);
		RELEASE_TABLE_FROM_LIST(dvbpsi_nit_t, updm->psinit_monitor, nit);
		table_close(nit);

		memset(nit, 0, sizeof(table_t));
		
		/*!!fid==-1 when closed!!*/
		nit->fid = -1;
		nit->type = 3;
		
		nit->dmx = updm->upd.dmx;
		nit->done = nit_done;
		nit->args = updm;
		
		nit->filter.pid = updm->upd.pid;
		nit->filter.filter.filter[0] = updm->upd.tid;
		nit->filter.filter.mask[0] = 0xff;

		/*Current next indicator must be 1*/
		nit->filter.filter.filter[3] = 0x01;
		nit->filter.filter.mask[3] = 0x01;

		nit->filter.flags = DMX_CHECK_CRC;
		
		nit->buf_size = 16*1024;
		
		if(table_open(nit) != 0)
		{
			AM_DEBUG(1, "table open fail.");
		}
	}else {
		AM_DEBUG(3, "-->|start table nit: monitor[%d]", monitor);
	
		table_stop(nit);
		RELEASE_TABLE_FROM_LIST(dvbpsi_nit_t, updm->psinit, nit);
	}

	nit->timeout = updm->upd.table_timeout;
	nit->monitor = monitor;

	AM_TIME_GetTimeSpecTimeout(nit->timeout, &nit->end_time);

	table_start(nit);

	return 0;
}

static int compare_timespec(const struct timespec *ts1, const struct timespec *ts2)
{
	assert(ts1 && ts2);

	if ((ts1->tv_sec > ts2->tv_sec) ||
		((ts1->tv_sec == ts2->tv_sec)&&(ts1->tv_nsec > ts2->tv_nsec)))
		return 1;
		
	if ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec == ts2->tv_nsec))
		return 0;
	
	return -1;
}

static void get_wait_timespec_mon(tsupd_t *upd, struct timespec *ts)
{
	int rel, i;
	struct timespec now, tmp;

#define UPDM_NORMAL_CHECK 200

	ts->tv_sec = 0;
	ts->tv_nsec = 0;
	AM_TIME_GetTimeSpec(&now);


	#define TABLE_MON_START_CHECK(_upd, evt1, evt2, start, now, id) \
		AM_MACRO_BEGIN \
			if((_upd)->tsevt & (evt1)) {\
				AM_DEBUG(5, "-->|check table[%d]", id); \
				if(compare_timespec(&(start), &(now))<=0) {\
					AM_DEBUG(3, "-->|table[%d] mon start", (id)); \
					(_upd)->tsevt &= ~(evt1); \
					(_upd)->tsevt |= (evt2); \
					pthread_cond_signal(&_upd->cond); \
				} \
			}\
		AM_MACRO_END

	TABLE_MON_START_CHECK(upd, tsevt_mon_table, tsevt_mon_table_start, upd->table_next_time, now, 3);

	if(upd->tsevt == 0)
	{
		#define TABLE_TIMEOUT_CHECK(table, now)\
			AM_MACRO_BEGIN\
				struct timespec end;\
				if (table_stopped != (table).status){\
					end = (table).end_time;\
					rel = compare_timespec(&(table).end_time, &now);\
					if (rel <= 0){\
						AM_DEBUG(4, "-->|table[%d] timeout", (table).type);\
						(table).done(TABLE_TIMEOUT, &(table));\
					}\
					else if (memcmp(&end, &(table).end_time, sizeof(struct timespec))){\
						if ((ts->tv_sec == 0 && ts->tv_nsec == 0) || \
							(compare_timespec(&(table).end_time, ts) < 0))\
							*ts = (table).end_time;\
					}\
				}\
			AM_MACRO_END

		TABLE_TIMEOUT_CHECK(upd->table, now);

	}

	/*minimal poll interval*/
	AM_TIME_GetTimeSpecTimeout(UPDM_NORMAL_CHECK, &tmp);
	if((ts->tv_sec == 0 && ts->tv_nsec == 0)
		|| compare_timespec(&tmp, ts) < 0)
	{
		AM_DEBUG(4, "m:%d", UPDM_NORMAL_CHECK);
		*ts = tmp;
	}
}


static int on_new_nit(tsupdm_t *updm)
{
	int ret = 0;

	pthread_mutex_lock(&updm->upd.table.lock);

	if(updm->upd.table.monitor)
	{
		AM_DEBUG(3, "-->| on new nit");

		dvbpsi_nit_t *nit;
		LIST_FOR_EACH(updm->psinit_monitor, nit)
		{
			if(nit->i_version != updm->upd.table_ver)
			{
				ret = 1;
				if(updm->upd.cb)
				{
					table_secbuf_t *secbuf; 
					list_for_each_entry(secbuf, &updm->upd.table.secbufs, head)
					{
						updm->upd.cb(secbuf->buf, secbuf->len, updm->upd.cb_args);
					}
				}
				goto end;
			}
		}
	}
	else 
	{
		AM_DEBUG(1, "-->| on new nit");

		if(!updm->upd.table.sec_len_max) 
		{
			//3 1st, timeout
			if(updm->upd.cb)
				updm->upd.cb(NULL, 0, updm->upd.cb_args);
			ret = 1;
		}
		else
		{
			dvbpsi_nit_t *nit;
			LIST_FOR_EACH(updm->psinit, nit)
			{
				updm->upd.table_ver = nit->i_version;
			}

			if(updm->upd.cb)
			{
				table_secbuf_t *secbuf;	
				list_for_each_entry(secbuf, &updm->upd.table.secbufs, head)
				{
					updm->upd.cb(secbuf->buf, secbuf->len, updm->upd.cb_args);
				}
			}
		}
	}

end:
	pthread_mutex_unlock(&updm->upd.table.lock);
	return ret;
}

static void *updm_thread(void *para)
{
	tsupdm_t *updm = (tsupdm_t*)para;
	tsupd_t *upd = &updm->upd;

	int ret = 0;
	struct timespec to;

	AM_DEBUG(4,"updm thread start.\n");

	updm_restart_nit(updm, 0);

	AM_DEBUG(4,"nit started.\n");

	while (!upd->quit)
	{
		pthread_mutex_lock(&upd->lock);

		get_wait_timespec_mon(upd, &to);
		
		if (to.tv_sec == 0 && to.tv_nsec == 0)
			ret = pthread_cond_wait(&upd->cond, &upd->lock);
		else
			ret = pthread_cond_timedwait(&upd->cond, &upd->lock, &to);
		
		AM_DEBUG(4, "upd work: timeout[%d], tsevt[0x%x]", (ret== ETIMEDOUT), upd->tsevt);

		/*new nit*/
		if(upd->tsevt & tsevt_new_table)
		{
			if(on_new_nit(updm))
			{
				AM_DEBUG(1, "restart nit. nit changed or 1st timeout");
				
				updm_restart_nit(updm, 0);
			}
			else
			{
				upd->tsevt |= tsevt_mon_table;
				AM_TIME_GetTimeSpecTimeout(upd->table_check_interval, &upd->table_next_time);
				AM_DEBUG(3, "-->|next nit mon");
			}
		
			upd->tsevt &= ~tsevt_new_table;
		}

		/*monitor*/
		if(upd->tsevt & tsevt_mon_table_start)
			updm_restart_nit(updm, 1);

		pthread_mutex_unlock(&upd->lock);
	}

	AM_DEBUG(1, "table_close");
	table_stop(&upd->table);
	RELEASE_TABLE_FROM_LIST(dvbpsi_nit_t, updm->psinit, &upd->table);
	RELEASE_TABLE_FROM_LIST(dvbpsi_nit_t, updm->psinit_monitor, &upd->table);
	table_close(&upd->table);

	return NULL;
}



#define _CHECKID(_id, _updx, _upd, _type, _magic) \
	do{ \
		tsupd_t* t = ((tsupd_t*)_id); \
		if((t->magic!=(_magic)) \
			|| (t->stat==stat_idle)) \
			return AM_UPD_ERROR_BADID; \
		_updx = (_type*)t; \
		_upd = t; \
	}while(0);

#define CHECKMID(_id, _updm, _upd) \
	_CHECKID(_id, _updm, _upd, tsupdm_t, _UPDM_MAGIC)

AM_ErrorCode_t AM_TSUPD_OpenMonitor(AM_TSUPD_OpenMonitorParam_t *para, AM_TSUPD_MonHandle_t *mid)
{
	if(!mid)
		return AM_UPD_ERROR_BADPARA;

	if(tsupdm.upd.stat!=stat_idle)
	{
		return AM_UPD_ERROR_BADSTAT;
	}

	memset(&tsupdm, 0, sizeof(tsupdm));

	tsupdm.upd.stat = stat_stopped;
	tsupdm.upd.table.fid = -1;
	tsupdm.upd.dmx = para->dmx;
	tsupdm.upd.magic = _UPDM_MAGIC;

	*mid = &tsupdm;

	AM_DEBUG(2, "open monitor");
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TSUPD_StartMonitor(AM_TSUPD_MonHandle_t mid, AM_TSUPD_MonitorParam_t *para)
{
#define NIT_TIMEOUT_DEFAULT 10000 //10s
#define NIT_MONITOR_INTERVAL_DEFAULT 20000 //20s

	tsupdm_t *updm;
	tsupd_t *upd;

	pthread_mutexattr_t mta;
	int rc;

	CHECKMID(mid, updm, upd);

	if(upd->stat==stat_started)
		return AM_UPD_ERROR_BADSTAT;

	upd->cb = para->callback;
	upd->cb_args = para->callback_args;
	updm->use_ext_nit = para->use_ext_nit;

	if(!updm->use_ext_nit) {
		upd->pid = AM_SI_PID_NIT;
		upd->tid = AM_SI_TID_NIT_ACT;
		upd->table_timeout = para->timeout;
		upd->table_check_interval = para->nit_check_interval;

		if(!upd->table_timeout)          upd->table_timeout=NIT_TIMEOUT_DEFAULT;
		if(!upd->table_check_interval)   upd->table_check_interval=NIT_MONITOR_INTERVAL_DEFAULT;

		pthread_mutexattr_init(&mta);
	//	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
		pthread_mutex_init(&upd->lock, &mta);
		pthread_cond_init(&upd->cond, NULL);
		pthread_mutexattr_destroy(&mta);

		rc = pthread_create(&upd->thread, NULL, updm_thread, (void*)updm);
		if(rc)
		{
			AM_DEBUG(1, "am_upd thread create fail: %s", strerror(rc));
			pthread_mutex_destroy(&upd->lock);
			pthread_cond_destroy(&upd->cond);
			return AM_FAILURE;
		}
	}
	else {
		AM_DEBUG(1, "use ext nit inject\n");
	}

	upd->stat = stat_started;

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TSUPD_StopMonitor(AM_TSUPD_MonHandle_t mid)
{
	tsupdm_t *updm;
	tsupd_t *upd;

	CHECKMID(mid, updm, upd);
	
	if(upd->stat==stat_started) {

		if(! updm->use_ext_nit) {
			pthread_t t;

			pthread_mutex_lock(&upd->lock);
			upd->cb = NULL;
			upd->quit = 1;
			t = upd->thread;
			pthread_mutex_unlock(&upd->lock);

			pthread_cond_signal(&upd->cond);
			
			if (t != pthread_self())
				pthread_join(t, NULL);

		} else {

			upd->cb = NULL;

		}

		upd->stat = stat_stopped;

	}

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TSUPD_InjectNIT(AM_TSUPD_MonHandle_t mid, unsigned char* p_nit, unsigned int len)
{
	tsupdm_t *updm;
	tsupd_t *upd;

	CHECKMID(mid, updm, upd);

	if(upd->stat==stat_started) {
		if(updm->use_ext_nit) {
			if(upd->cb)
				upd->cb(p_nit, len, upd->cb_args);
		}
	}
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TSUPD_CloseMonitor(AM_TSUPD_MonHandle_t mid)
{
	tsupdm_t *updm;
	tsupd_t *upd;

	CHECKMID(mid, updm, upd);

	if(upd->stat!=stat_stopped) {
		AM_TSUPD_StopMonitor(mid);
	}

	memset(updm, 0, sizeof(*updm));
	
	return AM_SUCCESS;
}


static int dlt_done(int status, table_t *table)
{
	tsupdd_t *updd = (tsupdd_t *)table->args;

	if(status==TABLE_DONE)
	{
		table_secbuf_t *secbuf;
		unsigned int sec_data_len_max = table->sec_len_max-4-5;// 4:crc 5:header_after_len
		unsigned int pdl_data_len_max = 4 + (table->last+1)*sec_data_len_max;
		updd->pdl_data = malloc(pdl_data_len_max);
		updd->dl_data_len = 0;
		if(updd->pdl_data) {
			unsigned char *ptr = (unsigned char *)updd->pdl_data + 4;
			updd->dl_data_len += 4;
			unsigned int sec_data_len = 0;
			unsigned short part_num=0, last_part_num=0;
			list_for_each_entry(secbuf, &table->secbufs, head)
			{
				if(!part_num) {
					part_num = ((uint16_t)secbuf->buf[3]<<4) | ((uint16_t)secbuf->buf[4]>>4);
					last_part_num = ((uint16_t)(secbuf->buf[4]&0xf) << 8) | (uint16_t)secbuf->buf[5];
				}
				sec_data_len = secbuf->len-4-8;
				updd->dl_data_len += sec_data_len;
				if(sec_data_len != sec_data_len_max) {
					AM_DEBUG(1, "Warning: sec len[%d] is not sec_len_max[%d]", sec_data_len, sec_data_len_max);
				}
				memcpy(ptr+(secbuf->sec_num*sec_data_len),
						secbuf->buf+8,
						sec_data_len);
			}

			unsigned char *ppn = (unsigned char*)updd->pdl_data;
			ppn[0] = (part_num>>8) & 0xff;
			ppn[1] = part_num & 0xff;
			ppn[2] = (last_part_num>>8) & 0xff;
			ppn[3] = last_part_num & 0xff;
			AM_DEBUG(1, "Note: dlt part %d of %d done. size: %d\n", part_num, last_part_num, updd->dl_data_len);

		}
	}
	else if(status==TABLE_TIMEOUT)
	{
		AM_DEBUG(1, "dlt timeout!");
		updd->upd.table.sec_len_max = 0;
		updd->dl_data_len = 0;
	}

	updd->upd.tsevt |= tsevt_new_table;
	pthread_cond_signal(&updd->upd.cond);

	table_stop(table);

	return 0;
}


static int updm_restart_dlt(tsupdd_t *updd)
{
	table_t *tdl = &updd->upd.table;

	updd->upd.tsevt &= ~(tsevt_new_table);
	updd->upd.table_ver = 0xff;

	AM_DEBUG(1, "-->|start table dlt: pid[%x] tid[%x] ext:[%x]", updd->upd.pid, updd->upd.tid, updd->upd.ext);
	
	table_close(tdl);

	memset(tdl, 0, sizeof(table_t));
	
	/*!!fid==-1 when closed!!*/
	tdl->fid = -1;
	tdl->type = 0x10;
	
	tdl->dmx = updd->upd.dmx;
	tdl->done = dlt_done;
	tdl->args = updd;
	
	tdl->filter.pid = updd->upd.pid;
	tdl->filter.filter.filter[0] = updd->upd.tid;
	tdl->filter.filter.mask[0] = 0xff;
	tdl->filter.filter.filter[1] = (updd->upd.ext&0xff0)>>4;
	tdl->filter.filter.mask[1] = 0xff;
	tdl->filter.filter.filter[2] = (updd->upd.ext&0xf)<<4;
	tdl->filter.filter.mask[2] = 0xf0;

	tdl->filter.flags = DMX_CHECK_CRC;
	
	tdl->buf_size = 300*1024;
	
	if(table_open(tdl) != 0)
	{
		AM_DEBUG(1, "table open fail.");
	}

	tdl->timeout = updd->upd.table_timeout;

	AM_TIME_GetTimeSpecTimeout(tdl->timeout, &tdl->end_time);

	table_start(tdl);

	return 0;
}


static void get_wait_timespec_dl(tsupd_t *upd, struct timespec *ts)
{
	int rel, i;
	struct timespec now, tmp;

#define UPDD_NORMAL_CHECK 200

	ts->tv_sec = 0;
	ts->tv_nsec = 0;
	AM_TIME_GetTimeSpec(&now);

	if(upd->tsevt == 0)
	{
		#define TABLE_TIMEOUT_CHECK(table, now)\
			AM_MACRO_BEGIN\
				struct timespec end;\
				if (table_stopped != (table).status){\
					end = (table).end_time;\
					rel = compare_timespec(&(table).end_time, &now);\
					if (rel <= 0){\
						AM_DEBUG(4, "-->|table[%d] timeout", (table).type);\
						(table).done(TABLE_TIMEOUT, &(table));\
					}\
					else if (memcmp(&end, &(table).end_time, sizeof(struct timespec))){\
						if ((ts->tv_sec == 0 && ts->tv_nsec == 0) || \
							(compare_timespec(&(table).end_time, ts) < 0))\
							*ts = (table).end_time;\
					}\
				}\
			AM_MACRO_END

		TABLE_TIMEOUT_CHECK(upd->table, now);

	}

	/*minimal poll interval*/
	AM_TIME_GetTimeSpecTimeout(UPDD_NORMAL_CHECK, &tmp);
	if((ts->tv_sec == 0 && ts->tv_nsec == 0)
		|| compare_timespec(&tmp, ts) < 0)
	{
		AM_DEBUG(4, "m:%d", UPDD_NORMAL_CHECK);
		*ts = tmp;
	}
}


static void *updd_thread(void *para)
{
	tsupdd_t *updd = (tsupdd_t*)para;
	tsupd_t *upd = &updd->upd;
	
	int ret = 0;
	struct timespec to;

	AM_DEBUG(4,"thread start.\n");

	updm_restart_dlt(updd);

	AM_DEBUG(4,"dlt started.\n");

	while (!upd->quit)
	{
		pthread_mutex_lock(&upd->lock);

		get_wait_timespec_dl(upd, &to);
		
		if (to.tv_sec == 0 && to.tv_nsec == 0)
			ret = pthread_cond_wait(&upd->cond, &upd->lock);
		else
			ret = pthread_cond_timedwait(&upd->cond, &upd->lock, &to);
		
		AM_DEBUG(4, "updd work: timeout[%d], tsevt[0x%x]", (ret== ETIMEDOUT), upd->tsevt);

		/*new dlt*/
		if(upd->tsevt & tsevt_new_table)
		{
		
			AM_DEBUG(1, "-->| on new updt");

			if(upd->cb)
			{
				upd->cb(updd->pdl_data, 
					upd->table.sec_len_max? updd->dl_data_len : 0,
					upd->cb_args);
			}

			upd->tsevt &= ~tsevt_new_table;

			table_close(&upd->table);
		}

		pthread_mutex_unlock(&upd->lock);
	}

	table_close(&upd->table);
	if(updd->pdl_data)
		free(updd->pdl_data);

	AM_DEBUG(1, "dl thread exit.");
	return NULL;
}

#define CHECKDID(_id, _updd, _upd) \
	_CHECKID(_id, _updd, _upd, tsupdd_t, _UPDD_MAGIC)

AM_ErrorCode_t AM_TSUPD_OpenDownloader(AM_TSUPD_OpenDownloaderParam_t *para, AM_TSUPD_DlHandle_t *did)
{
	tsupdd_t *tsupdd;

	if(!did)
		return AM_UPD_ERROR_BADPARA;

	tsupdd = (tsupdd_t*)malloc(sizeof(tsupdd_t));
	if(!tsupdd)
		return AM_UPD_ERROR_NOMEM;
	
	memset(tsupdd, 0, sizeof(tsupdd_t));

	tsupdd->upd.stat = stat_stopped;
	tsupdd->upd.table.fid = -1;
	tsupdd->upd.dmx = para->dmx;
	tsupdd->upd.magic = _UPDD_MAGIC;

	*did = tsupdd;

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TSUPD_StartDownloader(AM_TSUPD_DlHandle_t did, AM_TSUPD_DownloaderParam_t *para)
{
#define DLT_TIMEOUT_DEFAULT 20000 //20s

	tsupdd_t *updd;
	tsupd_t *upd;

	pthread_mutexattr_t mta;
	int rc;


	CHECKDID(did, updd, upd);

	if(upd->stat==stat_started)
		return AM_UPD_ERROR_BADSTAT;

	upd->cb = para->callback;
	upd->cb_args = para->callback_args;
	upd->table_timeout = para->timeout;

	upd->pid = para->pid;
	upd->tid = para->tableid;
	upd->ext = para->ext;

	if(!upd->table_timeout)			upd->table_timeout=DLT_TIMEOUT_DEFAULT;

	pthread_mutexattr_init(&mta);
//	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&upd->lock, &mta);
	pthread_cond_init(&upd->cond, NULL);
	pthread_mutexattr_destroy(&mta);

	rc = pthread_create(&upd->thread, NULL, updd_thread, (void*)updd);
	if(rc)
	{
		AM_DEBUG(1, "am_upd dl thread create fail: %s", strerror(rc));
		pthread_mutex_destroy(&upd->lock);
		pthread_cond_destroy(&upd->cond);
		return AM_FAILURE;
	}

	upd->stat = stat_started;

	return AM_SUCCESS;
}

//data valid before stopDownloader called
AM_ErrorCode_t AM_TSUPD_GetDownloaderData(AM_TSUPD_DlHandle_t did, unsigned char **ppdata, unsigned int *len)
{
	tsupdd_t *updd;
	tsupd_t *upd;

	CHECKDID(did, updd, upd);

	if(!ppdata || !len)
		return AM_UPD_ERROR_BADPARA;

	if(upd->stat!=stat_started)
		return AM_UPD_ERROR_BADSTAT;

	*ppdata = updd->pdl_data;
	*len = updd->dl_data_len;
	
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TSUPD_StopDownloader(AM_TSUPD_DlHandle_t did)
{
	tsupdd_t *updd;
	tsupd_t *upd;

	CHECKDID(did, updd, upd);
	
	if(upd->stat==stat_started) {

		pthread_t t;

		pthread_mutex_lock(&upd->lock);
		upd->cb = NULL;
		upd->quit = 1;
		t = upd->thread;
		pthread_mutex_unlock(&upd->lock);

		pthread_cond_signal(&upd->cond);
		
		if (t != pthread_self())
			pthread_join(t, NULL);

		upd->stat = stat_stopped;
	}
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_TSUPD_CloseDownloader(AM_TSUPD_DlHandle_t did)
{
	tsupdd_t *updd;
	tsupd_t *upd;

	CHECKDID(did, updd, upd);

	if(upd->stat!=stat_stopped) {
		AM_TSUPD_StopDownloader(did);
	}

	memset(updd, 0, sizeof(*updd));
	
	return AM_SUCCESS;
}


