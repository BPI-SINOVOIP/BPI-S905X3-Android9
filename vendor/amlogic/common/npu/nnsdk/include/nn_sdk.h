/****************************************************************************
*
*    Copyright (c) 2019  by amlogic Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of amlogic Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of amlogic Corporation.
*
*****************************************************************************/

#ifndef _AMLNNSDK_H
#define _AMLNNSDK_H

#ifdef __cplusplus
extern "C" {
#endif
/////////////////////////some value define////////////////////////
#define MAX_NAME_LEGTH              64
#define INPUT_MAX_NUM               16
#define OUTPUT_MAX_NUM              32
#define INPUT_CNANNEL               3
#define MAX_DETECT_NUM              230
#define SUPPORT_NET_NUM             60
#define ADDRESS_MAX_NUM             16

///////////////////////////////////////////////////////////////

//////////////////////nn sdk demo struct type//////////////////
typedef struct __box
{
	float x;
	float y;
	float w;
	float h;
	float score;
	float objectClass;
}detBox;

typedef struct __point
{
	float x;
	float y;
}point_t;

typedef struct __nn_image_out
{
	int height;
	int width;
	int channel;
	unsigned char *data;  //this buffer is returned by aml_module_output_get
}image_out_t;

/*=================================================
output of aml_module_t type:IMAGE_CLASSIFY
score: top 5 class confidence
top:   the class of top 5
=================================================*/
typedef struct __nn_image_classify
{
	float   score[5];
	unsigned int  topClass[5];
}img_classify_out_t;
/*=================================================
output of aml_module_t type:OBJECT_DETECT
detNum: the number of object detect
pBox:   the object info detect
=================================================*/
typedef struct __nn_obj_detect
{
	unsigned int  detNum;
	detBox *pBox;
}obj_detect_out_t;
/*=================================================
output of aml_module_t type:FACE_DETECTION
detNum: the number of face detect
pBox:   the face info detect
=================================================*/
typedef struct __nn_face_detect
{
	unsigned int  detNum;
	detBox pBox[MAX_DETECT_NUM];
}face_detect_out_t;
/*=================================================
output of aml_module_t type:FACE_LANDMARK_5
pos value:
0:left-eye,
1:right-eye,
2:nose,
3:left-mouth,
4:right-mouth
=================================================*/
typedef struct __nn_face_landmark_5
{
	unsigned int   detNum;
	detBox facebox[MAX_DETECT_NUM];
	point_t pos[MAX_DETECT_NUM][5];
}face_landmark5_out_t;

/*=================================================
output of aml_module_t type:FACE_LANDMARK_68
68 pos index:
0-16是下颌线，
17-21是右眼眉，
22-26是左眼眉，
27-35是鼻子，
36-41是右眼，
42-47是左眼，
48-60是嘴外轮廓，
61-67是嘴内轮廓。
=================================================*/
typedef struct __nn_face_landmark_68
{
	unsigned int detNum;
	detBox facebox[MAX_DETECT_NUM];
	point_t pos[MAX_DETECT_NUM][68];
}face_landmark68_out_t;
/*=================================================
output of aml_module_t type:FACE_RECOGNIZE
faceVector: the vector generate by input face,
can recognized by this vector
=================================================*/
typedef struct __nn_face_recognize
{
	float faceVector[512];
}face_recognize_out_t;
/*=================================================
output of aml_module_t type:FACE_COMPARISION
compare the input two image whether the same person or not:
compareScore: the score of compare two input face,
if compareScore >= 1.18, same person for input two face
if compareScore < 1.18,  not same person
=================================================*/
typedef struct __nn_face_compare
{
	float compareScore;
}face_compare_out_t;
/*=================================================
output of aml_module_t type:FACE_AGE
age value:
=================================================*/
typedef struct __nn_face_age
{
	int age;
}face_age_out_t;
/*=================================================
output of aml_module_t type:FACE_GENDER
female rate:
gender > 0.5: female
gender < 0.5: male
=================================================*/
typedef struct __nn_face_gender
{
	float gender;
}face_gender_out_t;
/*=================================================
output of aml_module_t type:FACE_EMOTION
0: 'angry',
1: 'disgust',
2: 'fear',
3: 'happy',
4: 'sad',
5: 'surprise',
6: 'neutral'
=================================================*/
typedef struct __nn_face_emotion
{
	int emotion;
    float prob;
}face_emotion_out_t;
/*=================================================
output of aml_module_t type:BODY_POSE
now just support single person pose
0:'Nose',
1:'Neck',
2:'R-Sho',
3:'R-Elb',
4:'R-Wr',
5:'L-Sho',
6:'L-Elb',
7:'L-Wr',
8:'R-Hip',
9:'R-Knee',
10:'R-Ank',
11:'L-Hip',
12:'L-Knee',
13:'L-Ank',
14:'R-Eye',
15:'L-Eye',
16:'R-Ear',
17:'L-Ear'
=================================================*/
typedef struct __nn_bodypos
{
	int valid;    //whether this point is valid
	point_t pos;
}bodypos_t;
typedef struct __nn_body_pose
{
	bodypos_t bpos[18];
}body_pose_out_t;


/*=================================================
output of aml_module_t type:FINGER_POSE
=================================================*/
typedef struct __nn_finger_pose
{
	image_out_t fingerOut;
}finger_pose_out_t;
/*=================================================
output of aml_module_t type:IMAGE_SR
=================================================*/
typedef struct __nn_sr_out
{
	image_out_t srOut;
}sr_out_t;
/*=================================================
output of aml_module_t type:IMAGE_SEGMENTATION
=================================================*/
typedef struct __nn_segment_out
{
	image_out_t segOut;
}segment_out_t;

/*=================================================
output of aml_module_t type:HEAD_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_head_detect
{
	obj_detect_out_t headOut;
}head_det_out_t;
/*=================================================
output of aml_module_t type:CARPLATE_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_car_detect
{
	obj_detect_out_t carOut;
}car_det_out_t;
/*=================================================
output of aml_module_t type:TEXT_DETECTION
the return value are detect region for these type.
=================================================*/
typedef struct __nn_text_detect
{
	obj_detect_out_t textOut;
}text_det_out_t;
/*=================================================
output of aml_module_t type:CARPLATE_RECOG
the output for car license confidence
confidence: the confidence of character for car license
val: the index number for license value;
10(0-9)+26(A-Z)+32(chinese character)
=================================================*/
typedef struct __nn_car_recognize
{
	float confidence;
	unsigned char val[32];
}car_license_out_t;
//////////////////////////////////////////////////////

typedef struct __nn_person_detect
{
    unsigned int  detNum;
    detBox pBox[MAX_DETECT_NUM];
}person_detect_out_t;

typedef struct __nn_yoloface_v2
{
    unsigned int  detNum;
    detBox *pBox;
}yoloface_v2_out_t;

typedef struct __nn_yolov2
{
    unsigned int  detNum;
    detBox *pBox;
}yolov2_out_t;

typedef struct __nn_yolov3
{
    unsigned int  detNum;
    detBox *pBox;
}yolov3_out_t;

typedef struct __nn_facenet
{
    unsigned char faceVector[128];
}facenet_out_t;

typedef struct __nn_face_recognize_uint
{
    unsigned char faceVector[512];
}face_recognize_uint_out_t;
/////////////////////common type//////////////////////
/*=====================================================
the common type for sdk api
======================================================*/
typedef enum _amlnn_model_ {
    CAFFE    = 0,
	TENSORFLOW,
    TENSORFLOWLITE,
    DARKNET,
    ONNX,
	KERAS,
	PYTORCH,
    MEDEL_MAX
} amlnn_model_type;
/*=================================================================================
load nbg type
==================================================================================*/
typedef enum _amlnn_nbg_type_ {
    NN_NBG_FILE    = 0,
    NN_NBG_MEMORY
} amlnn_nbg_type;

typedef enum _amlnn_input_ {
	RGB24_RAW_DATA = 0,
	TENSOR_RAW_DATA,
	QTENSOR_RAW_DATA,
	BINARY_RAW_DATA,
	INPUT_DMA_DATA,
	RAW_DATA_MAX
} amlnn_input_type;

 /*  QUERY_PERF_RUN,  query the time of run,should query after amlnn_outputs_get. */
typedef enum _amlnn_query_cmd {
    QUERY_INPUT_NUM = 0,                                /* query the number of input  tensor. */
	QUERY_OUTPUT_NUM,                                   /* query the number of output  tensor. */
    QUERY_INPUT_ATTR,                                   /* query the attribute of input tensor. */
    QUERY_OUTPUT_ATTR,                                  /* query the attribute of output tensor. */
    QUERY_SDK_VERSION,                                  /* query the sdk & driver version */
    QUERY_CMD_MAX
} amlnn_query_cmd;

typedef enum _nn_buffer_format_e
{
    /*! \brief A float type of buffer data */
    NN_BUFFER_FORMAT_FP32       = 0,
    /*! \brief A half float type of buffer data */
    NN_BUFFER_FORMAT_FP16       = 1,
    /*! \brief A 8 bit unsigned integer type of buffer data */
    NN_BUFFER_FORMAT_UINT8      = 2,
    /*! \brief A 8 bit signed integer type of buffer data */
    NN_BUFFER_FORMAT_INT8       = 3,
    /*! \brief A 16 bit unsigned integer type of buffer data */
    NN_BUFFER_FORMAT_UINT16     = 4,
    /*! \brief A 16 signed integer type of buffer data */
    NN_BUFFER_FORMAT_INT16      = 5
}   nn_buffer_format_e;

typedef enum _nn_buffer_quantize_format_e
{
    /*! \brief Not quantized format */
    NN_BUFFER_QUANTIZE_NONE                    = 0,
    /*! \brief The data is quantized with dynamic fixed point */
    NN_BUFFER_QUANTIZE_DYNAMIC_FIXED_POINT     = 1,
    /*! \brief The data is quantized with TF asymmetric format */
    NN_BUFFER_QUANTIZE_TF_ASYMM                = 2
}   nn_buffer_quantize_format_e;

typedef enum {
    IMAGE_CLASSIFY      = 0,
    OBJECT_DETECT       = 1,    /// object_detection
    FACE_DETECTION      = 2,    ///
    FACE_LANDMARK_5     = 3,    /// landmark 5 points
    FACE_LANDMARK_68    = 4,    ///< Face Landmark (68 KeyPoints)
    FACE_RECOGNIZE      = 5,    ///< Face Recognition
    FACE_COMPARISION    = 6,    ///< Face compare,whether same person
    FACE_AGE            = 7,    ///< Face age
    FACE_GENDER         = 8,    ///< Face gender
    FACE_EMOTION        = 9,    ///< Face emotion
    BODY_POSE           = 10,   ///< Body Pose
    FINGER_POSE         = 11,   ///< Finger Landmark(21 KeyPoint)
    HEAD_DETECTION      = 12,   ///< Head Detection
    CARPLATE_DETECTION  = 13,   ///< Car Plate Detection
    CARPLATE_RECOG      = 14,   ///< Car Plate Recognition
    TEXT_DETECTION      = 15,   ///< text region detect
    IMAGE_SR            = 16,   ///< image SR
    IMAGE_SEGMENTATION  = 17,   ///< image segment, based on icnet
    PERSON_DETECT       = 18,   ///< person detect
    YOLOFACE_V2         = 19,
    YOLO_V2             = 20,
    YOLO_V3             = 21,
    FACE_NET            = 22,
    FACE_RECOG_U        = 23,
    CUSTOM_NETWORK      = 99,   ///< custom network, for user development
    MODEL_MAX           = 100    ///< max model number
} aml_module_t;

typedef enum {
    AML_OUTDATA_FLOAT32      = 0,
    AML_OUTDATA_RAW          = 1,
	AML_OUTDATA_DMA          = 2
} aml_output_format_t;
typedef enum {
	AML_PROFILE_NONE         = 0,
    AML_PROFILE_PERFORMANCE  = 1,
    AML_PROFILE_BANDWIDTH    = 2,
	AML_PROFILE_MEMORY       = 3
} aml_profile_type_t;

typedef enum {
    AML_PERFORMANCE_MODE         = 1,
    AML_POWER_SAVE_MODE          = 2,
	AML_MINIMUM_POWER_MODE       = 3
} aml_policy_type_t;

typedef  struct __amlnn_module_out_data_t
{
	int typeSize;
    aml_module_t mdType;
    aml_output_format_t format;
} aml_output_config_t;

typedef struct _nn_buffer_create_params_t
{
    unsigned int     num_of_dims; /*!< \brief The number of dimensions specified in *sizes*/
    unsigned int      sizes[4];       /*!< \brief The pointer to an array of dimension */
    nn_buffer_format_e          data_format; /*!< \brief Data format for the tensor, see <tt>\ref nn_buffer_format_e </tt> */
    nn_buffer_quantize_format_e quant_format; /*!< \brief Quantized format see <tt>\ref nn_buffer_quantize_format_e </tt>. */
    union {
        struct {
            unsigned char fixed_point_pos; /*!< \brief Specifies the fixed point position when the input element type is int16, if 0 calculations are performed in integer math */
        } dfp;

        struct {
            float        scale;       /*!< \brief Scale value for the quantized value */
            unsigned int zeroPoint;  /*!< \brief  A 32 bit integer, in range [0, 255] */
        } affine;
     }
     quant_data; /*<! \brief The union of quantization information */
} nn_buffer_params_t;

typedef struct out_buf
{
	unsigned int size;
	unsigned char *buf;
	nn_buffer_params_t *param;
	char  name[MAX_NAME_LEGTH];     //output tensor name
}outBuf_t;

typedef struct __nnout
{
	int typeSize;
	unsigned int num;   /*===========output tensor number============*/
	outBuf_t out[OUTPUT_MAX_NUM];
}nn_output;


typedef struct
{
	int valid;
	float mean[INPUT_CNANNEL];
	float scale;
}input_info;

typedef struct __nn_input
{
	int typeSize;
	int  input_index;
	int  size;
	unsigned char* input;
	amlnn_input_type input_type;
	input_info info;
}nn_input;

typedef struct __assign_address
{
	unsigned char* inAddr[ADDRESS_MAX_NUM];
	unsigned char* outAddr[ADDRESS_MAX_NUM];
}assign_user_address_t;

typedef struct __aml_nn_config
{
	int typeSize;
	const char *path;
	const char *pdata;
	int length;
	amlnn_model_type modelType;
	amlnn_nbg_type nbgType;
	assign_user_address_t inOut;
}aml_config;

typedef struct {
    unsigned int dim_count;       /*dim count*/
    unsigned int sizes_of_dim[4]; /*dim value,just support 4-d dim*/
    unsigned int data_format;     /*see as nn_buffer_format_e*/
    unsigned int data_type;       /*not use*/
    unsigned int quantization_format; /*see as nn_buffer_quantize_format_e*/
    int fixed_point_pos;          /*for int8/int16 QUANTIZE_DYNAMIC_FIXED_POINT*/
    float TF_scale;               /*as tf define,scale*/
    int TF_zeropoint;             /*as tf define,zeropoint*/
    char name[MAX_NAME_LEGTH];    /*not use,will used in future*/
} info;

typedef struct {
	unsigned int valid;
    unsigned int num;
	info *info;
} tensor_info;

typedef enum {
    AML_INPUT_TENSOR      = 0,
    AML_OUTPUT_TENSOR     = 1,
} aml_flush_type_t;
//////////////////////////////////////////////////////////


/*=============================================================
                     NNSDK main api 
==============================================================*/
void* aml_module_create(aml_config* config);                   /*==========create aml network module======*/
int aml_module_input_set(void* context,nn_input *pInput);      /*==========set network input==============*/
void* aml_module_output_get(void* context,aml_output_config_t outconfig); /*=======run and get output====*/
void* aml_module_output_get_simple(void* context); /*=======run and get output simply,for custom network====*/
int aml_module_destroy(void* context);      /*=====destroy network environment,free the alloced buffer====*/

/*============================================================
             some aml sdk util functions
==============================================================*/
unsigned char * aml_util_mallocAlignedBuffer(int mem_size);  /*======malloc 4k align buffer for dma IO====*/
void aml_util_freeAlignedBuffer(unsigned char *addr);        /*======free buffer alloced by above=========*/
/*==swap input buffer,the inputId(for multi-number input)is ordered as amlnn_get_input_tensor_info array==*/
int aml_util_swapInputBuffer(void *context,void *newBuffer,unsigned int inputId);
tensor_info* aml_util_getInputTensorInfo(const char* nbgdata);  /*====get model input tensor information list=====*/
tensor_info* aml_util_getOutputTensorInfo(const char* nbgdata); /*====get model output tensor information list====*/
void aml_util_freeTensorInfo(tensor_info* tinfo);     /*====free the tensor_info memory get by above two functions*/
/*===========flush tensor cache memory=======================*/
int  aml_util_flushTensorHandle(void* context,aml_flush_type_t type);
void process_top5(float *buf,unsigned int num,img_classify_out_t* clsout);
int  aml_util_setProfile(aml_profile_type_t type,const char *savepath); /*===set profile type===*/
int  aml_util_setPowerPolicy(aml_policy_type_t type); /*===set power policy===*/
#ifdef __cplusplus
} //extern "C"
#endif
#endif // _AMLNNSDK_H
