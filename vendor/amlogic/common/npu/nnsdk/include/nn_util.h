/****************************************************************************
*   amlogic nn api util header file
*
*   Neural Network appliction network definition some util header file
*
*   Date: 2019.8
****************************************************************************/
#ifndef _AMLOGIC_NN_UTIL_H
#define _AMLOGIC_NN_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    float x, y;
} landmark;
typedef struct{
    float x, y, w, h, prob_obj;
} box;
typedef struct{
    int index;
    int classId;
    float **probs;
} sortable_bbox;

///////////////////////////////////////some util api///////////////////////////////////////////////
unsigned char *get_jpeg_rawData(const char *name,unsigned int width,unsigned int height);
void softmax(float *input, int n, float temp, float *output);
void flatten(float *x, int size, int layers, int batch, int forward);
void do_nms_sort(box *boxes, float **probs, int total, int classes, float thresh);
int nms_comparator(const void *pa, const void *pb);
float box_iou(box a, box b);
float box_union(box a, box b);
float box_intersection(box a, box b);
float overlap(float x1, float w1, float x2, float w2);
float logistic_activate(float x);
float sigmod(float x);
unsigned char *transpose(const unsigned char * src,int width,int height);
int init_fb(void);
void *camera_thread_func(void *arg);
int sysfs_control_read(const char* name,char *out);
int sysfs_control_write(const char* pname,char *value);
#ifdef __cplusplus
} //extern "C"
#endif
#endif