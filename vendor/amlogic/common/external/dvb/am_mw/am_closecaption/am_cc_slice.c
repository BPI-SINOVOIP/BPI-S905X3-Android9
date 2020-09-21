/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
/*
**
**
**
*/


/*===============================================================
*                    include files
*===============================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>  /* uint16_t */
#include <sys/types.h> /* size_t, ssize_t */
#include <unistd.h>
#include <pthread.h>
#include <am_types.h>
#include "am_cc_decoder.h"
#include "am_debug.h"
#include "am_vbi.h"
#include "am_cc_slice.h"

#if 0
#undef AM_DEBUG
#define AM_DEBUG(level,arg...) printf(arg)
#endif

static void am_vbi_slice_no_mem_exit(void)
{
	AM_DEBUG(1,"\n+++++++++out of memory +++++++\n");
}
static void am_vbi_slice_premature_exit(void)
{
        AM_DEBUG(1,"\n+++++++++Premature end of input file.++++++++\n");
}
static void bad_format_exit(void)
{
        AM_DEBUG(1, "\n++++++++Invalid data in input file.+++++++++\n");
}

void read_error_exit(const char * msg)
{
        if (NULL == msg)
                msg = strerror (errno);

        AM_DEBUG(1,"Read error: %s.", msg);
}

void write_error_exit(const char * msg)
{
        if (NULL == msg)
                msg = strerror (errno);

        AM_DEBUG(1,"Read error: %s.", msg);
}

static AM_Bool_t read_more(AM_VBI_Slice_Stream_t * st)
{
        unsigned int retry;
        uint8_t *s;
        uint8_t *e;

        s = /* const cast */ st->end;
        e = st->buffer + sizeof (st->buffer);
        AM_DEBUG(1,"111read_more %d \n",sizeof (st->buffer));
        if (s >= e)
                s = st->buffer;

        //**********************************get buffer way***********
        return AM_TRUE;
        //**********************************finish**********
        retry = 100;

        do {
                ssize_t actual;
                                int saved_errno;

                actual = read (st->fd, s, e - s);
                if (0 == actual)
                        return AM_FALSE; /* EOF */
                AM_DEBUG(1,"111read_more actual %d \n",actual);
                if (actual > 0) {
                        st->bp = s;
                        st->end = s + actual;
                        return AM_TRUE;
                }

                saved_errno = errno;

                if (EINTR != saved_errno) {
                        read_error_exit (/* msg: errno */ NULL);
                }
        } while (--retry > 0);

        read_error_exit ( NULL);

        return AM_FALSE;
}

static AM_Bool_t am_vbi_slice_next_byte(AM_VBI_Slice_Stream_t *        st,
          		 int *                  c)
{
        do {

                if (st->bp < st->end) {
                        *c = *st->bp++;
                        AM_DEBUG(1,"am_vbi_slice_next_byte  %x\n",*c);
                        return AM_TRUE;
                }
        } while (read_more (st));

        return AM_FALSE; /* EOF */
}

static void am_vbi_slice_next_block(AM_VBI_Slice_Stream_t *	st,
                                    uint8_t *	buffer,
                                    unsigned int	buffer_size)
{
        do {
                unsigned int available;

                available = st->end - st->bp;

                AM_DEBUG(1,"%s -->available :%d ,buffer_size ;%d \n",__FUNCTION__,available,buffer_size);
                if (buffer_size <= available) {
                        memcpy (buffer, st->bp, buffer_size);
                        st->bp += buffer_size;
                        return;
                }

                memcpy (buffer, st->bp, available);

                st->bp += available;

                buffer += available;
                buffer_size -= available;

        } while (read_more (st));

        am_vbi_slice_premature_exit();
}

AM_VBI_Slice_Stream_t* vbi_slice_read_stream(const char *  fbuffer,
               		       int length,
                               unsigned int      ts_pid,
                               AM_VBI_SLICE_CALLBACK_FN * callback ,
                               void  *userdata)
{
        AM_VBI_Slice_Stream_t *st;

        st = calloc (1, sizeof (*st));
        
        if (NULL == st)
                am_vbi_slice_no_mem_exit ();

        st->callback            = callback;
        st->sample_time         = 0.0;
        st->stream_time         = 0;
        st->bp                  = st->buffer;
        st->end                 = st->buffer;

        AM_DEBUG(1,"finish vbi_slice_read_stream" );

        memset (st->buffer, 0,sizeof (st->buffer));
        memcpy (st->buffer,fbuffer, length);
        st->end  = st->buffer + length;
        st->userdata    = userdata;

        AM_DEBUG(1,"st->bp,st->end,length === 0x%x  0x%x   %d",st->bp,st->end,length);
        
	return st;
}


static AM_Bool_t vbi_slicce_process_stream(AM_VBI_Slice_Stream_t *st)
{
        AM_DEBUG(1,"\n========start %s \n",__FUNCTION__);
        for (;;) {

                AM_VBI_Sliced_t *s;
		double dt = 0;
		AM_Bool_t success;
                int n_lines;
                int count;

                /* Time in seconds since last frame. */
                if (dt < 0.0)
                        dt = -dt;
                st->sample_time += dt;

                //AM_DEBUG(1,"st->sample_time = %lf\n", st->sample_time);
                n_lines = (st->end - st->bp)<8 ? 1:(st->end - st->bp) / 8;   //8 is hardware struct length
                
                if ((unsigned int) n_lines > N_ELEMENTS (st->sliced))
                        n_lines = N_ELEMENTS (st->sliced);   //bad_format_exit ();
                if(n_lines == 0)
                        break;

                s = st->sliced;

                for (count = n_lines; count > 0; --count) {
                        int index;
                        int field_id;
                        int nbytes;
                        int line;
                        
                       /* no need parse vbi preamble data from VBI data due to the new VBI 
                       ** driver directly send two bytes cc playload to its ringbuf for app
                       */
  #if 0                       
                        if (!am_vbi_slice_next_byte(st, &index))
                               am_vbi_slice_premature_exit();
                        if (!am_vbi_slice_next_byte(st, &field_id))
                               am_vbi_slice_premature_exit();
                        if (!am_vbi_slice_next_byte(st, &nbytes))
                               am_vbi_slice_premature_exit();
                        if (!am_vbi_slice_next_byte(st, &nbytes))
                               am_vbi_slice_premature_exit();
                        s->line += (line & 15) * 256;
                       
                        if (!am_vbi_slice_next_byte(st, &line))
                               am_vbi_slice_premature_exit();
                        s->line = line;
                        
                        if (!am_vbi_slice_next_byte(st, &line))
                               am_vbi_slice_premature_exit();
                        s->line += (line & 15) * 256;

                        AM_DEBUG(1," index= 0x%x, field_id =0x%x ,nbytes =0x%x ,line = 0x%x \n,",index,field_id,nbytes,s->line);
                        //****************************************change if 284 = 21  else ....
                        if(s->line == 284)
                                s->line = 21;
                        else if(s->line == 21)
                                s->line = 284;
#endif
                     
                     index = 0 ;                        
                     switch (index) {
                        case 0:
                                s->id = VBI_SLICED_CAPTION_525;
                                am_vbi_slice_next_block(st, s->data, 2);
                                AM_DEBUG(1,"\ns->data[0]=0x%x ,s->data[1]=0x%x \n",s->data[0],s->data[1]);
                                break;

                        case 1:
                                s->id = VBI_SLICED_TELETEXT_B;
                                AM_DEBUG(1,"\n======process VBI_SLICED_TELETEXT_B=====\n");
                                break;

                        case 2:
                                s->id = VBI_SLICED_CAPTION_625;
                                AM_DEBUG(1,"\n======process VBI_SLICED_CAPTION_625 =====\n");
                                break;

                        case 3:
                                s->id = VBI_SLICED_VPS;
                                AM_DEBUG(1,"\n======process VBI_SLICED_VPS =====\n");
                                break;

                        case 4:
                                s->id = VBI_SLICED_WSS_625;
                                AM_DEBUG(1,"\n======process VBI_SLICED_WSS_625 =====\n");
                                break;

                        case 5:
                                s->id = VBI_SLICED_WSS_CPR1204;
                                AM_DEBUG(1,"\n======process VBI_SLICED_WSS_625 =====\n");
                                break;

                        case 255:
                                AM_DEBUG(1,"\n======process VBI_SLICED_RAW =====\n");
                                break;

                        default:
                                bad_format_exit ();
                                break;
                        }

                        ++s;
                }
                
                st->stream_time = st->sample_time * 90000;
                success = st->callback(st->sliced, 
                                       n_lines,
                                       st->sample_time,
                                       st->stream_time,
                                       st->userdata);

                if (!success)
                        return AM_FALSE;
        }

        return AM_TRUE;
}


void vbi_slice_delete_stream(AM_VBI_Slice_Stream_t *st)
{
       if (NULL == st)
           return;

        if (st->close_fd) {
            if (-1 == close (st->fd)) {
                   if (NULL != st->write_func)
                      write_error_exit ( NULL);
             }
        }

        CLEAR (*st);
        free (st);


}

AM_Bool_t decode_frame(const AM_VBI_Sliced_t *     s,
	             unsigned int           n_lines,
             	     double	sample_time,
             	     int64_t	stream_time,
                     void  *userdata)
{
        
        static double metronome = 0.0;
        static double last_sample_time = 0.0;
        static int64_t last_stream_time = 0;

        AM_DEBUG(1,"\n========start %s \n",__FUNCTION__);

#if 0
        if (option_dump_time || option_metronome_tick > 0.0) {
                /* Sample time: When we captured the data, in
                                seconds since 1970-01-01 (gettimeofday()).
                   Stream time: For ATSC/DVB the Presentation Time Stamp.
                                For analog the frame number multiplied by
                                the nominal frame period (1/25 or
                                1001/30000 s). Both given in 90 kHz units.
                   Note this isn't fully implemented yet. */

                if (option_metronome_tick > 0.0) {
                        AM_DEBUG(1,"ST %f (adv %+f, err %+f) PTS %"
                                PRId64 " (adv %+" PRId64 ", err %+f)\n",
                                sample_time, sample_time - last_sample_time,
                                sample_time - metronome,
                                stream_time, stream_time - last_stream_time,
                                (double) stream_time - metronome);

                        metronome += option_metronome_tick;
                } else {
                        AM_DEBUG(1,"ST %f (%+f) PTS %" PRId64 " (%+" PRId64 ")\n",
                                sample_time, sample_time - last_sample_time,
                                stream_time, stream_time - last_stream_time);
                }

                last_sample_time = sample_time;
                last_stream_time = stream_time;
        }
#endif
        AM_DEBUG(1,"decode_frame while\n");
        
       
        while (n_lines > 0) {
                switch (s->id) {
                case VBI_SLICED_TELETEXT_B_L10_625:
                case VBI_SLICED_TELETEXT_B_L25_625:
                case VBI_SLICED_TELETEXT_B_625:
                       //process  vbi teletext 
                       break;

                case VBI_SLICED_VPS:
                case VBI_SLICED_VPS_F2:
                       //process  vbi VPS
                       break;

                case VBI_SLICED_CAPTION_625_F1:
                case VBI_SLICED_CAPTION_625_F2:
                case VBI_SLICED_CAPTION_625:
                case VBI_SLICED_CAPTION_525_F1:
                case VBI_SLICED_CAPTION_525_F2:
                case VBI_SLICED_CAPTION_525:
                        AM_CC_Decode_Line21VBI(s->data[0],s->data[1],s->line);
                        break;

                case VBI_SLICED_WSS_625:
                        //process WSS_625
                        break;

                case VBI_SLICED_WSS_CPR1204:
                       //process WSS_CPR1204
                        break;
                }

                ++s;
                --n_lines;
        }

        return AM_TRUE;
}


AM_Bool_t  AM_NTSC_VBI_Decoder_Test(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
     int length =  len;
     AM_VBI_Slice_Stream_t *st;
    int idx = 0;

#if 0 //prevous cc slice parser has something wrong and straitly forward vbi playload data to CC decdoer.
     AM_DEBUG(1,"decode_vbi  len = %d\n",len);

     if(len < 0  || data == NULL)
        return AM_FALSE;

     st = vbi_slice_read_stream (data,length,0,decode_frame,NULL);

     vbi_slicce_process_stream(st);
     vbi_slice_delete_stream(st);
#else
    for(idx = 0; idx < len; idx+=2){
         AM_CC_Decode_Line21VBI(data[idx],data[idx+1],284);
    }
#endif

     return AM_TRUE;
}

