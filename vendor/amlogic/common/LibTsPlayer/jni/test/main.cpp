#include "CTsPlayer.h"
#include "../media_processor/CTsPlayerImpl.h"
#include "CTC_MediaControl.h"
#include "CTC_MediaProcessor.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <android/log.h>

using namespace android;
using android::sp;

/*
FILE *fopen(const char *path, const char *mode);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

*/
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO  , "ctsplayer", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN  , "ctsplayer", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "ctsplayer", __VA_ARGS__)

/*int osd_blank(char *path,int cmd)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);

    if (fd >= 0) {
        sprintf(bcmd,"%d",cmd);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}*/

int set_sys_node(char *path,int cmd)
{
    int fd;
    char  bcmd[16];
    fd = open(path, O_CREAT|O_RDWR | O_TRUNC, 0644);

    if(fd>=0) {
        sprintf(bcmd,"%d",cmd);
        write(fd,bcmd,strlen(bcmd));
        close(fd);
        return 0;
    }

    return -1;
}

unsigned char buffer[1024*64];
int main(int argc,char **argv)
{
      if ((argc != 4) && (argc != 6)) {
          LOGI("Input Format: \n");
          LOGI("ctsplayer file_name vfmt video_pid afmt audio_pid\n");
          return 0;
      }

       sp<CTsPlayer> player= new CTsPlayer();
       VIDEO_PARA_T VideoPara;
       AUDIO_PARA_T AudioPara;
       FILE *file;
       char *filename=NULL;

       int ret;
       int bufdatalen=0;
       int writelen;
       int w_test=0,h_test=0;

       filename=argv[1];
       LOGI("filenme: %s\n",argv[1]);

       memset(&VideoPara,0,sizeof(VideoPara));
       memset(&AudioPara,0,sizeof(AudioPara));

       VideoPara.vFmt=(vformat_t)atoi(argv[2]); // VFORMAT_MPEG12:0  VFORMAT_MPEG4:1 VFORMAT_H264:2 VFORMAT_HEVC:11
       VideoPara.pid=atoi(argv[3]);

       if (argv[4] != 0) {
           AudioPara.aFmt=(aformat_t)atoi(argv[4]);//AFORMAT_MPEG:0  AFORMAT_AAC:2 AFORMAT_AC3:3
           AudioPara.pid=atoi(argv[5]);
       }
       LOGI("[%s %d] vfmt:%d vpid:%d afmt:%d apid:%d", __FUNCTION__, __LINE__,VideoPara.vFmt,VideoPara.pid,AudioPara.aFmt,AudioPara.pid);

       player->InitVideo(&VideoPara);
       if (AudioPara.pid != 0) {
           player->InitAudio(&AudioPara);
       }

       player->GetVideoPixels(w_test,h_test);
       LOGI("GetVideoPixels, width:%d,height:%d",w_test,h_test);
       //player->SetVideoWindow(0,0,w_test,h_test);// 0,0,-1,-1

       set_sys_node("/sys/class/graphics/fb0/osd_display_debug",1);
       set_sys_node("/sys/class/graphics/fb0/blank",0);//clear all osd0 ,don't need it on APK

       if (!player->StartPlay()) {
           LOGI("Player start failed\n");
           return 0;
       }

       file=fopen(filename,"r");
       if (file == NULL) {
           LOGI("open file %s failed\n",filename);
           return 0;
       }

       while (1) {
           if (feof(file))
           {
               LOGI("playfile %s end, seek 0\n",filename);
               fseek(file,0,SEEK_SET);
            }

            if (bufdatalen <= 0) {
                ret=fread(buffer,188,174,file);
                if (ret > 0) {
                    bufdatalen = ret*188;
                    //LOGI("Read Data %d\n", bufdatalen);
                } else {
                    LOGI("read data failed %d\n",ret);
                    break;
                }
             }

             writelen = player->WriteData(buffer,bufdatalen);

             if (writelen > 0 && writelen != bufdatalen ) {
                 memcpy(buffer,buffer+writelen,bufdatalen-writelen);
                 //LOGI("WriteData %d\n", writelen);
                 usleep(10000);
             }
             if (writelen > 0)
                 bufdatalen-=writelen;
       }

       return 0;
}


