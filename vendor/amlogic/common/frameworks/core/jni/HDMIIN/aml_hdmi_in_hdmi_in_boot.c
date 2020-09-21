/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC aml_hdmi_in_hdmi_in_boot
 */

//#include <jni.h>

#include "jni.h"
#include "JNIHelp.h"

#include <string.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/properties.h>

#include "aml_hdmi_in_hdmi_in_boot.h"


#ifdef __cplusplus
extern "C"
{
#endif

// jstring to char*
static char* jstring2string(JNIEnv* env, jstring jstr)
{
	char* rtn = NULL;

	jclass clsstring = (*env)->FindClass(env, "java/lang/String");

	jstring strencode = (*env)->NewStringUTF(env, "utf-8");
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr= (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid, strencode);
	jsize alen = (*env)->GetArrayLength(env, barr);
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);

	if(alen > 0)
	{
		rtn = (char*)malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}

	(*env)->ReleaseByteArrayElements(env, barr, ba, 0);

	return rtn;
}

//// char* to jstring
//jstring string2jstring(JNIEnv* env, char* pat)
//{
//       jclass strClass = (*env)->FindClass(env, "java/lang/String");
//       jmethodID ctorID = (*env)->GetMethodID(env, strClass, "<init>", "([BLjava/lang/String;)V");
//       jbyteArray bytes = (*env)->NewByteArray(env, strlen(pat));
//
//       (*env)->SetByteArrayRegion(env, bytes, 0, strlen(pat), (jbyte*)pat);
//       jstring encoding = (*env)->NewStringUTF(env, "utf-8");
//
//       return (jstring)(*env)->NewObject(env, strClass, ctorID, bytes, encoding);
//}

static jstring string2jstring(JNIEnv* env, const char* pat)
{
       jclass strClass = (*env)->FindClass(env, "java/lang/String");
       jmethodID ctorID = (*env)->GetMethodID(env, strClass, "<init>", "([BLjava/lang/String;)V");
       jbyteArray bytes = (*env)->NewByteArray(env, strlen(pat));
       (*env)->SetByteArrayRegion(env, bytes, 0, strlen(pat), (jbyte*)pat);
       jstring encoding = (*env)->NewStringUTF(env, "utf-8");
       return (jstring)(*env)->NewObject(env, strClass, ctorID, bytes, encoding);
}

JNIEXPORT jstring JNICALL Java_aml_hdmi_1in_hdmi_1in_1boot_readFileJNI
  (JNIEnv *env, jobject thiz, jstring file)
{
	char* fileName = jstring2string(env, file);

	int fd = 0;
	int count = 0;
	char* content = NULL;
	int index = 0;

	jstring jstr;

	fd = open(fileName, O_RDONLY);

	if(fd == -1)
	{
		return NULL;
	}

	count = lseek(fd, 0, SEEK_END);

	lseek(fd, 0, SEEK_SET);

	content = (char*)malloc(count + 1);
	if(content == NULL)
	{
		close(fd);

		return NULL;
	}

	memset((void*)content, 0, count + 1);

	// ALOGI("before read, count is: %d\n", count);
	count = read(fd, (void*)content, count);

	for(index = 0; index < count; index++)
	{
		// ALOGI("0x%2X    ", (char)content[index]);
	}
	for(index = count -1; index >= 0; index--)
	{
		if((char)content[index] != '*')
		{
			if(((char)content[index] < '0')  || ((char)content[index] > '9'))
			{
				if(((char)content[index] < 'A')  || ((char)content[index] > 'Z'))
				{
					if(((char)content[index] < 'a')  || ((char)content[index] > 'z'))
					{
						content[index] = 0;
					}
				}
			}
		}
	}

	// ALOGI("after read, count is: %d\n", count);
	// ALOGI("read, content is: %s\n", content);

	jstr = string2jstring(env, content);

	free(content);

	close(fd);

	return jstr;
}


// char* to jstring
/*
static jstring char2jstring(JNIEnv* env, const char* pat)
{
	jclass strClass = env->FindClass("Ljava/lang/String;");
	jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
	jbyteArray bytes = env->NewByteArray(strlen(pat));
	env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*)pat);
	jstring encoding = env->NewStringUTF("utf-8");

	return (jstring)env->NewObject(strClass, ctorID, bytes, encoding);
}
*/


/*
 * Class:     aml_hdmi_in_hdmi_in_boot
 * Method:    writeFileJNI
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_aml_hdmi_1in_hdmi_1in_1boot_writeFileJNI
	(JNIEnv *env, jobject thiz, jstring file, jstring str)
{
	char* fileName = jstring2string(env, file);
	char* cstr = jstring2string(env, str);

	int fd = 0;
	int count = 0;

	 printf("fileName is: %s\n", fileName);
	 printf("cstr is : %s\n", cstr);

	ALOGI("fileName is: %s\n", fileName);
	ALOGI("cstr is: %s\n", cstr);

	fd = open(fileName, O_WRONLY);
	if(fd == -1)
	{
		ALOGI("open file %s failure!\n", fileName);

		return -1;
	}

	count = write(fd, cstr, strlen(cstr));
	ALOGI("count of written chars is: %d\n", count);

	if(count == -1){			//write 10 times if write failed
		int i=0;
		for(i=0;i<10;i++){
			count = write(fd, cstr, strlen(cstr));
			ALOGI("count of written chars is: %d\n", count);
			if(count != -1)
				break;
		}
	}

	close(fd);

	return count;

	/*
	char *fileName = (*env)->GetStringUTFChars(env, file, NULL);
	char *cstr = (*env)->GetStringUTFChars(env, str, NULL);

	int fd = open(fileName, O_WRONLY);
	int count = write(fd, cstr, 100);

	printf("count is: %d\n", count);

	(*env)->ReleaseStringUTFChars(env, file, fileName);
	(*env)->ReleaseStringUTFChars(env, str, cstr);

	return count;
	*/
}

#ifdef __cplusplus
}
#endif
