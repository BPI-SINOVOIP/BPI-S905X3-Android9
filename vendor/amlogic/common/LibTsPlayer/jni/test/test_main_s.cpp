#define LOG_TAG "ctc_player"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <CTC_MediaControl.h>
#define kReadSize (64*1024)
#define LOG_LINE() ALOGD("[%s:%d]", __FUNCTION__, __LINE__);
#define kNum 1
unsigned char* mBuf[kNum];// = (unsigned char*) malloc(sizeof(unsigned char) * kReadSize);
VIDEO_PARA_T vidPara = {0, 640, 480, 24, VFORMAT_H264, 0};
char filename[256];
int active_bitmask = 0x0;
pthread_t tid[kNum];

int x = 100, y = 100, w = 640, h = 360;
int last_active_index = 0;
ITsPlayer* player[kNum];
int mSourceFD[kNum];
int my_idx = 0;
int idx[kNum];

int amsysfs_set_sysfs_int(const char *path, int val)
{
    int fd;
    int bytes;
    char  bcmd[16];
    fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd >= 0) {
        sprintf(bcmd, "%d", val);
        bytes = write(fd, bcmd, strlen(bcmd));
        close(fd);
        return 0;
    } else {
        ALOGE("unable to open file %s,err: %s", path, strerror(errno));
    }
    return -1;
}

void* doWrite(void *arg) {
    int i = *(static_cast<int*>(arg));
    while (true) {
        int ret = read(mSourceFD[i], mBuf[i], kReadSize);
        if (ret == 0) {
            ALOGD("player %d rewind", i);
            lseek(mSourceFD[i], 0, SEEK_SET);
            continue;
        }
        int sizeWritten = 0;
        while ((sizeWritten=player[i]->WriteData(mBuf[i], ret)) == -1) {
            usleep(5*1000);
        }
		usleep(5 * 1000);
        //PAVBUF_STATUS p = (PAVBUF_STATUS)malloc(sizeof(AVBUF_STATUS));
        //player[i]->GetAvbufStatus(p);
       // ALOGD("abuf_size:%d,aes_size:%d, vbuf_size:%d,ves_size:%d",
       //p->abuf_size,p->abuf_data_len, p->vbuf_size,p->vbuf_data_len);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    ALOGD("start ctc_test");
	amsysfs_set_sysfs_int("/sys/module/amvideo/parameters/ctsplayer_exist", 0);

//    if (argc >= 6) {
//        x = atoi(argv[2]);
//        y = atoi(argv[3]);
//        w = atoi(argv[4]);
//        h = atoi(argv[5]);
//    }
//    if (argc >= 7)
//        my_idx = atoi(argv[6]);
	int type_mp4 = atoi(argv[6]);

    for (int i=0;i<kNum;i++) {
        LOG_LINE();
        usleep(atoi(argv[2])  * 1000);
        player[i] = GetMediaControl(2);
        LOG_LINE();
       // player[i]->InitVideo(&vidPara);
        if (1 == type_mp4)
            player[i]->setDataSource(argv[1]);
        player[i]->SetVideoWindow(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
        player[i]->StartPlay();
        LOG_LINE();
		if (0 == type_mp4) {
			sprintf(filename, "%s", argv[1]);

			mSourceFD[i] = open(filename, O_RDONLY);
			ALOGD("open file %s result: %s",filename, strerror(errno));

			mBuf[i] = (unsigned char*) malloc(sizeof(unsigned char) * kReadSize);
			if (mBuf[i] == NULL) {
				ALOGD("mBuf[%d] == NULL", i);
				return 0;
			}

			idx[i] = i;
			pthread_create(&(tid[i]), NULL, doWrite, (void *)(&idx[i]));
		}
        usleep(100 * 1000);
    }

//    while (1) {
//        char levels_value[92];
//        int active_idx = 0;
//        if(property_get("media.ctc.active-idx",levels_value,NULL)>0)
//            sscanf(levels_value, "%d", &active_idx);
//
//        if (active_idx == my_idx)
//            player->resumeAudio();
//        else
//            player->pauseAudio();
//
//        int ret = read(mSourceFD, mBuf, kReadSize);
//        if (ret == 0) {
//            ALOGD("EOF");
//            break;
//        }
//        player->WriteData(mBuf, ret);
//    }

    //free(mBuf);
    //mBuf = NULL;
    //close(mSourceFD);
    //mSourceFD = NULL;

    ALOGD("[%s:%d]", __FUNCTION__, __LINE__);
    while (true)
		usleep(100 * 1000);

    ALOGD("[%s:%d]", __FUNCTION__, __LINE__);
    return 0;
}
