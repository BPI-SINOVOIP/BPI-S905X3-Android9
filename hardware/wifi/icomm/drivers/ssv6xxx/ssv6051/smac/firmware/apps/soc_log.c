#include <stdarg.h>
#include <config.h>
#include <rtos.h>
#include "log.h"
#include <lib/ssv_lib.h>
#include <lib/lib-impl.h>

_os_mutex		g_log_mutex;
u32				g_dbg_mutex;

u8		g_magic_str[8]	= "$$v62@@";
u32		g_magic_u32		= 0xAAAAAAAA;

log_prnf_cfg_st		g_log_prnf_cfg;
log_out_desc_st		g_log_out;

// global variable for acceleration
//char s_acc_buf[ACC_BUF_MAX] = {0};
char s_prn_buf[PRN_BUF_MAX];
char s_prn_tmp[PRN_BUF_MAX];
char s_tag_lvl[PRN_TAG_MAX];
char s_tag_mod[PRN_TAG_MAX];
char s_tag_fl[PRN_TAG_MAX];

// u8		g_log_frmbuf[LOG_FRMBUF_MAX];

const char*	g_log_lvl_tag[LOG_LEVEL_OFF] = {
	""     ,		// LOG_LEVEL_ON
	"[TRACE] ",		// LOG_LEVEL_TRACE
	"[DEBUG] ",		// LOG_LEVEL_DEBUG
	"[INFO]  ",		// LOG_LEVEL_INFO
	"[WARN]  ",		// LOG_LEVEL_WARN
	"[FAIL]  ",		// LOG_LEVEL_FAIL
	"[ERROR] ",		// LOG_LEVEL_ERROR
	"[FATAL] ",		// LOG_LEVEL_FATAL
	// LOG_LEVEL_OFF
};

#define LOG_LVL_TAG_LEN   sizeof("[TRACE] ")

const char*	g_log_mod_tag[LOG_MODULE_ALL] = {
        ""       ,              // LOG_MODULE_EMPTY
        "<INIT> ",              // LOG_MODULE_INIT
        "<INT>  ",              // LOG_MODULE_INT
        "<OS>   ",              // LOG_MODULE_OS
        "<CMD>  ",              // LOG_MODULE_CMD
        //                      // LOG_MODULE_ALL
};

#define LOG_MOD_TAG_LEN   sizeof("<defrag> ")
/* ============================================================================
		static local functions on host/soc side
============================================================================ */
void _log_evt_prnf_dump(log_evt_prnf_st *prnf);

#if (CONFIG_LOG_ENABLE)
static inline const char *_extract_filename(const char *str)
{
	return str;
}
#endif

void	_log_out_dst_open_dump(u8 dst_open)
{
	log_printf("dst_open : 0x%02x ", dst_open);
	LOG_TAG_SUPPRESS_ON();
	if (dst_open & LOG_OUT_HOST_TERM)		log_printf("(LOG_OUT_HOST_TERM) ");
	if (dst_open & LOG_OUT_HOST_FILE)		log_printf("(LOG_OUT_HOST_FILE) ");
	if (dst_open & LOG_OUT_SOC_TERM)		log_printf("(LOG_OUT_SOC_TERM) ");
	if (dst_open & LOG_OUT_SOC_HOST_TERM)	log_printf("(LOG_OUT_SOC_HOST_TERM) ");
	if (dst_open & LOG_OUT_SOC_HOST_FILE)	log_printf("(LOG_OUT_SOC_HOST_FILE) ");
	LOG_TAG_SUPPRESS_OFF();
	log_printf("\n\r");
}

void	_log_out_dst_cur_dump(u8 dst_cur)
{
	log_printf("dst_cur  : 0x%02x ", g_log_out.dst_cur);
	LOG_TAG_SUPPRESS_ON();
	if (dst_cur & LOG_OUT_HOST_TERM)		log_printf("(LOG_OUT_HOST_TERM) ");
	if (dst_cur & LOG_OUT_HOST_FILE)		log_printf("(LOG_OUT_HOST_FILE) ");
	if (dst_cur & LOG_OUT_SOC_TERM)			log_printf("(LOG_OUT_SOC_TERM) ");
	if (dst_cur & LOG_OUT_SOC_HOST_TERM)	log_printf("(LOG_OUT_SOC_HOST_TERM) ");
	if (dst_cur & LOG_OUT_SOC_HOST_FILE)	log_printf("(LOG_OUT_SOC_HOST_FILE) ");
	LOG_TAG_SUPPRESS_OFF();
	log_printf("\n\r");
}

/* ============================================================================
		static local functions on host side
============================================================================ */


/* ============================================================================
		exported functions on host/soc side
============================================================================ */
void _log_tag_lvl_str(const u32 n, u8 str[LOG_MAX_TAG])
{
	memset(str, 0, LOG_MAX_TAG);
	switch (n)
	{
	case LOG_LEVEL_ON:
	    snprintf((char *)str, sizeof(str), "%s\n", "[ON]    ");
	    break;
	case LOG_LEVEL_OFF:
	    snprintf((char *)str, sizeof(str), "%s\n", "[OFF]   ");
	    break;
	default:
	    snprintf((char *)str, sizeof(str), "%s\n", g_log_lvl_tag[n]);
	    break;
	}
	return;
}

// return: # of str entries are copied
u32 _log_tag_mod_str(const u32 m, u8 str[LOG_MODULE_ALL][LOG_MAX_TAG])
{
	u32		i, r;

	r = 0;
	memset(str, 0, LOG_MODULE_ALL*LOG_MAX_TAG);
	for (i=0; i<LOG_MODULE_ALL; i++)
	{
		if ((m & LOG_MODULE_MASK(i)) && (strlen(g_log_mod_tag[i]) > 0))
		{
			snprintf((char *)str[r], sizeof(str[0]), "%s", g_log_mod_tag[i]);
			r++;
		}
	}
	return r;
}

void _log_prnf_cfg_dump(log_prnf_cfg_st *p)
{
	u32		i, r;
	u8		b_lvl[LOG_MAX_TAG];
	u8		b_mod[LOG_MODULE_ALL][LOG_MAX_TAG];


	log_printf("< %s >\n\r", "prnf cfg");

	log_printf("%-10s : %d\n", "level  tag", p->prn_tag_lvl);
	log_printf("%-10s : %d\n", "module tag", p->prn_tag_mod);
	
	_log_tag_lvl_str(p->lvl, b_lvl);
	log_printf("%-10s : %s", "level", b_lvl);
	
	log_printf("%-10s : %d\n", "fileline", p->fl);
	
	r = _log_tag_mod_str(p->mod, b_mod);
	log_printf("%-10s : ", "module");
	for (i=0; i<r; i++)
		log_printf("%s", b_mod[i]);

	log_printf("\n\n");

	return;
}


#ifdef ENABLE_LOG_TO_HOST
void _log_soc_send_evt_prn_buf(const char *buf, size_t size)
{
	log_soc_evt_st		*soc_evt;
	log_evt_prnf_buf_st	*prn;
	MsgEvent			*msg_evt;

	LOG_EVENT_ALLOC(msg_evt, SOC_EVT_LOG, LOG_SEVT_MAX_LEN);
	soc_evt = (log_soc_evt_st *)(LOG_EVENT_DATA_PTR(msg_evt));
	memset(soc_evt, 0, LOG_SEVT_MAX_LEN);
	LOG_EVENT_SET_LEN(msg_evt, LOG_SEVT_MAX_LEN);

	prn = (log_evt_prnf_buf_st *)(&(soc_evt->data));
	// dst_open & dst_cur
	prn->dst_open	= g_log_out.dst_open;
	prn->dst_cur		= g_log_out.dst_cur;
	// copy fmt string
	prn->fmt_len = size;//strlen(buf);
	strncpy((char *)(prn->buf), buf, prn->fmt_len);
	// for now, 'arg_len' is always 0
	prn->arg_len = 0;
	prn->len	 = sizeof(log_evt_prnf_buf_st) + prn->fmt_len + prn->arg_len;

	// cal soc evt's length & set cmd id
	soc_evt->len = prn->len + LOG_SEVT_CMD_DATA_OFFSET;
	soc_evt->cmd = LOG_SEVT_PRNF_BUF;
	//printf("# %d\n\r", __LINE__);

	LOG_EVENT_SEND(msg_evt);	
	return;
}

void _log_evt_prn_buf_dump(log_evt_prnf_buf_st *prn)
{
	u32		i;
	u32		arg_cnt;

	log_printf("< %s >\n", __FUNCTION__);
	log_printf("len     = %d\n", prn->len);
	log_printf("fmt_len = %d\n", prn->fmt_len);
	log_printf("arg_len = %d\n", prn->arg_len);
	_log_out_dst_open_dump(prn->dst_open);
	_log_out_dst_cur_dump(prn->dst_cur);
	
	log_printf("fmt = ");
	for (i=0; i<prn->fmt_len; i++)
		log_printf("%c", *((char *)(prn->buf+i)));
	
	arg_cnt = prn->arg_len / 4;
	log_printf("arg_cnt = %d\n", arg_cnt);
	for (i=0; i<arg_cnt; i++)
		log_printf("%d : 0x%08x\n", i, *(int *)(prn->buf + prn->fmt_len + i*4));
}

// ======================================================================================
// note : 
//		 These _log_evt_alloc() & _log_evt_free() are the copy of CmdEngine_HostEventAlloc() 
//	to avoid circular inclusion issue. If we want to solve this problem, we have to place 
//	our host evt func to another place.
// ======================================================================================
MsgEvent *_log_evt_alloc(ssv6xxx_soc_event hEvtID, s32 size)
{
    MsgEvent *MsgEv;
    HDR_HostEvent *chevt;

    MsgEv = msg_evt_alloc();
    if (MsgEv == NULL)
        return NULL;
    memset((void *)MsgEv, 0, sizeof(MsgEvent));
    MsgEv->MsgType = MEVT_PKT_BUF;
    MsgEv->MsgData = (u32)PBUF_MAlloc(size+sizeof(HDR_HostEvent) , NOTYPE_BUF);
    if (MsgEv->MsgData == 0) {
        msg_evt_free(MsgEv);
        return NULL;
    }
    chevt = (HDR_HostEvent *)MsgEv->MsgData;
    chevt->c_type  = HOST_EVENT;
    chevt->h_event = hEvtID;
    chevt->len     = size;
    return MsgEv;
}

void	_log_evt_free(MsgEvent *MsgEvt)
{
	if (MsgEvt == NULL)
		return;
	
	if (MsgEvt->MsgData != (u32)NULL)
		PBUF_MFree((void *)(MsgEvt->MsgData));

	msg_evt_free(MsgEvt);
	return;
}
#endif // ENABLE_LOG_TO_HOST

void LOG_hcmd_dump(log_hcmd_st	*hcmd)
{
	log_printf("< log hcmd >\n");
	log_printf("target : %d (0:HOST , 1:SOC)\n", hcmd->target);
	log_printf("cmd    : %d (", hcmd->cmd);
	switch (hcmd->cmd)
	{
	case LOG_HCMD_TAG_ON_FL		: log_printf("LOG_HCMD_TAG_ON_FL");		break;
	case LOG_HCMD_TAG_OFF_FL	: log_printf("LOG_HCMD_TAG_OFF_FL");	break;	
	case LOG_HCMD_TAG_ON_LEVEL	: log_printf("LOG_HCMD_TAG_ON_LEVEL");	break;
	case LOG_HCMD_TAG_OFF_LEVEL	: log_printf("LOG_HCMD_TAG_OFF_LEVEL");	break;
	case LOG_HCMD_TAG_ON_MODULE	: log_printf("LOG_HCMD_TAG_ON_MODULE");	break;
	case LOG_HCMD_TAG_OFF_MODULE: log_printf("LOG_HCMD_TAG_OFF_MODULE");break;	
	case LOG_HCMD_MODULE_ON		: log_printf("LOG_HCMD_MODULE_ON");		break;
	case LOG_HCMD_MODULE_OFF	: log_printf("LOG_HCMD_MODULE_OFF");	break;	
	case LOG_HCMD_LEVEL_SET		: log_printf("LOG_HCMD_LEVEL_SET");		break;
	case LOG_HCMD_PRINT_USAGE	: log_printf("LOG_HCMD_PRINT_USAGE");	break;
	case LOG_HCMD_PRINT_STATUS	: log_printf("LOG_HCMD_PRINT_STATUS");	break;
	case LOG_HCMD_PRINT_MODULE	: log_printf("LOG_HCMD_PRINT_MODULE");	break;
	case LOG_HCMD_TEST			: log_printf("LOG_HCMD_TEST");			break;
    default: break;
	}
	log_printf("\n");
	log_printf("data   : %d ", hcmd->data);
	if ((hcmd->cmd == LOG_HCMD_MODULE_ON) || (hcmd->cmd == LOG_HCMD_MODULE_OFF))
	{
		switch (hcmd->data)
		{
		case LOG_MODULE_EMPTY:	log_printf("(empty)\n");			break;
		case LOG_MODULE_ALL:	log_printf("(all)\n");				break;
		default:	log_printf("(%s)\n", g_log_mod_tag[hcmd->data]);	break;
		}
		return;
	}
	if (hcmd->cmd == LOG_HCMD_LEVEL_SET)
	{
		switch (hcmd->data)
		{
		case LOG_LEVEL_OFF:		log_printf("(off)\n");				break;
		case LOG_LEVEL_ON:		log_printf("(on)\n");				break;
		default:	log_printf("(%s)\n", g_log_lvl_tag[hcmd->data]);	break;
		}
		return;
	}
	log_printf("\n");
}

void LOG_init(bool tag_level, bool tag_mod, u32 level, u32 mod_mask, bool fileline)
{
	LOG_MUTEX_INIT();

	memset(&g_log_prnf_cfg, 0, sizeof(log_prnf_cfg_st));
	g_log_prnf_cfg.prn_tag_lvl	= true;
	g_log_prnf_cfg.prn_tag_mod	= true;
	g_log_prnf_cfg.prn_tag_sprs	= false;
	g_log_prnf_cfg.chk_tag_sprs	= false;

	g_log_prnf_cfg.prn_tag_lvl	= tag_level;
	g_log_prnf_cfg.prn_tag_mod	= tag_mod;
	g_log_prnf_cfg.lvl			= level;

	mod_mask |= LOG_MODULE_MASK(LOG_MODULE_EMPTY);	// by default, enable LOG_MODULE_EMPTY
	g_log_prnf_cfg.mod			= mod_mask;
	g_log_prnf_cfg.fl			= fileline;

	// init frm buf
	//memset(g_log_frmbuf, 0xAA, LOG_FRMBUF_MAX);
}


#ifdef ENABLE_LOG_TO_HOST
bool LOG_soc_exec_hcmd(log_hcmd_st *hcmd)
{
	u8	cmd = hcmd->cmd;
	u8	idx = hcmd->data;

	if (cmd == LOG_HCMD_TEST)
	{
#if 1
		// t_printf(LOG_LEVEL_ON,    LOG_MODULE_EMPTY,		T("0 args\n"));
		// t_printf(LOG_LEVEL_TRACE, LOG_MODULE_MRX,		T("1 char args  - %c\n"), 'a');
		// t_printf(LOG_LEVEL_DEBUG, LOG_MODULE_MTX,		T("1 str args   - %s\n"), T("hello world"));
		// t_printf(LOG_LEVEL_INFO,  LOG_MODULE_EDCA,		T("3 digit args - %d, %3d, %03d\n"), 9, 9, 9);
		// t_printf(LOG_LEVEL_WARN,  LOG_MODULE_PBUF,		T("3 hex args   - %x, %X, %08x\n"), 0xab, 0xab, 0xab);
		// t_printf(LOG_LEVEL_FAIL,  LOG_MODULE_L3L4,		T("mix args     - %10s, %d, %s\n"), T("string"), 10, T("hello world"));
		// t_printf(LOG_LEVEL_ERROR, LOG_MODULE_MGMT,		T("mix args     - %-3d, %-10s, %-08x\n"), 10, "string", 0xab);
		// t_printf(LOG_LEVEL_ON,    LOG_MODULE_FRAG,		T("0 args\n"));
		// t_printf(LOG_LEVEL_TRACE, LOG_MODULE_DEFRAG,		T("1 char args  - %c\n"), 'a');
		// t_printf(LOG_LEVEL_DEBUG, LOG_MODULE_MLME,		T("1 str args   - %s\n"), T("hello world"));
		t_printf(LOG_LEVEL_ERROR,  LOG_MODULE_CMD,		T("3 digit args - %d, %3d, %03d\n"), 9, 9, 9);
		// t_printf(LOG_LEVEL_WARN,  LOG_MODULE_WPA,		T("3 hex args   - %x, %X, %08x\n"), 0xab, 0xab, 0xab);
		// t_printf(LOG_LEVEL_FAIL,  LOG_MODULE_MAIN,		T("mix args     - %10s, %d, %s\n"), T("string"), 10, T("hello world"));

		return true;
#endif
		// LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_HOST_TERM);
		// LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_HOST_FILE);
		log_printf("\n");
		log_printf("test : log_printf() %s\n", "Hello~ I am handsome smart Eric Chen!!");
		LOG_PRINTF("test : LOG_PRINTF() %d\n", LOG_LEVEL_ON);
		LOG_TRACE ("test : LOG_TRACE () %d\n", LOG_LEVEL_TRACE);
		LOG_DEBUG ("test : LOG_DEBUG () %d\n", LOG_LEVEL_DEBUG);
		LOG_INFO  ("test : LOG_INFO  () %d\n", LOG_LEVEL_INFO);
		LOG_WARN  ("test : LOG_WARN  () %d\n", LOG_LEVEL_WARN);
		LOG_FAIL  ("test : LOG_FAIL  () %d\n", LOG_LEVEL_FAIL);
		LOG_ERROR ("test : LOG_ERROR () %d\n", LOG_LEVEL_ERROR);
		// LOG_OUT_DST_CUR_ON(LOG_OUT_SOC_HOST_TERM);
		// LOG_OUT_DST_CUR_ON(LOG_OUT_SOC_HOST_FILE);
		
		//log_printf("g_log_frmbuf = 0x%08x, size = %d\n\r", g_log_frmbuf, LOG_FRMBUF_MAX);
		//ssv6xxx_raw_dump_ex((s8 *)g_log_frmbuf, SIZE_1KB, true, 16, 16, 16, LOG_LEVEL_ON, LOG_MODULE_EMPTY);
		return true;
	}

	switch (cmd)
	{
	case LOG_HCMD_PRINT_STATUS:                                 goto ret_prn_status;
	case LOG_HCMD_TAG_ON_FL:      LOG_FILELINE_TURN_ON();       goto ret_prn_status;
	case LOG_HCMD_TAG_OFF_FL:     LOG_FILELINE_TURN_OFF();      goto ret_prn_status;
	case LOG_HCMD_TAG_ON_LEVEL:   LOG_TAG_LEVEL_TURN_ON();      goto ret_prn_status;
	case LOG_HCMD_TAG_OFF_LEVEL:  LOG_TAG_LEVEL_TURN_OFF();     goto ret_prn_status;
	case LOG_HCMD_TAG_ON_MODULE:  LOG_TAG_MODULE_TURN_ON();     goto ret_prn_status;
	case LOG_HCMD_TAG_OFF_MODULE: LOG_TAG_MODULE_TURN_OFF();    goto ret_prn_status;
	case LOG_HCMD_LEVEL_SET:      LOG_LEVEL_SET(idx);           goto ret_prn_status;
	case LOG_HCMD_MODULE_ON:      LOG_MODULE_TURN_ON(idx);      goto ret_prn_status;
	case LOG_HCMD_MODULE_OFF:
		LOG_MODULE_TURN_OFF(idx);
		if (idx == LOG_MODULE_ALL)
			LOG_MODULE_TURN_ON(LOG_MODULE_EMPTY);
		goto ret_prn_status;
	default:
		return false;
	}

	// never reach here!!
	return false;

ret_prn_status:
	{
		u16				len;
		MsgEvent		*msg_evt;
		log_soc_evt_st	*soc_evt;
		log_evt_status_report_st	status;

		len = sizeof(log_soc_evt_st) + sizeof(log_evt_status_report_st);
		LOG_EVENT_ALLOC(msg_evt, SOC_EVT_LOG, len);
	
		soc_evt = (log_soc_evt_st *)(LOG_EVENT_DATA_PTR(msg_evt));
		memset(soc_evt, 0, len);
		soc_evt->len = len;
		soc_evt->cmd = LOG_SEVT_STATUS_REPORT;

		memcpy(&(status.prnf_cfg), &(g_log_prnf_cfg), sizeof(log_prnf_cfg_st));
		memcpy(&(status.out_desc), &(g_log_out),	  sizeof(log_out_desc_st));

		memcpy(&(soc_evt->data), &status, sizeof(log_evt_status_report_st));	
		LOG_EVENT_SET_LEN(msg_evt, len);
#if 0
		log_printf("%s()<=\n", __FUNCTION__);
		log_printf("soc_evt->len = %d\n", soc_evt->len);
		log_printf("sizeof(log_evt_status_report_st) = %d\n", sizeof(log_evt_status_report_st));
		LOG_out_desc_dump();
		ssv6xxx_raw_dump((s8 *)soc_evt, soc_evt->len);
		log_printf("%s()=>\n", __FUNCTION__);
#endif
		LOG_EVENT_SEND(msg_evt);

		return true;
	}

	return true;
}
#endif // ENABLE_LOG_TO_HOST

void LOG_out_desc_dump(void)
{
	log_printf("< log output descriptor >\n\r");
	_log_out_dst_open_dump(g_log_out.dst_open);
	_log_out_dst_cur_dump(g_log_out.dst_cur);

	return;
}

// init g_log_out 
bool LOG_out_init(void)	
{
	memset(&g_log_out, 0, sizeof(log_out_desc_st));
	return true;
}

bool	_soc_out_dst_open(u8 _dst, const u8 *_path)
{
	ASSERT((_dst == LOG_OUT_SOC_TERM) || (_dst == LOG_OUT_SOC_HOST_TERM) || (_dst == LOG_OUT_SOC_HOST_FILE));
	if (_dst == LOG_OUT_SOC_TERM)
	{
		if (LOG_OUT_DST_IS_OPEN(LOG_OUT_SOC_TERM))
			return false;

		LOG_OUT_DST_OPEN(LOG_OUT_SOC_TERM);
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_TERM)
	{
		if (LOG_OUT_DST_IS_OPEN(LOG_OUT_SOC_HOST_TERM))
			return false;

		LOG_OUT_DST_OPEN(LOG_OUT_SOC_HOST_TERM);
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_FILE)
	{
		if (LOG_OUT_DST_IS_OPEN(LOG_OUT_SOC_HOST_FILE))
			return false;
		
		LOG_PRINTF("%s(): to be done for fopen() on host side!\n\r");

		/* TODO : we must implement the 'sync' evt handling mechanism between host/soc side

		if (g_log_out.fp != NULL)
		{
			LOG_FAIL("g_log_out.fp != NULL\n\r");
			return false;
		}
		if ((g_log_out.fp = fopen((const char *)_path, LOG_OUT_FILE_MODE)) == NULL)
		{
			LOG_FAIL("fail to fopen() %s\n", _path);
			return false;
		}
		strcpy((char *)(g_log_out.path), (const char *)_path);
		LOG_TRACE("%s(): fopen() %s\n", __FUNCTION__, _path);
		*/
		LOG_OUT_DST_OPEN(LOG_OUT_SOC_HOST_FILE);
		return true;
	}

	// should never reach here!!
	return false;
}

bool	_soc_out_dst_close(u8 _dst)
{
	ASSERT((_dst == LOG_OUT_SOC_TERM) || (_dst == LOG_OUT_SOC_HOST_TERM) || (_dst == LOG_OUT_SOC_HOST_FILE));
	if (_dst == LOG_OUT_SOC_TERM)
	{
		if (LOG_OUT_DST_IS_OPEN(LOG_OUT_SOC_TERM))
		{
			LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_TERM);
			LOG_OUT_DST_CLOSE(LOG_OUT_SOC_TERM);
		}
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_TERM)
	{
		if (LOG_OUT_DST_IS_OPEN(LOG_OUT_SOC_HOST_TERM))
		{
			LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_HOST_TERM);
			LOG_OUT_DST_CLOSE(LOG_OUT_SOC_HOST_TERM);
		}
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_FILE)
	{
		if (LOG_OUT_DST_IS_OPEN(LOG_OUT_SOC_HOST_FILE))
		{
			/* TODO : we must implement the 'sync' evt handling mechanism between host/soc side

			LOG_TRACE("%s(): fclose() %s, fp = 0x%08x\n\r", __FUNCTION__, g_log_out.path, g_log_out.fp);
			memset(g_log_out.path, 0, LOG_MAX_PATH);
			fclose((FILE *)g_log_out.fp);
			g_log_out.fp = NULL;
			*/

			LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_HOST_FILE);
			LOG_OUT_DST_CLOSE(LOG_OUT_SOC_HOST_FILE);
			return true;
		}
	}
	// should never reach here!!
	return false;
}

bool	_soc_out_dst_turn_on(u8 _dst)
{
	ASSERT((_dst == LOG_OUT_SOC_TERM) || (_dst == LOG_OUT_SOC_HOST_TERM) || (_dst == LOG_OUT_SOC_HOST_FILE));
	if (_dst == LOG_OUT_SOC_TERM)
	{
		if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_TERM) == false)
			LOG_OUT_DST_CUR_ON(LOG_OUT_SOC_TERM);
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_TERM)
	{
		if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_HOST_TERM) == false)
			LOG_OUT_DST_CUR_ON(LOG_OUT_SOC_HOST_TERM);
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_FILE)
	{
		if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_HOST_FILE) == false)
		{
			/* TODO : we must implement the 'sync' evt handling mechanism between host/soc side
			if (g_log_out.fp == NULL)
			{
				LOG_ERROR("%s(): g_log_out.fp = NULL!", __FUNCTION__);
				return false;
			}
			*/
			LOG_OUT_DST_CUR_ON(LOG_OUT_SOC_HOST_FILE);
			return true;
		}
		return true;
	}
	// should never reach here!!
	return false;
}

bool	_soc_out_dst_turn_off(u8 _dst)
{
	ASSERT((_dst == LOG_OUT_SOC_TERM) || (_dst == LOG_OUT_SOC_HOST_TERM) || (_dst == LOG_OUT_SOC_HOST_FILE));
	if (_dst == LOG_OUT_SOC_TERM)
	{
		if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_TERM))
			LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_TERM);
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_TERM)
	{
		if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_HOST_TERM))
			LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_HOST_TERM);
		return true;
	}
	if (_dst == LOG_OUT_SOC_HOST_FILE)
	{
		if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_HOST_FILE))
		{
			/* TODO : we must implement the 'sync' evt handling mechanism between host/soc side
			if (g_log_out.fp == NULL)
			{
				LOG_ERROR("%s(): g_log_out.fp = NULL!", __FUNCTION__);
				return false;
			}
			*/
			LOG_OUT_DST_CUR_OFF(LOG_OUT_SOC_HOST_FILE);
			return true;
		}
		return true;
	}
	// should never reach here!!
	return false;
}


void soc_printf(u32 n, u32 m, const char *fmt, ...)
{
#if (CONFIG_LOG_ENABLE == 1)
    #define BUF_ROOM(buf_ptr, cur_ptr)  (sizeof(buf_ptr) - (cur_ptr - buf_ptr))

	va_list args;
	char *prn_buf = s_prn_buf;
	s32 len = 0;
	s32 total_len;

	if (   !LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_TERM)
	    #ifdef ENABLE_LOG_TO_HOST
	    && !LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_HOST_TERM)
	    #endif // ENABLE_LOG_TO_HOST
           )
	    return;

	if (!g_log_prnf_cfg.chk_tag_sprs)
	{
		if ((n < g_log_prnf_cfg.lvl) || ((g_log_prnf_cfg.mod & LOG_MODULE_MASK(m)) == 0))
		{
			return;
		}
	}

    // Form output string into the global print buffer
	LOG_MUTEX_LOCK();

	if (!g_log_prnf_cfg.prn_tag_sprs)
	{
		if (g_log_prnf_cfg.prn_tag_lvl)
		{
			if ((g_log_lvl_tag[n] != NULL) && (g_log_lvl_tag[n][0] != 0))
			{
				// len = snprintf(s_tag_lvl, sizeof(s_tag_lvel), "[%-5s] ", g_log_lvl_tag[n]);
				//len = snprintf(prn_buf, BUF_ROOM(s_prn_buf, prn_buf), "[%-5s] ", g_log_lvl_tag[n]);
				len = snputstr(prn_buf, BUF_ROOM(s_prn_buf, prn_buf), g_log_lvl_tag[n], LOG_LVL_TAG_LEN);
				if (len > 0)
				{
				    prn_buf += len;
				}
				else
				    ASSERT(len <= 0);
				// p += strlen(p);
			}
		}
		if (g_log_prnf_cfg.prn_tag_mod)
		{
			if ((g_log_mod_tag[m] != NULL) && (g_log_mod_tag[m][0] != 0))
			{
				//len = snprintf(s_tag_mod, sizeof(s_tag_mod), "<%-5s> ", g_log_mod_tag[m]);
				//len = snprintf(prn_buf, BUF_ROOM(s_prn_buf, prn_buf), "<%-5s> ", g_log_mod_tag[m]);
				len = snputstr(prn_buf, BUF_ROOM(s_prn_buf, prn_buf), g_log_mod_tag[m], LOG_MOD_TAG_LEN);
				if (len > 0)
				    prn_buf += len;
				else
				    ASSERT(len <= 0);
			}
		}
	}

	va_start(args, fmt);
	//vsnprintf(s_prn_tmp, sizeof(s_prn_tmp), fmt, args);
	len = vsnprintf(prn_buf, BUF_ROOM(s_prn_buf, prn_buf), fmt, args);
	prn_buf += len;
	total_len = prn_buf - s_prn_buf;
	// Make sure that the result is a valid null-terminated string.
	ASSERT(total_len < sizeof(s_prn_buf));
	ASSERT(*prn_buf == 0);
	va_end(args);

	LOG_MUTEX_UNLOCK();

	// Output the formed string.
	if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_TERM))	
		putstr(s_prn_buf, total_len);
	#ifdef ENABLE_LOG_TO_HOST
	if (LOG_OUT_DST_IS_CUR_ON(LOG_OUT_SOC_HOST_TERM))
		_log_soc_send_evt_prn_buf((const char *)s_prn_buf, total_len);
	 #endif // ENABLE_LOG_TO_HOST
#endif // CONFIG_LOG_ENABLE
}


// 1. always print out, skip the influence of 'level' & 'module' settings.
// 2. always print out file & line info.
void soc_fatal(u32 m, const char *file, u32 line, const char *fmt, ...)
{
#if (CONFIG_LOG_ENABLE == 1)
	va_list args;

	//memset(s_acc_buf, 0, ACC_BUF_MAX);

	LOG_MUTEX_LOCK();
	va_start(args, fmt);
	
	printf("[%-5s] ", g_log_lvl_tag[LOG_LEVEL_FATAL]);
	if (strcmp(g_log_mod_tag[m], "") != 0)
		printf("<%-5s> ", g_log_mod_tag[m]);

	printf("(%s #%4u) ", _extract_filename(file), line);

	va_start(args, fmt);
	print(0, 0, fmt, args);
	va_end(args);
	printf("\nprogram halt!!!\n");
	LOG_MUTEX_UNLOCK();
	halt();
#endif
}

void _log_evt_prnf_dump(log_evt_prnf_st *prnf)
{
	u32		i;
	u32		arg_cnt;

	log_printf("< %s >\n", __FUNCTION__);
	log_printf("len     = %d\n", prnf->len);
	log_printf("lvl     = %d\n", prnf->lvl);
    log_printf("mod     = %d\n", prnf->mod);
	_log_prnf_cfg_dump(&(prnf->prnf_cfg));

	log_printf("fmt_len = %d, offsetof = %d\n", prnf->fmt_len, sizeof(log_evt_prnf_st));
	log_printf("arg_len = %d\n", prnf->arg_len);
	
	arg_cnt = prnf->arg_len / 4;
	log_printf("arg_cnt = %d\n", arg_cnt);

	memcpy(&i, prnf->buf, sizeof(u32));	// just for removing compile warnings in soc toolchain
	log_printf("fmt : 0x%08x\n", i);

	for (i=0; i<arg_cnt; i++)			// including fmt, hence, +4
		log_printf("%03d : 0x%08x\n", i, *(int *)(prnf->buf + 4 + i*4));
}


#ifdef ENABLE_LOG_TO_HOST
void	_log_soc_send_evt_prnf_cfg_set(const u8 *p)
{
	u16				len;
	MsgEvent		*msg_evt;
	log_soc_evt_st	*soc_evt;

	len = sizeof(log_soc_evt_st) + sizeof(log_evt_prnf_cfg_set_st);
	LOG_EVENT_ALLOC(msg_evt, SOC_EVT_LOG, len);
	
	soc_evt = (log_soc_evt_st *)(LOG_EVENT_DATA_PTR(msg_evt));
	memset(soc_evt, 0, len);
	soc_evt->len = len;
	soc_evt->cmd = LOG_SEVT_PRNF_CFG_SET;

	memcpy(&(soc_evt->data), p, sizeof(log_evt_prnf_cfg_set_st));	
	LOG_EVENT_SET_LEN(msg_evt, len);
#if 0
	log_printf("%s()<=\n", __FUNCTION__);
	log_printf("soc_evt->len = %d\n", soc_evt->len);
	ssv6xxx_raw_dump((s8 *)soc_evt, soc_evt->len);
	log_printf("%s()=>\n", __FUNCTION__);
#endif
	LOG_EVENT_SEND(msg_evt);

	return;
}

void _t_printf(u32 n, u32 m, const char *fmt, ...)
{
	log_evt_prnf_st	*prnf;
	MsgEvent		*msg_evt;
	log_soc_evt_st	*soc_evt;
	va_list			args;
	u32				p, q;
	u32				arg_cnt;
	bool			touch_magic;
	u32				i;

	//printf("%s()<=, n=%d, m=%d\n\r", __FUNCTION__, n, m);
	
	//=> filter the level & mod af first
	if (!g_log_prnf_cfg.chk_tag_sprs)
	{
		if ((n < g_log_prnf_cfg.lvl) || ((g_log_prnf_cfg.mod & LOG_MODULE_MASK(m)) == 0))
		{
			return;
		}
	}
	//=> msg evt alloc & setting
	LOG_EVENT_ALLOC(msg_evt, SOC_EVT_LOG, LOG_EVT_PRNF_MAXLEN);
	soc_evt = (log_soc_evt_st *)LOG_EVENT_DATA_PTR(msg_evt);
	memset(soc_evt, 0, LOG_EVT_PRNF_MAXLEN);
	LOG_EVENT_SET_LEN(msg_evt, LOG_EVT_PRNF_MAXLEN);

	prnf = (log_evt_prnf_st *)(&(soc_evt->data));

	//=> format string info
	prnf->fmt_len	 = 4;
	*(prnf->buf)		 = (u8)((u32)fmt);
	*(prnf->buf + 1) = (u8)((u32)fmt >>  8);
	*(prnf->buf + 2) = (u8)((u32)fmt >> 16);
	*(prnf->buf + 3) = (u8)((u32)fmt >> 24);
	//printf("fmt : 0x%08x, buf[] = 0x%02x%02x%02x%02x\n", (u32)fmt, prnf->buf[0], prnf->buf[1], prnf->buf[2], prnf->buf[3]);
	//=> arg list
	va_start(args, fmt);
	q = 0;	touch_magic = false;	arg_cnt = 0;
	while (arg_cnt < LOG_DMSG_MAX_ARGS)
	{
		i = prnf->fmt_len + arg_cnt*4;
		p = (u32)va_arg(args, u32);
		if (p == (u32)g_magic_str)
		{
			//printf("%03d : %s (magic str!!)\n", arg_cnt, p);
			q = (u32)va_arg(args, u32);		// lookahead for g_magic_u32
			if (q == g_magic_u32)
			{
				touch_magic = true;
				break;
			}
			else
			{
				prnf->buf[i]   = (u8)(p);
				prnf->buf[i+1] = (u8)(p >>  8);
				prnf->buf[i+2] = (u8)(p >> 16);
				prnf->buf[i+3] = (u8)(p >> 24);
				//printf("%03d : 0x%08x, buf[] = 0x%02x%02x%02x%02x\n", arg_cnt, p, prnf->buf[i], prnf->buf[i+1], prnf->buf[i+2], prnf->buf[i+3]);
				arg_cnt++;

				prnf->buf[i]   = (u8)(q);	
				prnf->buf[i+1] = (u8)(q >>  8);	
				prnf->buf[i+2] = (u8)(q >> 16);	
				prnf->buf[i+3] = (u8)(q >> 24);	
				//printf("%03d : 0x%08x, buf[] = 0x%02x%02x%02x%02x\n", arg_cnt, q, prnf->buf[i], prnf->buf[i+1], prnf->buf[i+2], prnf->buf[i+3]);
				arg_cnt++;
				q = 0;
			}
		}
		else
		{
			prnf->buf[i]	   = (u8)(p);		
			prnf->buf[i+1] = (u8)(p >>  8);		
			prnf->buf[i+2] = (u8)(p >> 16);		
			prnf->buf[i+3] = (u8)(p >> 24);		
			//printf("%03d : 0x%08x, buf[] = 0x%02x%02x%02x%02x\n", arg_cnt, p, prnf->buf[i], prnf->buf[i+1], prnf->buf[i+2], prnf->buf[i+3]);
			arg_cnt++;			
		}
	};
	va_end(args);

	if (!touch_magic)	
	{
		//printf("%s()=>: not touch_magic !! return\n", __FUNCTION__);
		LOG_EVENT_FREE(msg_evt);
		return;
	}
	prnf->arg_len	= arg_cnt*4;
	//=> cal the total length
	prnf->len		= prnf->fmt_len + prnf->arg_len + sizeof(log_evt_prnf_st);

	//=> lvl & mod
	prnf->lvl = n;
	prnf->mod = m;

	//=> prnf cfg
	// memcpy(&(prnf->prnf_cfg), &g_log_prnf_cfg, sizeof(log_prnf_cfg_st));
    prnf->prnf_cfg.lvl			= g_log_prnf_cfg.lvl;
    prnf->prnf_cfg.mod			= g_log_prnf_cfg.mod;
    prnf->prnf_cfg.fl			= g_log_prnf_cfg.fl;
    prnf->prnf_cfg.prn_tag_lvl  = g_log_prnf_cfg.prn_tag_lvl;
    prnf->prnf_cfg.prn_tag_mod  = g_log_prnf_cfg.prn_tag_mod;
    prnf->prnf_cfg.prn_tag_sprs = g_log_prnf_cfg.prn_tag_sprs;
    prnf->prnf_cfg.chk_tag_sprs = g_log_prnf_cfg.chk_tag_sprs;

	//_log_evt_prnf_dump(prnf);
	//ssv6xxx_raw_dump((s8 *)prnf, prnf->len);

	// => cal soc evt's length & set cmd id
	soc_evt->len = prnf->len + LOG_SEVT_CMD_DATA_OFFSET;
	soc_evt->cmd = LOG_SEVT_PRNF;

	LOG_EVENT_SEND(msg_evt);
	//printf("%s()=>: \n", __FUNCTION__);
	return;
}
#endif // ENABLE_LOG_TO_HOST



