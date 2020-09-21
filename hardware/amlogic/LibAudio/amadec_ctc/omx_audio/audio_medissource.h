
/*
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <utils/String8.h>
#include <cutils/bitops.h>
*/
#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
namespace android
{


#define     DSPshort short
#define     DSPerr   short
#define     DSPushort  unsigned short                   /*!< DSP unsigned integer */
#define     GBL_BYTESPERWRD         2                   /*!< Bytes per packed word */
#define     GBL_BITSPERWRD          (GBL_BYTESPERWRD*8) /*!< Bytes per packed word */
#define     GBL_SYNCWRD             ((DSPshort)0x0b77)  /*!< AC-3 frame sync word */
#define     GBL_MAXFSCOD            3                   /*!< Number of defined sample rates */
#define     GBL_MAXDDDATARATE       38                  /*!< Number of defined data rates */
#define     BSI_BSID_STD            8                   /*!< Standard ATSC A/52 bit-stream ID */
#define     BSI_ISDD(bsid)          ((bsid) <= BSI_BSID_STD)
#define     GBL_MAXCHANCFGS         8                   /*!< Maximum number of channel configs */
#define     BSI_BSID_AXE            16                  /*!< Annex E bitstream ID */
#define     BSI_ISDDP(bsid)         ((bsid) <= BSI_BSID_AXE && (bsid) > 10)
#define     BSI_BSID_BITOFFSET      40                  /*!< Used to skip ahead to bit-stream ID */
#define     PTR_HEAD_SIZE 20


/*! \brief Bit Stream Operations module decode-side state variable structure */
typedef struct {
    DSPshort       *p_pkbuf;           /*!< Pointer to bitstream buffer */
    DSPshort        pkbitptr;          /*!< Bit count within bitstream word */
    DSPshort        pkdata;            /*!< Current bitstream word */
#if defined(DEBUG)
    const DSPshort  *p_start_pkbuf;    /*!< Pointer to beginning of bitstream buffer */
#endif /* defined(DEBUG) */
} BSOD_BSTRM;


const DSPshort gbl_chanary[GBL_MAXCHANCFGS] = { 2, 1, 2, 3, 3, 4, 4, 5 };
/* audio coding modes */
enum { GBL_MODE11 = 0, GBL_MODE_RSVD = 0, GBL_MODE10, GBL_MODE20,
       GBL_MODE30, GBL_MODE21, GBL_MODE31, GBL_MODE22, GBL_MODE32
     };

const DSPushort gbl_msktab[] = {
    0x0000, 0x8000, 0xc000, 0xe000,
    0xf000, 0xf800, 0xfc00, 0xfe00,
    0xff00, 0xff80, 0xffc0, 0xffe0,
    0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff
};


/*! Words per frame table based on sample rate and data rate codes */
const DSPshort gbl_frmsizetab[GBL_MAXFSCOD][GBL_MAXDDDATARATE] = {
    /* 48kHz */
    {
        64, 64, 80, 80, 96, 96, 112, 112,
        128, 128, 160, 160, 192, 192, 224, 224,
        256, 256, 320, 320, 384, 384, 448, 448,
        512, 512, 640, 640, 768, 768, 896, 896,
        1024, 1024, 1152, 1152, 1280, 1280
    },
    /* 44.1kHz */
    {
        69, 70, 87, 88, 104, 105, 121, 122,
        139, 140, 174, 175, 208, 209, 243, 244,
        278, 279, 348, 349, 417, 418, 487, 488,
        557, 558, 696, 697, 835, 836, 975, 976,
        1114, 1115, 1253, 1254, 1393, 1394
    },
    /* 32kHz */
    {
        96, 96, 120, 120, 144, 144, 168, 168,
        192, 192, 240, 240, 288, 288, 336, 336,
        384, 384, 480, 480, 576, 576, 672, 672,
        768, 768, 960, 960, 1152, 1152, 1344, 1344,
        1536, 1536, 1728, 1728, 1920, 1920
    }
};



typedef int (*fp_read_buffer)(unsigned char *, int);
struct AudioMediaSource : public MediaSource {
    AudioMediaSource(void *read_buffer);
    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();
    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options = NULL);
    int  GetReadedBytes();
    int  SetReadedBytes(int size);
    int  MediaSourceRead_buffer(unsigned char *buffer, int size);
    fp_read_buffer fpread_buffer;

    //----------------------------------------
    DSPerr  bsod_init(DSPshort *    p_pkbuf, DSPshort pkbitptr, BSOD_BSTRM *p_bstrm);
    DSPerr  bsod_unprj(BSOD_BSTRM   *p_bstrm, DSPshort *p_data,  DSPshort numbits);
    int Get_ChNum_DD(void *buf);
    int Get_ChNum_DDP(void *buf);
    DSPerr bsod_skip(BSOD_BSTRM     *p_bstrm, DSPshort  numbits);
    DSPerr bsid_getbsid(BSOD_BSTRM *p_inbstrm,  DSPshort *p_bsid);
    int Get_ChNum_AC3_Frame(void *buf);
    //---------------------------------------

    int sample_rate;
    int ChNum;
    int frame_size;
    int64_t bytes_readed_sum_pre;
    int64_t bytes_readed_sum;
    int  *pStop_ReadBuf_Flag;
protected:
    virtual ~AudioMediaSource();

private:
    bool mStarted;
    sp<DataSource> mDataSource;
    sp<MetaData>   mMeta;
    MediaBufferGroup *mGroup;
    int64_t mCurrentTimeUs;
    int     mBytesReaded;

    AudioMediaSource(const AudioMediaSource &);
    AudioMediaSource &operator=(const AudioMediaSource &);
};


}
