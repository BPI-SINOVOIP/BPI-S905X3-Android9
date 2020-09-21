/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <am_vbi.h>
#include <am_debug.h>
#include "am_vbi_internal.h"
#include "am_xds.h"
#include "am_cc_decoder.h"

#include <am_fend.h>
#include <am_db.h>


#define STEP_STMT(_stmt, _name, _sql)\
	AM_MACRO_BEGIN\
		int ret1, ret2;\
			ret1 = sqlite3_step(_stmt);\
			ret2 = sqlite3_reset(_stmt);\
			if (ret1 == SQLITE_ERROR && ret2 == SQLITE_SCHEMA){\
				printf("\n[%s:%d]Database schema changed, now re-prepare the stmts...",__FUNCTION__,__LINE__);\
				AM_DB_GetSTMT(&(_stmt), _name, _sql, 1);\
			}\
	AM_MACRO_END

static unsigned char fd1_closed = 0;
static unsigned int buf_size1 = 1024;
static  unsigned int  cc_type1 = 1;
static unsigned int exit_cnt1 = 20;
static int afe_fd1 = -1;
FILE *wr_fp;
char vbuf_addr1[2000];

extern void AM_Print_XDSDataInfo(void);
extern AM_ErrorCode_t AM_FEND_GetLastPara(struct dvb_frontend_parameters *para);
static void demux_vbi_start();
static void am_xds_store_process(void);

int main(int argc, char *argv[])
{
#if 1
	//demux_vbi_start();
        am_xds_store_process();
	return 0;
#else
    int    ret    = 0;
    unsigned int i = 0;
    pthread_t pid = 0;

 
    char name_buf[3] = {0};

    fd_set rds1,rds2;
    struct timeval tv1,tv2;
    
    afe_fd1 = open("/dev/vbi", O_RDWR|O_NONBLOCK);
    if (afe_fd1 < 0) {
        printf("open device 1 file Error!!! \n");
        return -1;
    }
    printf("open device 1 file  \n");

//while (1) {
    //ioctl(afe_fd, VBI_IOC_CC_EN);
    //sleep(2);
    //printf("input cc type value:\n");
    //scanf("%d", &cc_type);
    //printf("input cc type1 is:%d \n", cc_type1);
    
    ret = ioctl(afe_fd1, VBI_IOC_S_BUF_SIZE, &buf_size1);
    if (ret < 0) {
        printf("set afe_fd1 ioctl VBI_IOC_S_BUF_SIZE Error!!! \n");
        return 0;
    }
    //printf("set afe_fd1 ioctl VBI_IOC_S_BUF_SIZE   \n");
    
    ret = ioctl(afe_fd1, VBI_IOC_SET_FILTER, &cc_type1);//0,1
    if (ret < 0) {
        printf("set afe_fd1 ioctl VBI_IOC_SET_FILTER Error!!! \n");
        return 0;
    }
    //printf("set afe_fd1 ioctl VBI_IOC_SET_FILTER  \n");

    ret = ioctl(afe_fd1, VBI_IOC_START);
    if (ret < 0) {
        printf("set afe_fd1 ioctl VBI_IOC_START Error!!! \n");
        return 0;
    }
    //printf("set afe_fd1 ioctl VBI_IOC_START  \n");
       	//ioctl(afe_fd1, VBI_IOC_STOP);
        //	close(afe_fd1);
        ///	printf(" close afe !!!.\n");

	 while (1) {   

		//printf("input exit_cnt, zero for exit:\n");
		//scanf("%d", &exit_cnt);
		FD_ZERO(&rds1);
		FD_SET(afe_fd1, &rds1);
		/* Wait up to five seconds. */
		tv1.tv_sec = 1;
		tv1.tv_usec = 0;    


		printf("\n input exit_cnt1 is:%d \n", exit_cnt1);
               
		if (exit_cnt1 == 0) {
			if (!fd1_closed) {
				fd1_closed = 1;
				ioctl(afe_fd1, VBI_IOC_STOP);
				close(afe_fd1);
				printf("exit_cnt1 is :%d eixt buffer capture function!!!.\n", exit_cnt1);
			}
				return 0;
		}

		 ret = select(afe_fd1 + 1, &rds1, NULL, NULL, &tv1);
		if(ret < 0) {
			printf("afe_fd1 Read data Faild!......\n");
			break;
		} else if(ret == 0) {
			printf("afe_fd1 Read data Device Timeout!......\n");
			exit_cnt1--;
			//continue;
		} else if (FD_ISSET(afe_fd1, &rds1)) {
			/* write data to file */
			ret = read(afe_fd1, vbuf_addr1, 5);
			if  (ret <=  0) {
				printf("afe_fd1 read file error: ret=%x!!!! \n", ret);
				//close(afe_fd);
				//return 0;
			} else {
				printf("\n afe_fd1 read %2d bytes, exit_cnt1:%2d \n",ret, exit_cnt1);
				for (i=0; i<ret; i++) {
					printf("0x%02x,",vbuf_addr1[i]);
					if ((i > 0) && ((i+1)%8 == 0)){
						printf("\n");
					}
				}
				exit_cnt1--;
				//printf(" \nafe_fd1 finish read  5 buffer data \n");
			} 
		}
	}	
	fb:
	return 0;
#endif

}

static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{

        int i=0;
	char font_buf[64];
	int font_idx = 0;
        uint8_t *pdata = data ;

	printf("\ndump_bytes  len = %d\n",len);
        AM_NTSC_VBI_Decoder_Test( dev_no,  fid, data,  len,  user_data);
      
#if 0  
         for (i=0;i<len;i++) {
                printf("0x%2x ",pdata[i]);
                pdata[i]&=0x7F;
                if(pdata[i] > 0x20 && pdata[i] < 0x80){
                    if(font_idx == 64){
                        font_idx = 0;
                        printf("\nString:\n%s.\n", font_buf);
                    }
                    font_buf[font_idx] = pdata[i];
                    font_idx++;
                }
                if (i == 0){
                    if ((i % 7 == 0) && (i != 0) )
                        printf("\n");
                } else {
                    if ((i % 8 == 0) )
                        printf("\n");
                }
        }
#endif

}

static void demux_vbi_start()
{       
        int running = 1;
        char buf[256];

	int fid;
	AM_VBI_OpenPara_t para;
	memset(&para, 0, sizeof(para));
	printf("\n*******************************\n");

        AM_Set_CC_Country(CMD_SET_COUNTRY_USA);
        AM_Set_Cur_CC_Type(CMD_CC_1);       
 
        if(AM_Initialize_XDSDataServices() != AM_SUCCESS) {
              printf("###############AM_Initialize_XDSDataServices fail!\n\n");
          }
	

	if(AM_NTSC_VBI_Open(VBI_DEV_NO, &para) != AM_SUCCESS) {
	    printf("###############AM_DMX_Open fail!\n\n");
	}
	
	if( AM_SUCCESS != AM_NTSC_VBI_AllocateFilter(VBI_DEV_NO, &fid)){
            printf("\n+++++++++failure to allocate filter\n");  
        }
	
        if( AM_SUCCESS != AM_NTSC_VBI_SetCallback(VBI_DEV_NO, fid, dump_bytes, NULL)){
           printf("\n+++++++++failure to SetCallback\n");
        }
	
	if( AM_SUCCESS != AM_NTSC_VBI_SetBufferSize(VBI_DEV_NO, fid, 1024)){
           printf("\n+++++++++failure to SetBufferSize\n");
        }

	if( AM_SUCCESS != AM_NTSC_VBI_StartFilter(VBI_DEV_NO, fid)){
           printf("\n+++++++++failure to StartFilter\n");
        }
         
        while(running)
        {
                printf("********************\n");
                printf("* commands:\n");
                printf("* quit\n");
                printf("********************\n");

                if(fgets(buf, 256, stdin))
                {
                        if(!strncmp(buf, "quit", 4))
                        {
                                running = 0;
                                sleep(1);
                                AM_NTSC_VBI_StopFilter(VBI_DEV_NO,fid);
			        AM_NTSC_VBI_FreeFilter(VBI_DEV_NO,fid);

                                if(AM_NTSC_VBI_Close(VBI_DEV_NO)!= AM_SUCCESS) {
                                     return;
                                }
                                
                                AM_Terminate_XDSDataServices();
                        }
                        else if(!strncmp(buf, "xds", 3)){
                               AM_Print_XDSDataInfo();
                        }
                        else
                        {
                                sleep(2);
                        }
                }
        }
}

static AM_ErrorCode_t xds_get_svr_si_table_items(sqlite3 *sqldb,int *db_ts_id,int *srv_db_id,int *source_id,int *src)
{
   AM_ErrorCode_t ret = AM_SUCCESS;
   int row = 1;
   int ts_dbid = -1,srv_dbid =-1,src_id = -1,tmp_source_id =-1;
   char sql[256];
   int major_chan_num = -1,minor_chan_num =-1,default_chan_num = -1;
   char namestring[256];

   struct dvb_frontend_parameters fparam;
   
   ret = AM_FEND_GetLastPara(&fparam);

   if(AM_SUCCESS != ret ){
     printf("\n fail to get current ATV freqency,ret =%d !\n",ret);
   }

   /*Prepare sqlite3 stmts*/
   memset(sql, 0, sizeof(sql));

   snprintf(sql, sizeof(sql), "select db_id,src from ts_table where freq=%d",  fparam.frequency);
   printf("\n[%s:%d] --->perform sql :%s\n",__FUNCTION__,__LINE__,sql);
   if(AM_DB_Select(sqldb, sql, &row, "%d,%d", &ts_dbid,&src_id) !=AM_SUCCESS || row == 0){
      printf("\n==== cannot get current ts(%d)!", ts_dbid);
      return AM_FAILURE;
   }

   printf("\n====current ATV ts_db_id :%d,src_id %d,freg:%ld ,ntsc_current_channumner :0x20000 \n",ts_dbid,src_id,fparam.frequency/*,ntsc_current_channumner*/);
   memset(sql, 0, sizeof(sql));
#if 1
   snprintf(sql, sizeof(sql), "select db_id,name,major_chan_num,minor_chan_num,chan_num,source_id from srv_table where db_ts_id=%d \
                                                                and src=%d limit 1", ts_dbid,src_id );
   printf("\n[%s:%d] --->perform sql :%s\n",__FUNCTION__,__LINE__,sql);
   if (AM_DB_Select(sqldb, sql, &row, "%d,%s:256,%d,%d,%d,%d", &srv_dbid,namestring, &major_chan_num,
                    &minor_chan_num,&default_chan_num,&tmp_source_id) != AM_SUCCESS || row == 0)
   {
       /*No such service*/
       printf("\n[%s:%d]==== cannot get current srv_table\n",__FUNCTION__,__LINE__);
       return AM_FAILURE;
   }
#else
    snprintf(sql, sizeof(sql), "select db_id,name,major_chan_num,minor_chan_num,chan_num from srv_table where db_ts_id=%d \
                                                                and src=%d limit 1", ts_dbid,src_id );
    printf("\n[%s:%d] --->perform sql :%s\n",__FUNCTION__,__LINE__,sql);
                     if (AM_DB_Select(sqldb, sql, &row, "%d,%s:256,%d,%d,%d", &srv_dbid,namestring, &major_chan_num,
                         &minor_chan_num,&default_chan_num) != AM_SUCCESS || row == 0)
                     {
                        /*No such service*/
                        printf("\n==== cannot get current srv_table\n");
                     }

#endif
   printf("\n==== current srv db_id : (%d)!\n", srv_dbid);
   printf("\n==== current channel name: (%s)!\n", namestring);
   printf("\n==== current  major_chan_num:%d,minor_chan_num:%d,chan_num :0x%x ,source_id:%d\n",major_chan_num, minor_chan_num,default_chan_num,tmp_source_id);

   *db_ts_id = ts_dbid;
   *srv_db_id = srv_dbid;
   *source_id = tmp_source_id;
   *src = src_id;

   return ret;
}


static void am_xds_store_process(void)
{
   struct dvb_frontend_parameters fparam;
   AM_ErrorCode_t ret = AM_SUCCESS;
   int row = 1;
   int db_ts_id = -1,srv_db_id =-1;
   int src_id = -1;
   sqlite3_stmt *stmt,*update_startend_stmt;
   char sql[256];
   int go = 1;
   int source_id=-1,src=-1;

   const char *db_path ="/data/data/com.amlogic.tvservice/databases/dvb.db";

   const char *update_evt_sql = "update evt_table set name=? where db_id=?";  

   const char *insert_evt_sql = "insert into evt_table(src,db_net_id, db_ts_id, \
                db_srv_id, event_id, name, start, end, descr, items, ext_descr,nibble_level,\
                sub_flag,sub_status,parental_rating,source_id,rrt_ratings) \
                values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";

   const char *insert_evt_sql_name = "insert epg events";
   const char *update_startend_sql = "update evt_table set start=?, end=? where db_id=?";
   const char *update_startend_sql_name = "update startend";

   int major_chan_num = -1,minor_chan_num =-1;
   int default_chan_num = -1;
   char namestring[256];
   sqlite3 *hdb;
   char buf[256];

   // prepare database initialization
    AM_DB_Setup(db_path, hdb);

   ret = AM_FEND_GetLastPara(&fparam);

   if(AM_SUCCESS != ret ){
     printf("\n fail to get current ATV freqency,ret =%d !\n",ret);
   }

   printf( "\n curren ATV lock frequency is  %u,ret =%d \n", fparam.frequency,ret);

   if(fparam.frequency < 57000000){
     printf("\n invlid frequency settting for current ATV channel,please double check!\n");
   }
  
   printf("\nprepare database\n");
   AM_DB_HANDLE_PREPARE(hdb);

   //
   ret = xds_get_svr_si_table_items(hdb,&db_ts_id,&srv_db_id,&source_id,&src);
     if(AM_SUCCESS != ret)
     {
        printf("\n[%s:%d] fail to get svr_si table items\n",__FUNCTION__,__LINE__);
     }
   //
   do{
           printf("Enter your sql cmd:\n");
           if (gets(buf))
           {
                  if (!strncmp(buf, "select", 6))
                  {
                     /*Prepare sqlite3 stmts*/
		     memset(sql, 0, sizeof(sql));

                     snprintf(sql, sizeof(sql), "select db_id,src from ts_table where freq=%ld",  fparam.frequency);
                     printf("\n[%s:%d] --->perform sql :%s\n",__FUNCTION__,__LINE__,sql);
                     if(AM_DB_Select(hdb, sql, &row, "%d,%d", &db_ts_id,&src_id) !=AM_SUCCESS || row == 0){
                       printf("\n==== cannot get current ts(%d)!", db_ts_id);
                      }

                     printf("\n====current ATV ts_db_id :%d,src_id %d,freg:%ld\n",db_ts_id,src_id,fparam.frequency);

                     snprintf(sql, sizeof(sql), "select db_id,name,major_chan_num,minor_chan_num,chan_num from srv_table where db_ts_id=%d \
                                                                and src=%d limit 1", db_ts_id,src_id );
                     printf("\n[%s:%d] --->perform sql :%s\n",__FUNCTION__,__LINE__,sql);
                     if (AM_DB_Select(hdb, sql, &row, "%d,%s:256,%d,%d,%d", &srv_db_id,namestring, &major_chan_num, 
                         &minor_chan_num,&default_chan_num) != AM_SUCCESS || row == 0)
                     {
                        /*No such service*/
                        printf("\n==== cannot get current srv_table\n");
                     }

                     printf("\n==== current srv db_id : (%d)!\n", srv_db_id);
                     printf("\n==== current channel name: (%s)!\n", namestring);
                     printf("\n==== current  major_chan_num:%d,minor_chan_num:%d,chan_num :0x%x \n",major_chan_num, minor_chan_num,default_chan_num);
                 }
                 else if (!strncmp(buf, "insert",6))
                 {   
                     AM_Bool_t need_update;
                     int starttime, endtime, evt_dbid,event_id,source_id;
                     char event_name[256],event_descr[256],event_ext_descr[256],rrt_rating[256];

                     snprintf(sql, sizeof(sql), "select db_id,event_id,name,start,end ,descr,ext_descr,source_id,rrt_ratings from evt_table where db_ts_id=%d \
									and db_srv_id=%d", db_ts_id, srv_db_id);
                    printf("\n[%s:%d] --->perform sql :%s\n",__FUNCTION__,__LINE__,sql);
		    if (AM_DB_Select(hdb, sql, &row, "%d,%d,%s:256,%d,%d,%s:256,%s:256,%d,%s:256",&evt_dbid,&event_id,event_name, &starttime, &endtime,event_descr,
                                                                event_ext_descr,&source_id,rrt_rating) == AM_SUCCESS && row > 0)
		    {
			need_update = AM_TRUE;
                        printf("\n==== current evt_dbid : (%d),event_id:%d,starttime:%ld,endtime:%ld\n", evt_dbid,event_id,starttime,endtime);
                        printf("\n==== current evt_name: (%s)!\n",event_name);
                        printf("\n==== current event_descr: (%s)!\n",event_descr);
                        printf("\n==== current event_ext_descr: (%s)!\n",event_ext_descr);
                        
                        printf("\n==== current  source_id:%d\n",source_id);
                        printf("\n==== current rrt_rating: (%s)!\n",rrt_rating);
                        
                        printf("\n==== record already exist in evt_table and update it\n");

                        //update record
                        if (AM_DB_GetSTMT(&update_startend_stmt, update_startend_sql_name, update_startend_sql, 0) != AM_SUCCESS)
                         {
                           printf("\n=====prepare update events stmt failed");
                           break;
                         }

                        if (need_update)
			{     
                                starttime = endtime;
                                endtime +=10000 ;
				sqlite3_bind_int(update_startend_stmt, 1, starttime);
				sqlite3_bind_int(update_startend_stmt, 2, endtime);
				sqlite3_bind_int(update_startend_stmt, 3, evt_dbid);
				STEP_STMT(update_startend_stmt, update_startend_sql_name, update_startend_sql);
			}
			continue;
                    }
                    else
                   { 
                         printf("\n==== insert new record into evt_table\n");
                         if (AM_DB_GetSTMT(&stmt, insert_evt_sql_name, insert_evt_sql, 0) != AM_SUCCESS)
 			 { 		 
			   printf("\n===== prepare insert events stmt failed");
		           break;
			 }
                         
                         //add new record to evt_table
                         sqlite3_bind_int(stmt, 1, src_id);//for test
			 sqlite3_bind_int(stmt, 2, -1);
		         sqlite3_bind_int(stmt, 3, db_ts_id);
		         sqlite3_bind_int(stmt, 4, srv_db_id);
	  	         sqlite3_bind_int(stmt, 5, 119);
                         sqlite3_bind_text(stmt, 6,"Event-test-0", -1, SQLITE_STATIC);
			 sqlite3_bind_int(stmt, 7, 10000);//starttime
		         sqlite3_bind_int(stmt, 8, 10000+999);//endtime
                         sqlite3_bind_text(stmt, 9, "event-descr-test-0", -1, SQLITE_STATIC);//event descr
		         sqlite3_bind_text(stmt, 10, "items-1", -1, SQLITE_STATIC);//items descr
		         sqlite3_bind_text(stmt, 11, "exd-descr-test-2", -1, SQLITE_STATIC);//event extend descr
		         sqlite3_bind_int(stmt, 12, 0);
		         sqlite3_bind_int(stmt, 13, 0);
		         sqlite3_bind_int(stmt, 14, 0);
		         sqlite3_bind_int(stmt, 15, 0);
		         sqlite3_bind_int(stmt, 16, 99);//for test
		         sqlite3_bind_text(stmt, 17, "5 7 3", -1, SQLITE_STATIC);
		         STEP_STMT(stmt, insert_evt_sql_name, insert_evt_sql);
                   }
                 }
                  else if (!strncmp(buf, "quit", 4))
                 {
                     go=0;
                     break;
                 }

             }
     }while (go);

      AM_DB_Quit(hdb);

    return ; 
}
