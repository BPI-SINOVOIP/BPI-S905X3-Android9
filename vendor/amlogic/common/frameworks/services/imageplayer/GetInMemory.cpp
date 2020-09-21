/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#define LOG_TAG "ImagePlayerService::GetInMemory"

#include <curl/curl.h>
#include <GetInMemory.h>


static char* HTTP_IMAGE_PATH = "/sdcard/Pictures/imageserver.jpg";

namespace android {

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}
size_t getcontentlengthfunc(void *ptr, size_t size, size_t nmemb, void *stream) {
       ALOGE("ptr:%s",(char*)ptr);
       return size * nmemb;
}

size_t receive_data(void *buff,size_t len,size_t nmemb,FILE *file);
char* getFileByCurl(char* url) {
    CURL *curl_handle;
    CURLcode res;
    char* path = HTTP_IMAGE_PATH;
    FILE *fp = fopen(path, "wb");

    /*
    struct MemoryStruct chunk;
    chunk.memory = (char*)malloc(1);
    chunk.size = 0;
    */

    curl_global_init(CURL_GLOBAL_ALL);
    /* init the curl session */
    curl_handle = curl_easy_init();
    if (!curl_handle) {
         ALOGE("getFileByCurl init failed\n");
         return nullptr;
    }
    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, receive_data);
    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, getcontentlengthfunc);
    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
    /* get it! */
    res = curl_easy_perform(curl_handle);

    if (fp) {
        fclose(fp);
    }
    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    /* check for errors */
    if (res != CURLE_OK) {
        ALOGE("getFileByCurl curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return nullptr;
    }
    ALOGD("getFileByCurl http data is got.");
    return path;

}
size_t receive_data(void *buff,size_t len,size_t nmemb,FILE *file) {
    size_t r_size = fwrite(buff,len,nmemb,file);
    //ALOGE("receive_data %d[%d %d]",r_size,len,nmemb);
    return r_size;
}
}
