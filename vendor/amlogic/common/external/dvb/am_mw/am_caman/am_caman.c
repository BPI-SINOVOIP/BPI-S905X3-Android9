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

#define AM_DEBUG_LEVEL 2

#include <errno.h>
#include <assert.h>

#include "am_types.h"
#include "am_dmx.h"
#include "am_si.h"
#include "am_time.h"
#include "am_fend.h"

#include "am_caman.h"
#include "ca_dummy.h"
#include "am_cond.h"

#define CAMAN_ONE_TS

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

enum {
	TABLE_DONE,
	TABLE_TIMEOUT
};

typedef struct {
	struct list_head head;

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
	uint16_t		ext;
	uint8_t		ver;
	uint8_t		sec;
	uint8_t		last;
	uint8_t		mask[32];

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
};

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


static inline int tablectl_test_recved(table_t *table, AM_SI_SectionHeader_t *header)
{	
	return ((table->ext == header->extension) && 
		(table->ver == header->version) && 
		(table->last == header->last_sec_num) && 
		!BIT_TEST(table->mask, header->sec_num));
}
static inline int tablectl_test_complete(table_t *table)
{
	static uint8_t test_array[32] = {0};
	return !((table->ver != 0xff) &&
		memcmp(table->mask, test_array, sizeof(test_array)));
}
static int tablectl_test_section(table_t *table, AM_SI_SectionHeader_t *header)
{
	/*version changed, reset ctl*/
	if (table->ver != 0xff && 
		(table->ver != header->version ||\
		table->ext != header->extension ||\
		table->last != header->last_sec_num))
	{
		memset(table->mask, 0, sizeof(table->mask));
		table->ver = 0xff;
		return -1;
	}
	return 0;
}
static int tablectl_mark_section(table_t *table, AM_SI_SectionHeader_t *header)
{

	if (table->ver == 0xff)
	{
		int i;
		table->last = header->last_sec_num;
		table->ver = header->version;	
		table->ext = header->extension;
		for (i=0; i<(table->last+1); i++)
			BIT_SET(table->mask, i);
	}

	BIT_CLEAR(table->mask, header->sec_num);

	return AM_SUCCESS;
}

static void section_handler(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	table_t *table = (table_t *)user_data;
	AM_SI_SectionHeader_t header;
	table_secbuf_t *secbuf;

	UNUSED(dev_no);
	UNUSED(fid);

	if (!table || !data || (table->fid==-1))
		return;

	pthread_mutex_lock(&table->lock);

	/* the section_syntax_indicator bit must be 1 */
	if ((data[1]&0x80) == 0)
	{
		AM_DEBUG(1, "table: section_syntax_indicator is 0, skip this section");
		goto end;
	}
	
	if (AM_SI_GetSectionHeader(table->si, (uint8_t*)data, len, &header) != AM_SUCCESS)
	{
		AM_DEBUG(1, "Scan: section header error");
		goto end;
	}

	if(tablectl_test_section(table, &header)!=0)
	{
		table_secbuf_t *sec, *n;
		list_for_each_entry_safe(sec, n, &table->secbufs, head)
		{
			list_del(&sec->head);
			free(sec->buf);
			free(sec);
		}
	}
	
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
	list_add(&secbuf->head, &table->secbufs);

	tablectl_mark_section(table, &header);

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
		AM_SI_Destroy(table->si);
	
	pthread_mutex_destroy(&table->lock);

	return 0;
}

struct _ts_s;
struct _ca_s;
struct _ca_msg_s;
struct _caman_s;

typedef struct _ts_s _ts_t;
typedef struct _ca_s _ca_t;
typedef struct _ca_msg_s _ca_msg_t;
typedef struct _caman_s _caman_t;

struct _ts_s{
	int fend_fd;
	int dmx_fd;

	__u16  service_id;
	__u16  pmt_pid;
	__u8    cat_ver;
	__u8    pmt_ver;
	
	table_t pmt;
	dvbpsi_pmt_t *psipmt;
	dvbpsi_pmt_t *psipmt_monitor;
	struct timespec pmt_next_time;
	int pmt_timeout;
	int pmt_mon_interval;

	table_t cat;
	dvbpsi_cat_t *psicat;
	dvbpsi_cat_t *psicat_monitor;
	struct timespec cat_next_time;
	int cat_timeout;
	int cat_mon_interval;

	table_t pat;
	dvbpsi_pat_t *psipat;
	dvbpsi_pat_t *psipat_monitor;
	struct timespec pat_next_time;
	int pat_timeout;
	int pat_mon_interval;

	struct dvb_frontend_event fend_evt;

	int tsevt;
#define tsevt_new_ts 0x01
#define tsevt_new_pmt 0x02
#define tsevt_new_cat 0x04
#define tsevt_new_pat 0x08
#define tsevt_mon_pmt 0x20
#define tsevt_mon_cat 0x40
#define tsevt_mon_pat 0x80
#define tsevt_mon_pmt_start 0x200
#define tsevt_mon_cat_start 0x400
#define tsevt_mon_pat_start 0x800


	int pmt_disable;

	struct list_head calist;
	_ca_t *ca_force;

	_caman_t *caman;
} ;

struct _ca_msg_s{
	struct list_head head;
	
	unsigned int magic;
#define _CA_MSG_MAGIC	0x33445566

 	AM_CA_Msg_t msg;
} ;

struct _ca_s{
	struct list_head head;

	char *name;
	AM_CA_Type_t type;
	AM_CA_Ops_t ops;
	void *arg;
	void (*arg_destroy)(void *arg);

	int status;
	int auto_disable;

	struct list_head msglist;
	int (*msgcb)(char *name, AM_CA_Msg_t *msg);

	_ts_t *ts;
	_caman_t *caman;
} ;

struct _caman_s{
	pthread_mutex_t     lock;
	pthread_cond_t      cond;
	pthread_t          	thread;

	int                         quit;
	int                         pause;
	/*ca list*/
	struct list_head calist;
	int canum;
	
	_ts_t ts;
} ;

static _caman_t *caman = NULL;

static int ca_open(_ca_t *cai, _ts_t *ts);
static int ca_close(_ca_t *cai);

static void free_ca(_ca_t *cai)
{
	free(cai->name);
	free(cai);
}

static void free_ca_msg( _ca_msg_t *msgi)
{
	if(msgi->msg.data)
		free(msgi->msg.data);
	free(msgi);
}

static _ca_msg_t *get_ca_msg(_ca_t *cai, int pick)
{
	_ca_msg_t *msgi = NULL;
	if(!list_empty(&cai->msglist)) {
		msgi = list_first_entry(&cai->msglist, _ca_msg_t, head);
		if(!pick)
			list_del(&msgi->head);
	}
	return msgi;
}

static _ca_msg_t *add_ca_msg(_ca_t *cai, _ca_msg_t *msgi) 
{
	list_add_tail(&msgi->head, &cai->msglist);
	return msgi;
}

static void rm_ca_msgs(_ca_t *cai, void (*free_msg)( _ca_msg_t *msgi))
{
	struct list_head *pos = NULL, *n = NULL;
	list_for_each_safe(pos, n, &cai->msglist)
	{
		list_del(pos);
		if(free_msg)
			free_msg(list_entry(pos, _ca_msg_t, head));
	}
}

static _ca_t *get_ca(char *name)
{
	_ca_t *ca = NULL;
	
	list_for_each_entry(ca, &caman->calist, head)
	{
		if(0 == strcmp(ca->name, name))
			return ca;
	}
	list_for_each_entry(ca, &caman->ts.calist, head)
	{
		if(0 == strcmp(ca->name, name))
			return ca;
	}
	return NULL;
}

static _ca_t *add_ca(_ca_t *ca, struct list_head *list)
{
	list_add_tail(&ca->head, list);
	return ca;
}

static _ca_t *rm_ca(char *name, struct list_head *list)
{
	_ca_t *ca = NULL, *tmp = NULL;
	
	list_for_each_entry_safe(ca, tmp, list, head)
	{
		if(0 == strcmp(ca->name, name))
		{
			list_del(&ca->head);
			return ca;
		}
	}
	return NULL;
}
static inline int ca_num(void)
{
	return (caman->canum==1);
}

/*
	if name is NULL, do remove all the cas
*/
static int do_rm_ca(char *name, void (*free_ca)(_ca_t *cai))
{
	_ca_t *cai = NULL, *n = NULL;

#define DO_RM(cai, name, free_ca) \
		if(!name || (name && !strcmp(cai->name, name))) \
		{ \
			list_del(&cai->head); \
			 \
			if(cai->status && cai->ops.enable) \
				cai->ops.enable(cai->arg, 0); \
			 \
			if(cai->ts && (cai->ts->ca_force==cai)) \
				cai->ts->ca_force = NULL; \
			 \
			rm_ca_msgs(cai, free_ca_msg); \
			 \
 			ca_close(cai); \
			 \
			if(cai->arg_destroy) \
				cai->arg_destroy(cai->arg); \
			 \
			if(free_ca) \
				free_ca(cai); \
			 \
			if(name) \
				return 0; \
		}

	list_for_each_entry_safe(cai, n, &caman->calist, head)
	{
		DO_RM(cai, name, free_ca);
	}
	list_for_each_entry_safe(cai, n, &caman->ts.calist, head)
	{
		DO_RM(cai, name, free_ca);
	}
	return -1;
}

static int ts_changed(struct dvb_frontend_event *evt1, struct dvb_frontend_event *evt2)
{
	UNUSED(evt1);
	UNUSED(evt2);
	return 1;
}

static void caman_fend_callback(long dev_no, int event_type, void *param, void *user_data)
{
	struct dvb_frontend_event *evt = (struct dvb_frontend_event*)param;
	_caman_t *man = (_caman_t *)user_data;

	UNUSED(dev_no);
	UNUSED(event_type);

	if (!evt || (evt->status == 0))
		return;

	pthread_mutex_lock(&man->lock);

	if(ts_changed(&man->ts.fend_evt, evt))
	{
		man->ts.fend_evt = *evt;
		man->ts.tsevt |= tsevt_new_ts;
		printf("fend: new ts callback\n");
		pthread_cond_signal(&caman->cond);
	}

	pthread_mutex_unlock(&man->lock);

}

/*arrange cas*/
static int match_ca_auto(_caman_t *caman, _ts_t *ts, dvbpsi_ca_dr_t *cadesr)
{
	_ca_t *cai, *n;
	struct list_head tmp, *pos;

	INIT_LIST_HEAD(&tmp);
	/*check current unmatched ca in ts's calist*/
	list_for_each_entry_safe(cai, n, &ts->calist, head)
	{
		if(cai->ops.camatch)
			if(!cai->ops.camatch(cai->arg, cadesr->i_ca_system_id))
			{
				list_move(&cai->head, &tmp);
				if(cai->ops.stop_pmt)
					cai->ops.stop_pmt(cai->arg, -1);
				cai->ts = NULL;
			}
	}

	/*check matched ca*/
	list_for_each_entry_safe(cai, n, &caman->calist, head)
	{
		if(!cai->auto_disable/*not dis-auto*/ 
			&& cai->ops.camatch)
			if(cai->ops.camatch(cai->arg, cadesr->i_ca_system_id))
			{
				list_move(&cai->head, &ts->calist);
				if(cai->ops.open)
				{
					AM_CAMAN_Ts_t ts_ca = {.fend_fd = ts->fend_fd, .dmx_fd = ts->dmx_fd};
					cai->ops.open(cai->arg, &ts_ca);
					cai->ts = ts;
				}
			}
	}

	/*collect unused ca*/
	list_for_each(pos, &tmp)
	{
		list_move(pos, &caman->calist);
	}

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

static int pmt_done(int status, table_t *table)
{
	if(status==TABLE_DONE)
	{
		table_secbuf_t *secbuf;
		_ts_t *ts = (_ts_t *)table->args;
		
		list_for_each_entry(secbuf, &table->secbufs, head)
		{
			dvbpsi_pmt_t *p_table;
			if (AM_SI_DecodeSection(table->si, table->filter.pid, (uint8_t*)secbuf->buf, secbuf->len, (void**)&p_table) == AM_SUCCESS)
			{
				p_table->p_next = NULL;
				if(table->monitor)
					ADD_TO_LIST(p_table, ts->psipmt_monitor);
				else
					ADD_TO_LIST(p_table, ts->psipmt);
			}else
			{
				AM_DEBUG(1, "Decode Section failed");
			}
		}

		ts->tsevt |= tsevt_new_pmt;
		pthread_cond_signal(&ts->caman->cond);
	}
	else if(status==TABLE_TIMEOUT)
	{
		if(table->monitor)
			AM_DEBUG(3, "pmt timeout!");
		else
			AM_DEBUG(1, "pmt timeout!");
	}

	table_stop(table);

	return 0;
}

static int start_table_pmt(_ts_t *ts, int monitor, unsigned int pid, unsigned int pn, int (*pmt_done)(int, table_t *))
{

	table_t *pmt = &ts->pmt;

	ts->tsevt &= ~(tsevt_new_pmt|tsevt_mon_pmt|tsevt_mon_pmt_start);

	if(!monitor)
	{
		AM_DEBUG(1, "-->|start table pmt: monitor[%d] pid[%d] pn[%d]", monitor, pid, pn);

		table_stop(pmt);
		RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->psipmt, pmt);
		RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->psipmt_monitor, pmt);
		table_close(pmt);

		memset(pmt, 0, sizeof(table_t));

		/*!!fid==-1 when closed!!*/
		pmt->fid = -1;
		pmt->type = 2;

		pmt->dmx = ts->dmx_fd;
		pmt->done = pmt_done;
		pmt->args = ts;

		pmt->filter.pid = pid;
		pmt->filter.filter.filter[0] = AM_SI_TID_PMT;
		pmt->filter.filter.mask[0] = 0xff;
		
		pmt->filter.filter.filter[1] = (uint8_t)((pn&0xff00)>>8);
		pmt->filter.filter.mask[1] = 0xff;
		pmt->filter.filter.filter[2] = (uint8_t)(pn&0xff);
		pmt->filter.filter.mask[2] = 0xff;

		/*Current next indicator must be 1*/
		pmt->filter.filter.filter[3] = 0x01;
		pmt->filter.filter.mask[3] = 0x01;
		
		pmt->filter.flags = DMX_CHECK_CRC;

		pmt->buf_size = 16*1024;

		if(table_open(pmt) != 0)
		{
			AM_DEBUG(1, "table open fail.");
		}
	}
	else
	{
		AM_DEBUG(3, "-->|start table pmt: monitor[%d] pid[%d] pn[%d]", monitor, pid, pn);

		table_stop(pmt);
		RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->psipmt_monitor, pmt);
	}

	pmt->timeout = ts->pmt_timeout;
	pmt->monitor = monitor;

	AM_TIME_GetTimeSpecTimeout(pmt->timeout, &pmt->end_time);

	table_start(pmt);

	return pmt->fid;
}

static int cat_done(int status, table_t *table)
{
	if(status==TABLE_DONE)
	{
		table_secbuf_t *secbuf;
		_ts_t *ts = (_ts_t *)table->args;
		
		list_for_each_entry(secbuf, &table->secbufs, head)
		{
			dvbpsi_cat_t *p_table;
			if (AM_SI_DecodeSection(table->si, table->filter.pid, (uint8_t*)secbuf->buf, secbuf->len, (void**)&p_table) == AM_SUCCESS)
			{
				p_table->p_next = NULL;
				if(table->monitor)
					ADD_TO_LIST(p_table, ts->psicat_monitor);
				else
					ADD_TO_LIST(p_table, ts->psicat);
			}else
			{
				AM_DEBUG(1, "Decode Section failed");
			}
		}

		ts->tsevt |= tsevt_new_cat;
		pthread_cond_signal(&ts->caman->cond);
	}
	else if(status==TABLE_TIMEOUT)
	{
		if(table->monitor)
			AM_DEBUG(3, "cat timeout!");
	}

	table_stop(table);

	return 0;
}
static int start_table_cat(_ts_t *ts, int monitor, int (*cat_done)(int, table_t *))
{
	table_t *cat = &ts->cat;
	
	ts->tsevt &= ~(tsevt_new_cat|tsevt_mon_cat|tsevt_mon_cat_start);

	if(!monitor)
	{
		AM_DEBUG(1, "-->|start table cat: monitor[%d]", monitor);

		table_stop(cat);
		RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, ts->psicat, cat);
		RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, ts->psicat_monitor, cat);
		table_close(cat);

		memset(cat, 0, sizeof(table_t));
		
		/*!!fid==-1 when closed!!*/
		cat->fid = -1;
		cat->type = 0;

		cat->dmx = ts->dmx_fd;
		cat->done = cat_done;
		cat->args = ts;

		cat->filter.pid = AM_SI_PID_CAT;
		cat->filter.filter.filter[0] = AM_SI_TID_CAT;
		cat->filter.filter.mask[0] = 0xff;
		
		/*Current next indicator must be 1*/
		cat->filter.filter.filter[3] = 0x01;
		cat->filter.filter.mask[3] = 0x01;
		
		cat->filter.flags = DMX_CHECK_CRC;

		cat->buf_size = 16*1024;

		if(table_open(cat) != 0)
		{
			AM_DEBUG(1, "table open fail.");
		}
	}
	else
	{
		AM_DEBUG(3, "-->|start table cat: monitor[%d]", monitor);

		table_stop(cat);
		RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, ts->psicat, cat);
	}

	cat->timeout = ts->cat_timeout;
	cat->monitor = monitor;

	AM_TIME_GetTimeSpecTimeout(cat->timeout, &cat->end_time);

	table_start(cat);

	return cat->fid;
}

static int pat_done(int status, table_t *table)
{
	if(status==TABLE_DONE)
	{
		table_secbuf_t *secbuf;
		_ts_t *ts = (_ts_t *)table->args;
		
		list_for_each_entry(secbuf, &table->secbufs, head)
		{
			dvbpsi_pat_t *p_table;
			if (AM_SI_DecodeSection(table->si, table->filter.pid, (uint8_t*)secbuf->buf, secbuf->len, (void**)&p_table) == AM_SUCCESS)
			{
				p_table->p_next = NULL;
				if(table->monitor)
					ADD_TO_LIST(p_table, ts->psipat_monitor);
				else
					ADD_TO_LIST(p_table, ts->psipat);
			}else
			{
				AM_DEBUG(1, "Decode Section failed");
			}
		}

		ts->tsevt |= tsevt_new_pat;
		pthread_cond_signal(&ts->caman->cond);
	}
	else if(status==TABLE_TIMEOUT)
	{
		if(table->monitor)
			AM_DEBUG(3, "pat timeout!");
		else
			AM_DEBUG(1, "pat timeout!");
	}

	table_stop(table);

	return 0;
}
static int start_table_pat(_ts_t *ts, int monitor, int (*pat_done)(int, table_t *))
{
	table_t *pat = &ts->pat;
	
	ts->tsevt &= ~(tsevt_new_pat|tsevt_mon_pat|tsevt_mon_pat_start);

	if(!monitor)
	{
		AM_DEBUG(1, "-->|start table pat: monitor[%d]", monitor);

		table_stop(pat);
		RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->psipat, pat);
		RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->psipat_monitor, pat);
		table_close(pat);

		memset(pat, 0, sizeof(table_t));
		
		/*!!fid==-1 when closed!!*/
		pat->fid = -1;
		pat->type = 1;

		pat->dmx = ts->dmx_fd;
		pat->done = pat_done;
		pat->args = ts;

		pat->filter.pid = AM_SI_PID_PAT;
		pat->filter.filter.filter[0] = AM_SI_TID_PAT;
		pat->filter.filter.mask[0] = 0xff;
		
		/*Current next indicator must be 1*/
		pat->filter.filter.filter[3] = 0x01;
		pat->filter.filter.mask[3] = 0x01;
		
		pat->filter.flags = DMX_CHECK_CRC;

		pat->buf_size = 16*1024;

		if(table_open(pat) != 0)
		{
			AM_DEBUG(1, "table open fail.");
		}
	}
	else
	{
		AM_DEBUG(3, "-->|start table pat: monitor[%d]", monitor);
	
		table_stop(pat);
		RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->psipat, pat);
	}

	pat->timeout = ts->pat_timeout;
	pat->monitor = monitor;

	AM_TIME_GetTimeSpecTimeout(pat->timeout, &pat->end_time);

	table_start(pat);

	return pat->fid;
}

static int stop_table(table_t *table)
{
	table_close(table);
	return 0;
}

static int close_ts(_ts_t *ts)
{
	table_stop(&ts->pmt);
	table_stop(&ts->pat);
	table_stop(&ts->cat);

	ts->tsevt &= ~(tsevt_new_pmt|tsevt_mon_pmt|tsevt_mon_pmt_start);
	ts->tsevt &= ~(tsevt_new_pat|tsevt_mon_pat|tsevt_mon_pat_start);
	ts->tsevt &= ~(tsevt_new_cat|tsevt_mon_cat|tsevt_mon_cat_start);

	RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->psipmt, &ts->pmt);
	RELEASE_TABLE_FROM_LIST(dvbpsi_pmt_t, ts->psipmt_monitor, &ts->pmt);
	
	RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->psipat, &ts->pat);
	RELEASE_TABLE_FROM_LIST(dvbpsi_pat_t, ts->psipat_monitor, &ts->pat);
	
	RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, ts->psicat, &ts->cat);
	RELEASE_TABLE_FROM_LIST(dvbpsi_cat_t, ts->psicat_monitor, &ts->cat);

	table_close(&ts->pmt);
	table_close(&ts->pat);
	table_close(&ts->cat);

	return 0;
}

static int on_new_ts(_ts_t *ts)
{
	_ca_t *cai;

	AM_DEBUG(1, "-->| on new ts");

	cai = ts->ca_force;

	if(cai)
	{
		if(cai->ops.ts_changed)
			cai->ops.ts_changed(cai->arg);
	}
	else
	{
		list_for_each_entry(cai, &ts->calist, head)
		{
			if(cai->ops.ts_changed)
				cai->ops.ts_changed(cai->arg);
		}
	}

	/*PAT & CAT*/
	close_ts(ts);

	start_table_pat(ts, 0, pat_done);
	start_table_cat(ts, 0, cat_done);
	
	return 0;
}

static int on_new_pmt(_ts_t *ts)
{
	_ca_t *cai;
	int ret = 0;

	pthread_mutex_lock(&ts->pmt.lock);

	if(ts->pmt.monitor)
	{
		AM_DEBUG(3, "-->| on new pmt");

		dvbpsi_pmt_t *pmt;
		LIST_FOR_EACH(ts->psipmt_monitor, pmt)
		{
			if(pmt->i_version != ts->pmt_ver)
			{
				ret = 1;
				goto end;
			}
		}
	}
	else 
	{
		AM_DEBUG(1, "-->| on new pmt");

		dvbpsi_pmt_t *pmt;
		LIST_FOR_EACH(ts->psipmt, pmt)
		{
			ts->pmt_ver = pmt->i_version;
		}

		cai = ts->ca_force;
		if(cai)
		{
			if(cai->ops.start_pmt)
			{
				table_secbuf_t *secbuf;
				secbuf = list_first_entry(&ts->pmt.secbufs, table_secbuf_t, head);
				cai->ops.start_pmt(cai->arg, ts->service_id, secbuf->buf, secbuf->len);
			}
		}
		else
		{
			LIST_FOR_EACH(ts->psipmt, pmt)
			{
				dvbpsi_descriptor_t *descr;
				LIST_FOR_EACH(pmt->p_first_descriptor, descr)
				{
					if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_CA)
					{
						dvbpsi_ca_dr_t *pca = (dvbpsi_ca_dr_t*)descr->p_decoded;

						if (descr->i_length > 0)
						{
							match_ca_auto(ts->caman, ts, pca);
							ts->pmt_ver = pmt->i_version;
							goto pmt_check_end;
						}
					}
				}
			}
			pmt_check_end:

			if(!ts->pmt_disable) 
			{
				list_for_each_entry(cai, &ts->calist, head)
				{
					if(cai->ops.start_pmt)
					{
						table_secbuf_t *secbuf;
						secbuf = list_first_entry(&ts->pmt.secbufs, table_secbuf_t, head);
						cai->ops.start_pmt(cai->arg, ts->service_id, secbuf->buf, secbuf->len);
					}
				}
			}
		}
	}

end:
	pthread_mutex_unlock(&ts->pmt.lock);
	return ret;
}

static int on_new_cat(_ts_t *ts)
{
	int ret = 0;
	_ca_t *cai;


	pthread_mutex_lock(&ts->cat.lock);

	if(ts->cat.monitor)
	{
		AM_DEBUG(3, "-->| on new cat");

		dvbpsi_cat_t *cat;
		LIST_FOR_EACH(ts->psicat_monitor, cat)
		{
			if(cat->i_version != ts->cat_ver)
			{
				ret = 1;
				goto end;
			}
		}
	}
	else
	{
		AM_DEBUG(1, "-->| on new cat");

		dvbpsi_cat_t *cat;
		LIST_FOR_EACH(ts->psicat, cat)
		{
			ts->cat_ver = cat->i_version;
		}

		cai = ts->ca_force;
		if(cai)
		{
			if(cai->ops.new_cat)
			{
				table_secbuf_t *secbuf;
				secbuf = list_first_entry(&ts->cat.secbufs, table_secbuf_t, head);
				cai->ops.new_cat(cai->arg, secbuf->buf, secbuf->len);
			}
		}
		else
		{
			LIST_FOR_EACH(ts->psicat, cat)
			{
				dvbpsi_descriptor_t *descr;
				LIST_FOR_EACH(cat->p_first_descriptor, descr)
				{
					if (descr->p_decoded && descr->i_tag == AM_SI_DESCR_CA)
					{
						dvbpsi_ca_dr_t *pca = (dvbpsi_ca_dr_t*)descr->p_decoded;

						if (descr->i_length > 0)
						{
							match_ca_auto(ts->caman, ts, pca);
							ts->cat_ver = cat->i_version;
							goto cat_check_end;
						}
					}
				}
			}
			cat_check_end:
			list_for_each_entry(cai, &ts->calist, head)
			{
				if(cai->ops.new_cat)
				{
					table_secbuf_t *secbuf;
					secbuf = list_first_entry(&ts->cat.secbufs, table_secbuf_t, head);
					cai->ops.new_cat(cai->arg, secbuf->buf, secbuf->len);
				}
			}
		}
	}

end:
	pthread_mutex_unlock(&ts->cat.lock);
	return ret;
}

static int on_new_pat(_ts_t *ts)
{
	dvbpsi_pat_t *pat;
	dvbpsi_pat_program_t *prog;
	int ret = 0;

	pthread_mutex_lock(&ts->pat.lock);
	
	if(ts->pat.monitor)
	{
		AM_DEBUG(3, "-->| on new pat");

		if(0xffff != ts->service_id)
		{
			LIST_FOR_EACH(ts->psipat_monitor, pat)
			{
				LIST_FOR_EACH(pat->p_first_program, prog)
				{
					if (prog->i_number == ts->service_id)
					{
						if(prog->i_pid != ts->pmt_pid)
						{
							ret = 1;
							goto end;
						}
					}
				}
			}
		}
	}
	else
	{
		AM_DEBUG(1, "-->| on new pat");

		LIST_FOR_EACH(ts->psipat, pat)
		{
			LIST_FOR_EACH(pat->p_first_program, prog)
			{
				if ((0xffff != ts->service_id) 
					&& (prog->i_number == ts->service_id))
				{
					AM_DEBUG(1, "Got program %d's PMT, pid is 0x%x", prog->i_number, prog->i_pid);
					start_table_pmt(ts, 0, prog->i_pid, prog->i_number, pmt_done);
					ts->pmt_pid = prog->i_pid;
					goto end;
				}
			}
		}
	}

end:
	pthread_mutex_unlock(&ts->pat.lock);
	return ret;
}

static int on_process_msgs(_caman_t *man)
{
	_ca_t *cai;

#define ca_process_msg(cai) \
		do { \
			_ca_msg_t *msgi, *n; \
			list_for_each_entry_safe(msgi, n, &cai->msglist, head) \
			{ \
				if(cai->msgcb) { \
					list_del(&msgi->head); \
					cai->msgcb(cai->name, &msgi->msg); \
				} \
			} \
		}while(0)

	list_for_each_entry(cai, &man->calist, head)
	{
		ca_process_msg(cai);
	}

	list_for_each_entry(cai, &man->ts.calist, head)
	{
		ca_process_msg(cai);
	}
	return 0;
}


static void get_wait_timespec(_ts_t *tsi, struct timespec *ts)
{
	int rel, i;
	struct timespec now, tmp;

#define CAMAN_NORMAL_CHECK 200

	ts->tv_sec = 0;
	ts->tv_nsec = 0;
	AM_TIME_GetTimeSpec(&now);


	#define TABLE_MON_START_CHECK(tsi, evt1, evt2, start, now, id) \
		AM_MACRO_BEGIN \
			if((tsi)->tsevt & (evt1)) {\
				AM_DEBUG(5, "-->|check table[%d]", id); \
				if(compare_timespec(&(start), &(now))<=0) {\
					AM_DEBUG(3, "-->|table[%d] mon start", (id)); \
					(tsi)->tsevt &= ~(evt1); \
					(tsi)->tsevt |= (evt2); \
					pthread_cond_signal(&tsi->caman->cond); \
				} \
			}\
		AM_MACRO_END

	TABLE_MON_START_CHECK(tsi, tsevt_mon_pmt, tsevt_mon_pmt_start, tsi->pmt_next_time, now, 2);
	TABLE_MON_START_CHECK(tsi, tsevt_mon_pat, tsevt_mon_pat_start, tsi->pat_next_time, now, 1);
	TABLE_MON_START_CHECK(tsi, tsevt_mon_cat, tsevt_mon_cat_start, tsi->cat_next_time, now, 0);

	if(tsi->tsevt == 0)
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

		TABLE_TIMEOUT_CHECK(tsi->pmt, now);
		TABLE_TIMEOUT_CHECK(tsi->cat, now);
		TABLE_TIMEOUT_CHECK(tsi->pat, now);

	}

	/*minimal poll interval*/
	AM_TIME_GetTimeSpecTimeout(CAMAN_NORMAL_CHECK, &tmp);
	if((ts->tv_sec == 0 && ts->tv_nsec == 0)
		|| compare_timespec(&tmp, ts) < 0)
	{
		AM_DEBUG(4, "m:%d", CAMAN_NORMAL_CHECK);
		*ts = tmp;
	}
}

static void *caman_thread(void *para)
{
	_caman_t *man = (_caman_t*)para;
	_ts_t *ts;
	
	int ret = 0;
	struct dvb_frontend_info info;
	struct timespec to;
	
	AM_FEND_GetInfo(man->ts.fend_fd, &info);
	
	AM_EVT_Subscribe(man->ts.fend_fd, AM_FEND_EVT_STATUS_CHANGED, caman_fend_callback, (void*)man);

	while (!man->quit)
	{
		pthread_mutex_lock(&man->lock);

		ts = &man->ts;

		get_wait_timespec(ts, &to);

		if (to.tv_sec == 0 && to.tv_nsec == 0)
			ret = pthread_cond_wait(&man->cond, &man->lock);
		else
			ret = pthread_cond_timedwait(&man->cond, &man->lock, &to);

		/*process msgs*/
		on_process_msgs(man);

		if(man->pause)
		{
			ts->tsevt = 0;
			goto end;
		}

		AM_DEBUG(4, "caman work: timeout[%d], tsevt[0x%x]", (ret== ETIMEDOUT), ts->tsevt);

		//if (ret != ETIMEDOUT)
		{
			/*TS changed*/
			if(ts->tsevt & tsevt_new_ts)
			{
				on_new_ts(ts);

				ts->tsevt &= ~tsevt_new_ts;
			}

			/*new pmt*/
			if(ts->tsevt & tsevt_new_pmt)
			{
				if(on_new_pmt(ts))
				{
					AM_DEBUG(1, "pmt changed.");
					on_new_ts(ts);
				}
				else
				{
					ts->tsevt |= tsevt_mon_pmt;
					AM_TIME_GetTimeSpecTimeout(ts->pmt_mon_interval, &ts->pmt_next_time);
					AM_DEBUG(3, "-->|next pmt mon");
					//start_table_pmt(ts, 1, 0, 0, NULL);
				}

				ts->tsevt &= ~tsevt_new_pmt;
			}

			/*new cat*/
			if(ts->tsevt & tsevt_new_cat)
			{
				if(on_new_cat(ts))
				{
					AM_DEBUG(1, "cat changed.");
					on_new_ts(ts);
				}
				else
				{
					ts->tsevt |= tsevt_mon_cat;
					AM_TIME_GetTimeSpecTimeout(ts->cat_mon_interval, &ts->cat_next_time);
					AM_DEBUG(3, "-->|next cat mon");
					//start_table_cat(ts, 1, cat_done);
				}

				ts->tsevt &= ~tsevt_new_cat;
 			}

			/*new pat*/
			if(ts->tsevt & tsevt_new_pat)
			{
				if(on_new_pat(ts))
				{
					AM_DEBUG(1, "pat changed.");
					on_new_ts(ts);
				}
				else
				{
					ts->tsevt |= tsevt_mon_pat;
					AM_TIME_GetTimeSpecTimeout(ts->pat_mon_interval, &ts->pat_next_time);
					AM_DEBUG(3, "-->|next pat mon");
					//start_table_pat(ts, 1, pat_done);
				}

				ts->tsevt &= ~tsevt_new_pat;
			}

			/*monitor*/
			if(ts->tsevt & tsevt_mon_pmt_start)
				start_table_pmt(ts, 1, 0, 0, NULL);
			if(ts->tsevt & tsevt_mon_pat_start)
				start_table_pat(ts, 1, pat_done);
			if(ts->tsevt & tsevt_mon_cat_start)
				start_table_cat(ts, 1, cat_done);
		}
end:		
		pthread_mutex_unlock(&man->lock);
	}

	AM_EVT_Unsubscribe(man->ts.fend_fd, AM_FEND_EVT_STATUS_CHANGED, caman_fend_callback, (void*)man);

	return NULL;
}

static int _ca_send_msg(char *name, AM_CA_Msg_t *msg)
{
	_caman_t *man = caman;
	_ca_t *cai = NULL;
	_ca_msg_t *msgi = NULL;
	
	assert(name && msg);

	pthread_mutex_lock(&man->lock);

	cai = get_ca(name);
	if(!cai)
	{
		pthread_mutex_unlock(&man->lock);
		return -1;
	}

	msgi = malloc(sizeof(_ca_msg_t));
	if(!msgi)
	{
		pthread_mutex_unlock(&man->lock);
		return -2;
	}

	memset(msgi, 0, sizeof(_ca_msg_t));

	msgi->magic = _CA_MSG_MAGIC;
	msgi->msg = *msg;

	if(msg->dst == AM_CA_MSG_DST_APP)
	{
		add_ca_msg(cai, msgi);
	}
	else if(msg->dst == AM_CA_MSG_DST_CAMAN)
	{
		/*msg from ca to caman, handle it.*/
	}

	pthread_mutex_unlock(&man->lock);

	return 0;
}

static int ca_open(_ca_t *cai, _ts_t *ts)
{
	if(cai->status)
		return 0;

	if(cai->ops.open)
	{
		AM_CAMAN_Ts_t ts_ca = {.fend_fd = ts->fend_fd, .dmx_fd = ts->dmx_fd};
		if(0!= cai->ops.open(cai->arg, &ts_ca))
			return -1;
	}

	cai->status = 1;
	cai->ts = ts;

	if(cai->ops.register_msg_send)
		if(0!= cai->ops.register_msg_send(cai->arg, cai->name, _ca_send_msg))
			return -1;

	if(cai->ops.enable)
		if(0!= cai->ops.enable(cai->arg, 1))
			return -1;

	if(cai->ops.ts_changed)
		if(0!= cai->ops.ts_changed(cai->arg))
			return -1;

	return 0;
}

static int ca_close(_ca_t *cai)
{
	if(!cai->status)
		return 0;

	if(cai->ops.enable)
		if(0!= cai->ops.enable(cai->arg, 0))
			return -1;

	if(cai->ops.close)
		if(0!= cai->ops.close(cai->arg))
			return -1;

	cai->status = 0;
	cai->ts = NULL;

	return 0;
}

AM_ErrorCode_t AM_CAMAN_Open(AM_CAMAN_OpenParam_t *para)
{
#define PMT_TIMEOUT 3000
#define PMT_TIMEOUT_MONITOR 3000
#define PMT_MONITOR_INTERVAL 2000
#define CAT_TIMEOUT 3000
#define CAT_TIMEOUT_MONITOR 3000
#define CAT_MONITOR_INTERVAL 2000
#define PAT_TIMEOUT 3000
#define PAT_TIMEOUT_MONITOR 3000
#define PAT_MONITOR_INTERVAL 2000

	_caman_t *man = NULL;
	pthread_mutexattr_t mta;
	int rc;

	assert(para);
	
	if(caman)
		return AM_SUCCESS;

	AM_DEBUG(1, "CAMAN: Open[para:(%p)]", para);

	man =  malloc(sizeof(_caman_t));
	if(!man)
		return AM_CAMAN_ERROR_NO_MEM;

	memset(man, 0, sizeof(_caman_t));
	INIT_LIST_HEAD(&man->calist);
	man->pause = 0;
	man->quit = 0;
	man->canum = 0;

	man->ts.dmx_fd = para->ts.dmx_fd;
	man->ts.fend_fd = para->ts.fend_fd;
	man->ts.pmt_timeout = para->ts.pmt_timeout;
	man->ts.pmt_mon_interval = para->ts.pmt_mon_interval;
	man->ts.cat_timeout = para->ts.cat_timeout;
	man->ts.cat_mon_interval = para->ts.cat_mon_interval;
	man->ts.pat_timeout = para->ts.pat_timeout;
	man->ts.pat_mon_interval = para->ts.pat_mon_interval;

	if(man->ts.pmt_timeout == 0)            man->ts.pmt_timeout = PMT_TIMEOUT;
	if(man->ts.pmt_mon_interval == 0)    man->ts.pmt_mon_interval = PMT_MONITOR_INTERVAL;
	if(man->ts.cat_timeout == 0)            man->ts.cat_timeout = CAT_TIMEOUT;
	if(man->ts.cat_mon_interval == 0)    	man->ts.cat_mon_interval = CAT_MONITOR_INTERVAL;
	if(man->ts.pat_timeout == 0)            man->ts.pat_timeout = PAT_TIMEOUT;
	if(man->ts.pat_mon_interval == 0)    	man->ts.pat_mon_interval = PAT_MONITOR_INTERVAL;

	printf("-->|pmt[%d %d] cat[%d %d] pat[%d %d]", 
		man->ts.pmt_timeout, man->ts.pmt_mon_interval,
		man->ts.cat_timeout, man->ts.cat_mon_interval,
		man->ts.pat_timeout, man->ts.pat_mon_interval );
	
	man->ts.pmt.fid = -1;
	man->ts.cat.fid = -1;
	man->ts.pat.fid = -1;
	man->ts.tsevt = 0;
	
	man->ts.service_id = 0xffff;
	man->ts.pmt_pid = 0x1fff;
	man->ts.cat_ver = 0xff;
	man->ts.pmt_ver = 0xff;

	memset(&man->ts.fend_evt, 0, sizeof(man->ts.fend_evt));
	man->ts.caman = man;
	INIT_LIST_HEAD(&man->ts.calist);
	
	pthread_mutexattr_init(&mta);
//	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&man->lock, &mta);
	pthread_cond_init(&man->cond, NULL);
	pthread_mutexattr_destroy(&mta);
	/*创建监控线程*/
	rc = pthread_create(&man->thread, NULL, caman_thread, (void*)man);
	if(rc)
	{
		AM_DEBUG(1, "am_caman thread create fail: %s", strerror(rc));
		pthread_mutex_destroy(&man->lock);
		pthread_cond_destroy(&man->cond);
		free(man);
		return AM_CAMAN_ERROR_CANNOT_CREATE_THREAD;
	}

	caman = man;

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_Close(void)
{
	_caman_t *man = caman;
	pthread_t t;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	AM_DEBUG(1, "CAMAN: Close[]");

	pthread_mutex_lock(&man->lock);

	close_ts(&man->ts);
	
	man->quit = 1;
	t = man->thread;
	pthread_mutex_unlock(&man->lock);
	pthread_cond_signal(&man->cond);

	if (t != pthread_self())
		pthread_join(t, NULL);

	do_rm_ca(NULL, free_ca);

	pthread_mutex_destroy(&man->lock);
	pthread_cond_destroy(&man->cond);

	free(man);
	caman = NULL;

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_Pause(void)
{
	_caman_t *man = caman;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	AM_DEBUG(1, "CAMAN: Pause[]");

	pthread_mutex_lock(&man->lock);

	man->pause = 1;

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_Resume(void)
{
	_caman_t *man = caman;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	AM_DEBUG(1, "CAMAN: Resume[]");

	pthread_mutex_lock(&man->lock);

	man->pause = 0;

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_openCA(char *name)
{
	_caman_t *man = caman;
	_ts_t *ts = &man->ts;
	_ca_t *cai = NULL;
	int ret = 0;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;
	
	assert(name);

	AM_DEBUG(1, "CAMAN: openCA[name:(%s)]", name);

	pthread_mutex_lock(&man->lock);

	cai = get_ca(name);
	if(!cai)
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_CA_UNKOWN;
	}
	
	if(ca_open(cai, ts) != 0)
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_CA_ERROR;
	}

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_closeCA(char *name)
{
	_caman_t *man = caman;
	_ca_t *cai = NULL;
	int ret = 0;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(name);

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	AM_DEBUG(1, "CAMAN: closeCA[name:(%s)]", name);

	pthread_mutex_lock(&man->lock);
		
	cai = get_ca(name);
	if(!cai)
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_CA_UNKOWN;
	}
		
	if(ca_close(cai) != 0)
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_CA_ERROR;
	}

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_startService(int service_id, char *caname)
{
	_caman_t *man = caman;
	_ts_t *ts = &man->ts;
	_ca_t *cai = NULL, *fcai = NULL;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	AM_DEBUG(1, "CAMAN: startService[id(%d) caname:(%s)]", service_id, caname);

	pthread_mutex_lock(&man->lock);

	fcai = ts->ca_force;

	if(caname)
	{
		cai = get_ca(caname);
		if(!cai)
		{
			pthread_mutex_unlock(&man->lock);
			return AM_CAMAN_ERROR_CA_UNKOWN;
		}
		if(fcai && (fcai != cai))
		{
			if(fcai->ops.stop_pmt)
				fcai->ops.stop_pmt(fcai->arg, -1);
			fcai->ts = NULL;
		}
		cai->ts = ts;
		ts->ca_force = cai;
	}
	else
	{
		if(fcai)
		{
			if(fcai->ops.stop_pmt)
				fcai->ops.stop_pmt(fcai->arg, -1);
			fcai->ts = NULL;
			ts->ca_force = NULL;
		}
	}

	ts->pmt_disable = 0;

	ts->service_id = service_id;

	ts->tsevt |= tsevt_new_ts;

	pthread_cond_signal(&man->cond);
	
	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_stopService(int service_id)
{
	_caman_t *man = caman;
	_ts_t *ts = &man->ts;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	AM_DEBUG(1, "CAMAN: stopService[id(%d)]", service_id);

	pthread_mutex_lock(&man->lock);
	
	{
		_ca_t *cai, *fcai;
		fcai = ts->ca_force;
		if(fcai)
		{
			if(fcai->ops.stop_pmt)
				fcai->ops.stop_pmt(fcai->arg, service_id);
			fcai->ts = NULL;
			ts->ca_force = NULL;
		}
		else
		{
			list_for_each_entry(cai, &ts->calist, head)
			{
				if(cai->ops.stop_pmt)
					cai->ops.stop_pmt(cai->arg, service_id);
			}
		}
	}

	ts->pmt_disable = 1;
	ts->service_id = 0xffff;

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_stopCA(char *name)
{
	_caman_t *man = caman;
	_ca_t *cai = NULL;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(name);

	AM_DEBUG(1, "CAMAN: stopCA[caname:(%s)]", name);

	pthread_mutex_lock(&man->lock);

	cai = get_ca(name);
	if(!cai)
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_CA_UNKOWN;
	}

	if(cai->ops.stop_pmt)
		if(0 != cai->ops.stop_pmt(cai->arg, -1))
		{
			pthread_mutex_unlock(&man->lock);
			return AM_CAMAN_ERROR_CA_ERROR;
		}

	pthread_mutex_unlock(&man->lock);
	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_getMsg(char *caname, AM_CA_Msg_t **msg)
{
	_caman_t *man = caman;
	_ca_msg_t *msgi = NULL;
	_ca_t *cai = NULL;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(caname && msg);

	*msg = NULL;

	pthread_mutex_lock(&man->lock);

	cai = get_ca(caname);
	if(!cai)
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_CA_UNKOWN;
	}

	msgi = get_ca_msg(cai, 0);
	*msg = &msgi->msg;

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;

}

AM_ErrorCode_t AM_CAMAN_freeMsg(AM_CA_Msg_t *msg)
{
	if(!caman)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(msg);

	_ca_msg_t *msgi = container_of(msg, _ca_msg_t, msg);
	
	if(msgi && (msgi->magic != _CA_MSG_MAGIC))
		return AM_CAMAN_ERROR_BADPARAM;

	free_ca_msg(msgi);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_putMsg(char *name, AM_CA_Msg_t *msg)
{
	_caman_t *man = caman;
	int ret = AM_SUCCESS;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(name && msg);

	pthread_mutex_lock(&man->lock);
	
	if(msg->dst == AM_CA_MSG_DST_CA)
	{
		_ca_t *cai = get_ca(name);
		if(cai)
		{
			ret = cai->ops.msg_receive(cai->arg, msg);
		}
		else
		{
			pthread_mutex_unlock(&man->lock);
			return AM_CAMAN_ERROR_CA_UNKOWN;
		}
	}
	else if(msg->dst == AM_CA_MSG_DST_CAMAN)
	{
		/*msg from app to caman, handle it.*/
	}

	pthread_mutex_unlock(&man->lock);

	return ret;
}

AM_ErrorCode_t AM_CAMAN_setCallback(char *caname, int (*cb)(char *name, AM_CA_Msg_t *msg))
{
	_caman_t *man = caman;
	_ca_t *cai = NULL;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(caname || cb);

	AM_DEBUG(1, "CAMAN: setCallback[caname:(%s) cb(%p)]", caname, cb);

	pthread_mutex_lock(&man->lock);

	if(caname) {
		cai = get_ca(caname);
		if(!cai)
		{
			pthread_mutex_unlock(&man->lock);
			return AM_CAMAN_ERROR_CA_UNKOWN;
		}

		cai->msgcb = cb;
	}
	else
	{
		list_for_each_entry(cai, &man->calist, head)
		{
			cai->msgcb = cb;
		}
	}

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_registerCA(char *name, AM_CA_t *ca, AM_CA_Opt_t *opt)
{
	_caman_t *man = caman;
	_ca_t *cai = NULL;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(name && ca);

	AM_DEBUG(1, "CAMAN: registerCA[name:(%s) ca(%p) opt(%p)]", name, ca, opt);

	if(strcmp(name, "dummy")==0)
		ca = &dummy_ca;

	pthread_mutex_lock(&man->lock);

	if(get_ca(name))
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_CA_EXISTS;
	}
	
	cai = malloc(sizeof(_ca_t));
	if(!cai)
	{
		pthread_mutex_unlock(&man->lock);
		return AM_CAMAN_ERROR_NO_MEM;
	}

	memset(cai, 0, sizeof(_ca_t));

	cai->name = strdup(name);
	cai->type =  ca->type;
	cai->ops = ca->ops;
	cai->arg = ca->arg;
	cai->arg_destroy = ca->arg_destroy;
	cai->caman = caman;
	if(opt)
	{
		cai->auto_disable = opt->auto_disable;
	}
	INIT_LIST_HEAD(&cai->msglist);

	add_ca(cai, &caman->calist);
	caman->canum++;

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_unregisterCA(char *name)
{
	_caman_t *man = caman;

	if(!man)
		return AM_CAMAN_ERROR_NOTOPEN;

	assert(name);

	AM_DEBUG(1, "CAMAN: unregisterCA[name:(%s)]", name);

	pthread_mutex_lock(&man->lock);

	if(do_rm_ca(name, free_ca)==0)
		caman->canum--;

	pthread_mutex_unlock(&man->lock);

	return AM_SUCCESS;
}

AM_ErrorCode_t AM_CAMAN_getMatchedCA(char **names, unsigned int nnames)
{
	UNUSED(names);
	UNUSED(nnames);
	return AM_SUCCESS;
}

