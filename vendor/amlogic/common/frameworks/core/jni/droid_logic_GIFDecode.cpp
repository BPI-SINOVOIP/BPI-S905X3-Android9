
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define LOG_TAG "GIFDecode"

#include <jni.h>
#include <JNIHelp.h>

#include <utils/Log.h>

#include "CreateJavaOutputStreamAdaptor.h"
#include "ScopedLocalRef.h"
#include "GraphicsJNI.h"
#include "SkImageInfo.h"
#include "SkMovie.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkUtils.h"
#include "SkFrontBufferedStream.h"

#include "gif_lib.h"

namespace android
{
    GifFileType* fGIF;
    int fCurrIndex;
    int fLastDrawIndex;
    SkBitmap fBackup;

    static int Decode(GifFileType* fileType, GifByteType* out, int size)
    {
        SkStream* stream = (SkStream*) fileType->UserData;
        return (int) stream->read(out, size);
    }

    void GIFCreate(SkStream* stream)
    {
        fGIF = DGifOpen( stream, Decode, NULL );
        if (NULL == fGIF)
            return;

        if (DGifSlurp(fGIF) != GIF_OK)
        {
            DGifCloseFile(fGIF, NULL);
            fGIF = NULL;
        }
        fCurrIndex = -1;
        fLastDrawIndex = -1;
    }

    void GIFDestructor()
    {
        if (fGIF)
            DGifCloseFile(fGIF, NULL);
    }

    static SkMSec savedimage_duration(const SavedImage* image)
    {
        for (int j = 0; j < image->ExtensionBlockCount; j++) {
            if (image->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {
                SkASSERT(image->ExtensionBlocks[j].ByteCount >= 4);
                const uint8_t* b = (const uint8_t*)image->ExtensionBlocks[j].Bytes;
                return ((b[2] << 8) | b[1]) * 10;
            }
        }
        return 0;
    }

    bool onSetTime(SkMSec time)
    {
        if (NULL == fGIF)
            return false;

        SkMSec dur = 0;
        for (int i = 0; i < fGIF->ImageCount; i++) {
            dur += savedimage_duration(&fGIF->SavedImages[i]);
            if (dur >= time) {
                fCurrIndex = i;
                return fLastDrawIndex != fCurrIndex;
            }
        }
        fCurrIndex = fGIF->ImageCount - 1;
        return true;
    }

    static void copyLine(uint32_t* dst, const unsigned char* src, const ColorMapObject* cmap,
                         int transparent, int width)
    {
        for (; width > 0; width--, src++, dst++) {
            if (*src != transparent) {
                const GifColorType& col = cmap->Colors[*src];
                *dst = SkPackARGB32(0xFF, col.Red, col.Green, col.Blue);
            }
        }
    }

    static void blitNormal(SkBitmap* bm, const SavedImage* frame, const ColorMapObject* cmap,
                           int transparent)
    {
        int width = bm->width();
        int height = bm->height();
        const unsigned char* src = (unsigned char*)frame->RasterBits;
        uint32_t* dst = bm->getAddr32(frame->ImageDesc.Left, frame->ImageDesc.Top);
        GifWord copyWidth = frame->ImageDesc.Width;
        if (frame->ImageDesc.Left + copyWidth > width) {
            copyWidth = width - frame->ImageDesc.Left;
        }

        GifWord copyHeight = frame->ImageDesc.Height;
        if (frame->ImageDesc.Top + copyHeight > height) {
            copyHeight = height - frame->ImageDesc.Top;
        }

        for (; copyHeight > 0; copyHeight--) {
            copyLine(dst, src, cmap, transparent, copyWidth);
            src += frame->ImageDesc.Width;
            dst += width;
        }
    }

    static void fillRect(SkBitmap* bm, GifWord left, GifWord top, GifWord width, GifWord height,
                         uint32_t col)
    {
        int bmWidth = bm->width();
        int bmHeight = bm->height();
        uint32_t* dst = bm->getAddr32(left, top);
        GifWord copyWidth = width;
        if (left + copyWidth > bmWidth) {
            copyWidth = bmWidth - left;
        }

        GifWord copyHeight = height;
        if (top + copyHeight > bmHeight) {
            copyHeight = bmHeight - top;
        }

        for (; copyHeight > 0; copyHeight--) {
            sk_memset32(dst, col, copyWidth);
            dst += bmWidth;
        }
    }

    static void drawFrame(SkBitmap* bm, const SavedImage* frame, const ColorMapObject* cmap)
    {
        int transparent = -1;

        for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
            ExtensionBlock* eb = frame->ExtensionBlocks + i;
            if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
                eb->ByteCount == 4) {
                bool has_transparency = ((eb->Bytes[0] & 1) == 1);
                if (has_transparency) {
                    transparent = (unsigned char)eb->Bytes[3];
                }
            }
        }

        if (frame->ImageDesc.ColorMap != NULL) {
            // use local color table
            cmap = frame->ImageDesc.ColorMap;
        }

        if (cmap == NULL || cmap->ColorCount != (1 << cmap->BitsPerPixel)) {
            SkDEBUGFAIL("bad colortable setup");
            return;
        }
        blitNormal(bm, frame, cmap, transparent);
    }

    static bool checkIfWillBeCleared(const SavedImage* frame)
    {
        for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
            ExtensionBlock* eb = frame->ExtensionBlocks + i;
            if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
                eb->ByteCount == 4) {
                // check disposal method
                int disposal = ((eb->Bytes[0] >> 2) & 7);
                if (disposal == 2 || disposal == 3) {
                    return true;
                }
            }
        }
        return false;
    }

    static void getTransparencyAndDisposalMethod(const SavedImage* frame, bool* trans, int* disposal)
    {
        *trans = false;
        *disposal = 0;
        for (int i = 0; i < frame->ExtensionBlockCount; ++i) {
            ExtensionBlock* eb = frame->ExtensionBlocks + i;
            if (eb->Function == GRAPHICS_EXT_FUNC_CODE &&
                eb->ByteCount == 4) {
                *trans = ((eb->Bytes[0] & 1) == 1);
                *disposal = ((eb->Bytes[0] >> 2) & 7);
            }
        }
    }

    // return true if area of 'target' is completely covers area of 'covered'
    static bool checkIfCover(const SavedImage* target, const SavedImage* covered)
    {
        if (target->ImageDesc.Left <= covered->ImageDesc.Left
            && covered->ImageDesc.Left + covered->ImageDesc.Width <=
                   target->ImageDesc.Left + target->ImageDesc.Width
            && target->ImageDesc.Top <= covered->ImageDesc.Top
            && covered->ImageDesc.Top + covered->ImageDesc.Height <=
                   target->ImageDesc.Top + target->ImageDesc.Height) {
            return true;
        }
        return false;
    }

    static void disposeFrameIfNeeded(SkBitmap* bm, const SavedImage* cur, const SavedImage* next,
                                     SkBitmap* backup, SkColor color)
    {
        // We can skip disposal process if next frame is not transparent
        // and completely covers current area
        bool curTrans;
        int curDisposal;
        getTransparencyAndDisposalMethod(cur, &curTrans, &curDisposal);
        bool nextTrans;
        int nextDisposal;
        getTransparencyAndDisposalMethod(next, &nextTrans, &nextDisposal);
        if ((curDisposal == 2 || curDisposal == 3)
            && (nextTrans || !checkIfCover(next, cur))) {
            switch (curDisposal) {
            // restore to background color
            // -> 'background' means background under this image.
            case 2:
                fillRect(bm, cur->ImageDesc.Left, cur->ImageDesc.Top,
                         cur->ImageDesc.Width, cur->ImageDesc.Height,
                         color);
                break;

            // restore to previous
            case 3:
                bm->swap(*backup);
                break;
            }
        }

        // Save current image if next frame's disposal method == 3
        if (nextDisposal == 3) {
            const uint32_t* src = bm->getAddr32(0, 0);
            uint32_t* dst = backup->getAddr32(0, 0);
            int cnt = bm->width() * bm->height();
            memcpy(dst, src, cnt*sizeof(uint32_t));
        }
    }

    bool onGetBitmap(SkBitmap* bm)
    {
        const GifFileType* gif = fGIF;
        if (NULL == gif)
            return false;

        if (gif->ImageCount < 1) {
            return false;
        }

        const int width = gif->SWidth;
        const int height = gif->SHeight;
        if (width <= 0 || height <= 0) {
            return false;
        }

        // no need to draw
        if (fLastDrawIndex >= 0 && fLastDrawIndex == fCurrIndex) {
            return true;
        }

        int startIndex = fLastDrawIndex + 1;
        if (fLastDrawIndex < 0 || !bm->readyToDraw()) {
            // first time

            startIndex = 0;

            // create bitmap
            if (!bm->tryAllocN32Pixels(width, height)) {
                return false;
            }
            // create bitmap for backup
            if (!fBackup.tryAllocN32Pixels(width, height)) {
                return false;
            }
        } else if (startIndex > fCurrIndex) {
            // rewind to 1st frame for repeat
            startIndex = 0;
        }

        int lastIndex = fCurrIndex;
        if (lastIndex < 0) {
            // first time
            lastIndex = 0;
        } else if (lastIndex > fGIF->ImageCount - 1) {
            // this block must not be reached.
            lastIndex = fGIF->ImageCount - 1;
        }

        SkColor bgColor = SkPackARGB32(0, 0, 0, 0);
        if (gif->SColorMap != NULL) {
            const GifColorType& col = gif->SColorMap->Colors[fGIF->SBackGroundColor];
            bgColor = SkColorSetARGB(0xFF, col.Red, col.Green, col.Blue);
        }

        static SkColor paintingColor = SkPackARGB32(0, 0, 0, 0);
        // draw each frames - not intelligent way
        for (int i = startIndex; i <= lastIndex; i++) {
            const SavedImage* cur = &fGIF->SavedImages[i];
            if (i == 0) {
                bool trans;
                int disposal;
                getTransparencyAndDisposalMethod(cur, &trans, &disposal);
                if (!trans && gif->SColorMap != NULL) {
                    paintingColor = bgColor;
                } else {
                    paintingColor = SkColorSetARGB(0, 0, 0, 0);
                }

                bm->eraseColor(paintingColor);
                fBackup.eraseColor(paintingColor);
            } else {
                // Dispose previous frame before move to next frame.
                const SavedImage* prev = &fGIF->SavedImages[i-1];
                disposeFrameIfNeeded(bm, prev, cur, &fBackup, paintingColor);
            }

            // Draw frame
            // We can skip this process if this index is not last and disposal
            // method == 2 or method == 3
            if (i == lastIndex || !checkIfWillBeCleared(cur)) {
                drawFrame(bm, cur, gif->SColorMap);
            }
        }

        // save index
        fLastDrawIndex = lastIndex;
        return true;
    }

    SkBitmap* createFrameBitmap()
    {
        //get default bitmap, create a new bitmap and returns.
        SkBitmap fBitmap;
        SkBitmap *copyedBitmap = new SkBitmap();
        bool copyDone = false;
        if (!onGetBitmap(&fBitmap)) {// failure
            fBitmap.reset();
        }
        //now create a new bitmap from fBitmap
        if (fBitmap.canCopyTo(kRGB_565_SkColorType)) {
            copyDone = fBitmap.copyTo(copyedBitmap, kRGB_565_SkColorType);
        } else {
            copyDone = false;
        }
        if (copyDone) {
            return copyedBitmap;
        } else {
            return NULL;
        }
    }

    static void nativeDecodeStream(JNIEnv* env, jobject clazz, jobject istream) {
        jbyteArray byteArray = env->NewByteArray(16*1024);
        ScopedLocalRef<jbyteArray> scoper(env, byteArray);
        SkStream* strm = CreateJavaInputStreamAdaptor(env, istream, byteArray);
        if (NULL == strm) {
            return;
        }
        GIFCreate(strm);
        SkAutoTDelete<SkStreamRewindable> bufferedStream(SkFrontBufferedStream::Create(strm, 6));
        SkASSERT(bufferedStream.get() != NULL);
        //strm->unref();
    }

    static void nativeDestructor(JNIEnv* env, jobject clazz) {
        GIFDestructor();
    }

    static jint nativeWidth(JNIEnv *env, jobject clazz) {
        if (NULL == fGIF) return -1;
        return fGIF->SWidth;
    }

    static jint nativeHeight(JNIEnv *env, jobject clazz) {
        if (NULL == fGIF) return -1;
        return fGIF->SHeight;
    }

    static jint nativeTotalDuration(JNIEnv *env, jobject clazz) {
        if (NULL == fGIF) return -1;

        SkMSec dur = 0;
        for (int i = 0; i < fGIF->ImageCount; i++) {
            dur += savedimage_duration(&fGIF->SavedImages[i]);
        }
        return dur;
    }

    static jboolean nativeSetCurrFrame(JNIEnv *env, jobject clazz, jint frameIndex) {
        if (NULL == fGIF) {
            return false;
        }
        if (frameIndex >= 0 && frameIndex < fGIF->ImageCount) {
            fCurrIndex = frameIndex;
        } else {
            fCurrIndex = 0;
        }
        return true;
    }

    static jint nativeGetFrameDuration(JNIEnv *env, jobject clazz, jint frameIndex) {
        //for wrong frame index, return 0
        if (frameIndex < 0 || NULL == fGIF || frameIndex >= fGIF->ImageCount) {
            return 0;
        }
        return savedimage_duration(&fGIF->SavedImages[frameIndex]);
    }

    static jint nativeGetFrameCount(JNIEnv *env, jobject clazz) {
        //if fGIF is not valid, return 0
        if (NULL == fGIF) {
            return 0;
        }
        return fGIF->ImageCount < 0 ? 0 : fGIF->ImageCount;
    }

    static jobject nativeGetFrameBitmap(JNIEnv *env, jobject clazz, jint frameIndex) {
        int frameCount = fGIF->ImageCount;
        if (frameIndex < 0 && frameIndex >= frameCount ) {
            return NULL;
        }
        fCurrIndex = frameIndex;
        SkBitmap result;
        SkBitmap *createdBitmap = createFrameBitmap();
        if (createdBitmap != NULL) {
            JavaPixelAllocator  allocator(env);
            if (createdBitmap->copyTo(&result, &allocator)) {
                Bitmap* bitmap = allocator.getStorageObjAndReset();
                if (bitmap != NULL)
                    return GraphicsJNI::createBitmap(env, bitmap, false);
            }
        }
        return NULL;
    }

    static JNINativeMethod sMethods[] = {
        {"nativeDecodeStream", "(Ljava/io/InputStream;)V", (void*)nativeDecodeStream},
        {"nativeDestructor", "()V", (void*)nativeDestructor},
        {"nativeWidth", "()I", (void*)nativeWidth},
        {"nativeHeight", "()I", (void*)nativeHeight},
        {"nativeTotalDuration", "()I", (void*)nativeTotalDuration},
        {"nativeSetCurrFrame", "(I)Z", (void*)nativeSetCurrFrame},
        {"nativeGetFrameDuration", "(I)I", (void*)nativeGetFrameDuration},
        {"nativeGetFrameCount", "()I", (void*)nativeGetFrameCount},
        {"nativeGetFrameBitmap", "(I)Landroid/graphics/Bitmap;", (void*)nativeGetFrameBitmap},
    };

    int register_android_GIFDecode(JNIEnv* env) {
        return jniRegisterNativeMethods(env, "com/droidlogic/app/GIFDecodesManager",
                                  sMethods, NELEM(sMethods));
    }
}

using namespace android;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_android_GIFDecode(env);

    return JNI_VERSION_1_4;
}
