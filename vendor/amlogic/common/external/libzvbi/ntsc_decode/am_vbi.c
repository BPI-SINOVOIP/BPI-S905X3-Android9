#undef NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "vbi.h"
#include "exp-gfx.h"
#include "hamm.h"
#include "dvb_demux.h"
#include "sliced.h"
#include "sliced_vbi.h"
#include "am_vbi.h"
#include "vbi_dmx.h"

/********************define variable***************************/


typedef struct
{
	vbi_decoder       *dec;
	vbi_bool          cc_status;
	AM_VBI_CC_Para_t      cc_para;
	AM_VBI_XDS_Para_t     xds_para;
	int                page_no;
	int                sub_page_no;
	vbi_bool          disp_update;
	vbi_bool          running;
	uint64_t           pts;
	pthread_mutex_t    lock;
	pthread_cond_t     cond;
	pthread_t          thread;
}AM_VBI_Parser_t;

vbi_pgno		pgno = -1;

//AM_VBI_Parser_t *parser = NULL;

#if 0
static void
reset				(AM_VBI_Parser_t *parser)
{
	vbi_page page;
	vbi_bool success;
	int row;

	success = vbi_fetch_cc_page (parser->dec, &page, pgno, TRUE);
	assert (success);

	for (row = 0; row <= page.rows; ++row)
		render (parser,&page, row);

	vbi_unref_page (&page);
}
#endif
/**********************************************************/


 void vbi_cc_show(AM_VBI_Parser_t *parser)
{
	
	vbi_page page;
	vbi_bool success;
	int row;
	//user_data = user_data;

	//if (pgno != -1 && parser->page_no != pgno)
	//	return;

	/* Fetching & rendering in the handler
           is a bad idea, but this is only a test */
	AM_DEBUG("NTSC--------------------  vbi_cc_show****************\n");
	success = vbi_fetch_cc_page (parser->dec, &page, parser->page_no, TRUE);
	AM_DEBUG("NTSC--------------1212------vbi_fetch_cc_page  success****************\n");
	assert (success);
	
	 int i ,j;
	 if(parser->cc_para.draw_begin)
		 parser->cc_para.draw_begin(parser);
	
	vbi_draw_cc_page_region (&page, VBI_PIXFMT_RGBA32_LE, parser->cc_para.bitmap,
			parser->cc_para.pitch, 0, 0, page.columns, page.rows);
	
	 if(parser->cc_para.draw_end)
		 parser->cc_para.draw_end(parser);
	vbi_unref_page (&page);
}


static void* vbi_cc_thread(void *arg)
{
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)arg;

	pthread_mutex_lock(&parser->lock);
	AM_DEBUG("NTSC***************________________ vbi_cc_thread   parser->running = %d\n",parser->running);   
	while(parser->running)
	{
		AM_DEBUG("NTSC***************________________ vbi_cc_thread   disp_update = %d\n",parser->disp_update);   
		while(parser->running && !parser->disp_update){
			pthread_cond_wait(&parser->cond, &parser->lock);
		}
		AM_DEBUG("NTSC***************________________ vbi_cc_thread   disp_update = %d\n",parser->disp_update);  
		if(parser->disp_update){
			vbi_cc_show(parser);
			parser->disp_update = FALSE;
		}
	}
	pthread_mutex_unlock(&parser->lock);

	return NULL;
}

static void vbi_cc_handler(vbi_event *		ev,void *	user_data)
{
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)user_data;
	
	if(parser->page_no  == 0)
		return;
	//***************************************************temp add cc status 
	if(parser->cc_status == AM_TRUE)
	if(parser->page_no == ev->ev.caption.pgno){
		//parser->page_no = ev->ev.caption.pgno;
		parser->disp_update = AM_TRUE;
		pthread_cond_signal(&parser->cond);
		
		//vbi_cc_show(parser);
	}
}



	
	
	
	
	
	
static void vbi_xds_handler	(vbi_event *		ev, void *		user_data)
{
	AM_DEBUG("xds--------------------  xds_handler****************");
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)user_data;
	
	
	if(ev == NULL)
		return;
	vbi_program_info *	prog_info = ev->ev.prog_info;
	
	vbi_xds_subclass_program  xds_program = 0;
	switch(ev->type)
	{
		case VBI_EVENT_ASPECT :
			/* program identification number */
			if(!(prog_info->month == -1 && prog_info->day == -1 
				&& prog_info->hour == -1 && prog_info->min == -1 ))
			{
				xds_program = VBI_XDS_PROGRAM_ID;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_ID \n");
				break;
			}
			/* program name */
			if(!(prog_info->title[0] == 0))
			{
				xds_program = VBI_XDS_PROGRAM_NAME;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_NAME \n");
				break;
			}
			/* program aspect ratio */
			if(!(prog_info->aspect.first_line == prog_info->aspect.last_line == -1 &&  prog_info->aspect.ratio == 0.0))
			{
				xds_program = VBI_XDS_PROGRAM_ASPECT_RATIO;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_ASPECT_RATIO \n");
				break;
			}
		
		break;
		case VBI_EVENT_PROG_INFO :
			/* 02 Program Length */
			if(!(prog_info->length_hour == -1 && prog_info->length_min == -1 &&prog_info->elapsed_hour == -1 &&
				prog_info->elapsed_min == -1 &&prog_info->elapsed_sec == -1 ))
			{
				xds_program = VBI_XDS_PROGRAM_LENGTH;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_LENGTH \n");
				break;
			}
		
			/* 04 Program type */
			/*
			 *  If unknown type_classf == VBI_PROG_CLASSF_NONE.
			 *  VBI_PROG_CLASSF_EIA_608 can have up to 32 tags
			 *  identifying 96 keywords. Their numerical value
			 *  is given here instead of composing a string for
			 *  easier filtering. Use vbi_prog_type_str_by_id to
			 *  get the keywords. A zero marks the end.
			 */
			if(!(prog_info->type_classf == VBI_PROG_CLASSF_NONE))
			{
				xds_program = VBI_XDS_PROGRAM_TYPE;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_TYPE \n");
				break;
			}
			
			/* 05 Program rating */
			/*
			 *  For details STFW for "v-chip"
			 *  If unknown rating_auth == VBI_RATING_NONE
			 */
			if(!(prog_info->rating.auth == VBI_RATING_AUTH_NONE))
			{
				xds_program = VBI_XDS_PROGRAM_RATING;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_RATING \n");
				break;
			}
			
			/* 06 Program Audio Services */
			/*
			 *  BTSC audio (two independent tracks) is flagged according to XDS,
			 *  Zweiton/NICAM/EIA-J audio is flagged mono/none, stereo/none or
			 *  mono/mono for bilingual transmissions.
			 */
			if((prog_info->audio[0].mode != VBI_AUDIO_MODE_UNKNOWN) ||
				 (prog_info->audio[1].mode != VBI_AUDIO_MODE_UNKNOWN)) 
			{
				xds_program = VBI_XDS_PROGRAM_AUDIO_SERVICES;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_AUDIO_SERVICES \n");
				break;
			}
			
			/* 07 Program Caption Services */
			/*
			 *  Bits 0...7 corresponding to Caption page 1...8.
			 *  Note for the current program this information is also
			 *  available via vbi_classify_page().
			 *
			 *  If unknown caption_services == -1, _language[] = NULL
			 */
			if(!(prog_info->caption_services  == -1))
			{
				xds_program = VBI_XDS_PROGRAM_CAPTION_SERVICES;
				AM_DEBUG("xds*********VBI_XDS_PROGRAM_CAPTION_SERVICES \n");
				break;
			}
		break;	
		
	}	
	
	if(prog_info->rating.auth != VBI_RATING_AUTH_NONE)
	{
			AM_DEBUG("xds**********prog_info->rating_auth  = %d\n",prog_info->rating.auth );
	
			if(prog_info->rating.auth == VBI_RATING_AUTH_MPAA)
					AM_DEBUG("xds**********result*******************VBI_RATING_AUTH_MPAA\n");
			if(prog_info->rating.auth == VBI_RATING_AUTH_TV_US)
					AM_DEBUG("xds**********result*******************VBI_RATING_AUTH_TV_US\n");
			if(prog_info->rating.auth == VBI_RATING_AUTH_TV_CA_EN)
					AM_DEBUG("xds**********result*******************VBI_RATING_AUTH_TV_CA_EN\n");
			if(prog_info->rating.auth == VBI_RATING_AUTH_TV_CA_FR)
					AM_DEBUG("xds**********result*******************VBI_RATING_AUTH_TV_CA_FR\n");
			if(prog_info->rating.auth == VBI_RATING_AUTH_NONE)
					AM_DEBUG("result*******************VBI_RATING_AUTH_NONE\n");
			
			if (prog_info->rating.dlsv == VBI_RATING_D)
				AM_DEBUG("xds**********result*******************VBI_RATING_D\n");
			if (prog_info->rating.dlsv == VBI_RATING_L)
				AM_DEBUG("xds**********result*******************VBI_RATING_L\n");
			if (prog_info->rating.dlsv == VBI_RATING_S)
				AM_DEBUG("xds**********result*******************VBI_RATING_S\n");
			if (prog_info->rating.dlsv == VBI_RATING_V)
				AM_DEBUG("xds**********result*******************VBI_RATING_V\n");
			AM_DEBUG("xds**********result*******************prog_info->rating_id = %d\n",prog_info->rating.id );
			
	}
		
	parser->xds_para.xds_callback(parser,xds_program,*prog_info);
	
}

/**********************************************************/

static vbi_bool
decode_frame			(const vbi_sliced *	sliced,
				 unsigned int		n_lines,
				 const uint8_t *	raw,
				 const vbi_sampling_par *sp,
				 double			sample_time,
				 int64_t		stream_time,
				 void *	user_data)
{
	AM_DEBUG("NTSC--------------------  decode_frame\n");
	if(user_data == NULL) return FALSE;
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)user_data;
	raw = raw;
	sp = sp;
	stream_time = stream_time; /* unused */

	vbi_decode (parser->dec, sliced, n_lines, sample_time);
	return TRUE;
}



 vbi_bool
decode_vbi		(int dev_no, int fid, const uint8_t *data, int len, void *user_data){

	AM_DEBUG("NTSC--------------------  decode_vbi  len = %d\n",len);
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)user_data;
	if(user_data == NULL)	
		AM_DEBUG("NTSC--------------------  decode_vbi NOT user_data ");
	
    int length =  len;
	struct stream *st;
	if(len < 0  || data == NULL)
		goto error;
	st = read_stream_new (data,length,FILE_FORMAT_SLICED,
					0,decode_frame,parser);
	
	stream_loop (st);
	stream_delete (st);
	return AM_SUCCESS;
	
	error:
		return AM_FAILURE;
  
}

  void init_vbi_decoder( AM_VBI_Parser_t* parser)
  {	
		memset(parser, 0, sizeof(AM_VBI_Parser_t));
		parser->dec = vbi_decoder_new ();
		assert (NULL != parser->dec);
		pthread_mutex_init(&parser->lock, NULL);
		pthread_cond_init(&parser->cond, NULL);
  }




vbi_bool AM_VBI_CC_Create(AM_VBI_Handle_t *handle, AM_VBI_CC_Para_t *para)
{
	AM_VBI_Parser_t* parser = NULL;
	if(*handle == NULL)
	{
		
		parser = (AM_VBI_Parser_t*)malloc(sizeof(AM_VBI_Parser_t));
		if(!parser)
		{
			return AM_VBI_DMX_ERR_NOT_ALLOCATED;
		}
		init_vbi_decoder(parser);
	}else
		parser = (AM_VBI_Parser_t*)handle;
	
	vbi_bool success;
	success = vbi_event_handler_add (parser->dec, VBI_EVENT_CAPTION,
				 vbi_cc_handler, parser);
				 
	//******************************************************
	//success = vbi_event_handler_add (parser->dec, VBI_EVENT_ASPECT | VBI_EVENT_PROG_INFO,
	//			 vbi_xds_handler, parser);
	//************************************************
	assert (success);
	parser->cc_para    = *para;
	*handle = parser;
	return AM_SUCCESS;
}


 AM_ErrorCode_t AM_VBI_CC_Start(AM_VBI_Handle_t handle)
{
	AM_DEBUG("NTSC--------------------  ******************AM_NTSC_CC_Start \n");
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)handle;
	vbi_bool ret = AM_SUCCESS;
	
	//*******************************************************temp
	parser->cc_status = AM_TRUE;
	//*******************************************************temp
	if(!parser)
	{	
		AM_DEBUG("NTSC--------------------  ******************AM_CC_ERR_INVALID_HANDLE \n");
		return AM_CC_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);
	
	if(!parser->running)
	{
		parser->running = AM_TRUE;
		if(pthread_create(&parser->thread, NULL, vbi_cc_thread, parser))
		{
			parser->running = AM_FALSE;
			ret = AM_CC_ERR_CANNOT_CREATE_THREAD;
		}
	}
	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}





/********************************************************/
AM_ErrorCode_t AM_VBI_CC_Stop(AM_VBI_Handle_t handle)
{
	AM_DEBUG("NTSC--------------------  ******************AM_VBI_CC_Stop \n");
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)handle;
	vbi_bool ret = AM_SUCCESS;

	pthread_t th;
	vbi_bool wait = AM_FALSE;

	if(!parser)
	{
		AM_DEBUG("NTSC--------------------  ******************AM_CC_ERR_INVALID_HANDLE \n");
		return AM_CC_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);

	if(parser->running)
	{
		parser->running = AM_FALSE;
		wait = AM_TRUE;
		th = parser->thread;
	}

	pthread_mutex_unlock(&parser->lock);
	pthread_cond_signal(&parser->cond);

	if(wait)
	{
		pthread_join(th, NULL);
	}

	return AM_SUCCESS;
}


void* AM_VBI_CC_GetUserData(AM_VBI_Handle_t handle)
{
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)handle;
	if(!parser)
	{
		return NULL;
	}
	return parser->cc_para.user_data;
}

vbi_bool AM_VBI_XDS_Create(AM_VBI_Handle_t *handle,AM_VBI_XDS_Para_t *para)
{
	vbi_bool ret = AM_SUCCESS;
	AM_VBI_Parser_t* parser;
	if(*handle == NULL)
	{
		AM_DEBUG("NTSC--------------------  ******************AM_XDS_Create  handle is null \n");
		parser = (AM_VBI_Parser_t*)malloc(sizeof(AM_VBI_Parser_t));
		if(!parser)
		{
			return AM_VBI_DMX_ERR_NOT_ALLOCATED;
		}
		init_vbi_decoder(parser);
		*handle = parser;
	}else
		parser = (AM_VBI_Parser_t*)*handle;
	parser->xds_para =  *para;
	AM_DEBUG("NTSC--------------- ******************vbi_event_handler_add  xds_handle \n");
	ret = vbi_event_handler_add (parser->dec, VBI_EVENT_ASPECT | VBI_EVENT_PROG_INFO,
				 vbi_xds_handler, parser);
				 
	assert (ret);
	return AM_SUCCESS;
	
}


vbi_bool AM_VBI_CC_set_type(AM_VBI_Handle_t handle,VBI_CC_TYPE cc_type)
{
	AM_DEBUG("NTSC--------------------  ******************AM_VBI_CC_set_type ===%d\n",cc_type);
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)handle;
	
	if(cc_type < 1 || cc_type >8)
		return AM_FAILURE;
	else
		parser->page_no = cc_type;
		return AM_SUCCESS;
}

vbi_bool AM_VBI_CC_set_status(AM_VBI_Handle_t handle,vbi_bool flag)
{
	AM_DEBUG("NTSC--------------------  ******************AM_VBI_CC_set_status ===%d\n");
	AM_VBI_Parser_t *parser = (AM_VBI_Parser_t*)handle;
	if(flag == AM_TRUE)
		parser->cc_status = AM_TRUE;
	else
	if(flag == AM_FAILURE)
		parser->cc_status = AM_FAILURE;
	return AM_SUCCESS;
	
}

