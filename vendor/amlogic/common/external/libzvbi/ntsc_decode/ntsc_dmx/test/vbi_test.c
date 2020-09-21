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
#include <vbi_dmx.h>
#include <decode.h>
#define VBI_IOC_MAGIC 'X'


#define VBI_IOC_CC_EN       _IO (VBI_IOC_MAGIC, 0x01)
#define VBI_IOC_CC_DISABLE  _IO (VBI_IOC_MAGIC, 0x02)
#define VBI_IOC_SET_FILTER  _IOW(VBI_IOC_MAGIC, 0x03, int)
#define VBI_IOC_S_BUF_SIZE  _IOW(VBI_IOC_MAGIC, 0x04, int)
#define VBI_IOC_START       _IO (VBI_IOC_MAGIC, 0x05)
#define VBI_IOC_STOP        _IO (VBI_IOC_MAGIC, 0x06)
#define DECODER_VBI_VBI_SIZE                0x1000




static unsigned char fd1_closed = 0 , fd2_closed = 0;
static unsigned int buf_size1 = 1024;
static unsigned int buf_size2 = 1024;
static  unsigned int  cc_type1 = 1;
static  unsigned int  cc_type2 = 4;
static unsigned int exit_cnt1 = 20;
static unsigned int exit_cnt2 = 30;
static int afe_fd1 = -1,afe_fd2 = -1;
FILE *wr_fp;
char vbuf_addr1[2000];
char vbuf_addr2[2000];
#define DUAL_FD1

int main(int argc, char *argv[])
{
	demux_vbi_start();
	return 0;
    int    ret    = 0;
    unsigned int i = 0;
    pthread_t pid = 0;

 
    char name_buf[3] = {0};

    fd_set rds1,rds2;
    struct timeval tv1,tv2;
    
#if defined(DUAL_FD1) 
    afe_fd1 = open("/dev/vbi", O_RDWR|O_NONBLOCK);
    if (afe_fd1 < 0) {
        printf("open device 1 file Error!!! \n");
        return -1;
    }
    printf("open device 1 file  \n");
#endif
#if defined(DUAL_FD2)   
    afe_fd2 = open("/dev/vbi", O_RDWR|O_NONBLOCK);
    if (afe_fd2 < 0) {
        printf("open device 2 file Error!!! \n");
        return -1;
    }
    printf("open device 2 file  \n");
#endif
//while (1) {
    //ioctl(afe_fd, VBI_IOC_CC_EN);
    //sleep(2);
    //printf("input cc type value:\n");
    //scanf("%d", &cc_type);
    //printf("input cc type1 is:%d \n", cc_type1);
#if defined(DUAL_FD1)      
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
#endif
#if defined(DUAL_FD2)           
    ret = ioctl(afe_fd2, VBI_IOC_S_BUF_SIZE, &buf_size2);
    if (ret < 0) {
        printf("set afe_fd2 ioctl VBI_IOC_S_BUF_SIZE Error!!! \n");
        return 0;
    }
    //    printf("set afe_fd2 ioctl VBI_IOC_S_BUF_SIZE   \n");
        
    ret = ioctl(afe_fd2, VBI_IOC_SET_FILTER, &cc_type2);
    if (ret < 0) {
        printf("set afe_fd2 ioctl VBI_IOC_SET_FILTER Error!!! \n");
        return 0;
    }

    //printf("set afe_fd2 ioctl VBI_IOC_SET_FILTER  \n");
    ret = ioctl(afe_fd2, VBI_IOC_START);
    if (ret < 0) {
        printf("set afe_fd2 ioctl VBI_IOC_START Error!!! \n");
        return 0;
    }    
    //printf("set afe_fd2 ioctl VBI_IOC_START  \n");
#endif    

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
		#if defined(DUAL_FD2)   
			if (exit_cnt2 != 0)
				goto fd2;
			else 
		#endif
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

}

static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data){
	printf("dump_bytes  len = %d\n",len);
	decode_vbi_test( dev_no,  fid, data,  len,  user_data);
}


void demux_vbi_start(){
	int fid;
	AM_VBI_DMX_OpenPara_t para;
	memset(&para, 0, sizeof(para));
	printf("*******************************\n\n");
	if(	AM_NTSC_DMX_Open(VBI_DEV_NO, &para) != AM_SUCCESS) {
		printf("###############AM_DMX_Open fail!\n\n");
	}
	
	AM_NTSC_DMX_AllocateFilter(VBI_DEV_NO, &fid);
	AM_NTSC_DMX_SetCallback(VBI_DEV_NO, fid, dump_bytes, NULL);
	
	AM_NTSC_DMX_SetBufferSize(VBI_DEV_NO, fid, 1024);
	AM_NTSC_DMX_StartFilter(VBI_DEV_NO, fid);

	sleep(100);
	
}

