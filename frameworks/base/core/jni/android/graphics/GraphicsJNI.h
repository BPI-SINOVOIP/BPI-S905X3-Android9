#ifndef _ANDROID_GRAPHICS_GRAPHICS_JNI_H_
#define _ANDROID_GRAPHICS_GRAPHICS_JNI_H_

#include "Bitmap.h"
#include "SkBitmap.h"
#include "SkBRDAllocator.h"
#include "SkCodec.h"
#include "SkPixelRef.h"
#include "SkMallocPixelRef.h"
#include "SkPoint.h"
#include "SkRect.h"
#include "SkColorSpace.h"
#include "SkMatrix44.h"
#include <jni.h>
#include <hwui/Canvas.h>
#include <hwui/Bitmap.h>

class SkBitmapRegionDecoder;
class SkCanvas;

namespace android {
class Paint;
struct Typeface;
}

class GraphicsJNI {
public:
    // This enum must keep these int values, to match the int values
    // in the java Bitmap.Config enum.
    enum LegacyBitmapConfig {
        kNo_LegacyBitmapConfig          = 0,
        kA8_LegacyBitmapConfig          = 1,
        kIndex8_LegacyBitmapConfig      = 2,
        kRGB_565_LegacyBitmapConfig     = 3,
        kARGB_4444_LegacyBitmapConfig   = 4,
        kARGB_8888_LegacyBitmapConfig   = 5,
        kRGBA_16F_LegacyBitmapConfig    = 6,
        kHardware_LegacyBitmapConfig    = 7,

        kLastEnum_LegacyBitmapConfig = kHardware_LegacyBitmapConfig
    };

    // returns true if an exception is set (and dumps it out to the Log)
    static bool hasException(JNIEnv*);

    static void get_jrect(JNIEnv*, jobject jrect, int* L, int* T, int* R, int* B);
    static void set_jrect(JNIEnv*, jobject jrect, int L, int T, int R, int B);

    static SkIRect* jrect_to_irect(JNIEnv*, jobject jrect, SkIRect*);
    static void irect_to_jrect(const SkIRect&, JNIEnv*, jobject jrect);

    static SkRect* jrectf_to_rect(JNIEnv*, jobject jrectf, SkRect*);
    static SkRect* jrect_to_rect(JNIEnv*, jobject jrect, SkRect*);
    static void rect_to_jrectf(const SkRect&, JNIEnv*, jobject jrectf);

    static void set_jpoint(JNIEnv*, jobject jrect, int x, int y);

    static SkIPoint* jpoint_to_ipoint(JNIEnv*, jobject jpoint, SkIPoint* point);
    static void ipoint_to_jpoint(const SkIPoint& point, JNIEnv*, jobject jpoint);

    static SkPoint* jpointf_to_point(JNIEnv*, jobject jpointf, SkPoint* point);
    static void point_to_jpointf(const SkPoint& point, JNIEnv*, jobject jpointf);

    static android::Canvas* getNativeCanvas(JNIEnv*, jobject canvas);
    static void getSkBitmap(JNIEnv*, jobject bitmap, SkBitmap* outBitmap);
    static SkPixelRef* refSkPixelRef(JNIEnv*, jobject bitmap);
    static SkRegion* getNativeRegion(JNIEnv*, jobject region);

    /*
     *  LegacyBitmapConfig is the old enum in Skia that matched the enum int values
     *  in Bitmap.Config. Skia no longer supports this config, but has replaced it
     *  with SkColorType. These routines convert between the two.
     */
    static SkColorType legacyBitmapConfigToColorType(jint legacyConfig);
    static jint colorTypeToLegacyBitmapConfig(SkColorType colorType);

    /** Return the corresponding native colorType from the java Config enum,
        or kUnknown_SkColorType if the java object is null.
    */
    static SkColorType getNativeBitmapColorType(JNIEnv*, jobject jconfig);

    static bool isHardwareConfig(JNIEnv* env, jobject jconfig);
    static jint hardwareLegacyBitmapConfig();

    static jobject createRegion(JNIEnv* env, SkRegion* region);

    static jobject createBitmapRegionDecoder(JNIEnv* env, SkBitmapRegionDecoder* bitmap);

    static android::Bitmap* mapAshmemBitmap(JNIEnv* env, SkBitmap* bitmap,
            int fd, void* addr, size_t size, bool readOnly);

    /**
     * Given a bitmap we natively allocate a memory block to store the contents
     * of that bitmap.  The memory is then attached to the bitmap via an
     * SkPixelRef, which ensures that upon deletion the appropriate caches
     * are notified.
     */
    static bool allocatePixels(JNIEnv* env, SkBitmap* bitmap);

    /** Copy the colors in colors[] to the bitmap, convert to the correct
        format along the way.
        Whether to use premultiplied pixels is determined by dstBitmap's alphaType.
    */
    static bool SetPixels(JNIEnv* env, jintArray colors, int srcOffset,
            int srcStride, int x, int y, int width, int height,
            const SkBitmap& dstBitmap);

    static sk_sp<SkColorSpace> defaultColorSpace();
    static sk_sp<SkColorSpace> linearColorSpace();
    static sk_sp<SkColorSpace> colorSpaceForType(SkColorType type);
    static bool isColorSpaceSRGB(SkColorSpace* colorSpace);

    static SkColorSpaceTransferFn getNativeTransferParameters(JNIEnv* env, jobject transferParams);
    static SkMatrix44 getNativeXYZMatrix(JNIEnv* env, jfloatArray xyzD50);
    static sk_sp<SkColorSpace> getNativeColorSpace(JNIEnv* env, jobject colorSpace);

    static jobject getColorSpace(JNIEnv* env, sk_sp<SkColorSpace>& decodeColorSpace,
            SkColorType decodeColorType);
};

class HeapAllocator : public SkBRDAllocator {
public:
   HeapAllocator() { };
    ~HeapAllocator() { };

    virtual bool allocPixelRef(SkBitmap* bitmap) override;

    /**
     * Fetches the backing allocation object. Must be called!
     */
    android::Bitmap* getStorageObjAndReset() {
        return mStorage.release();
    };

    SkCodec::ZeroInitialized zeroInit() const override { return SkCodec::kYes_ZeroInitialized; }
private:
    sk_sp<android::Bitmap> mStorage;
};

/**
 *  Allocator to handle reusing bitmaps for BitmapRegionDecoder.
 *
 *  The BitmapRegionDecoder documentation states that, if it is
 *  provided, the recycled bitmap will always be reused, clipping
 *  the decoded output to fit in the recycled bitmap if necessary.
 *  This allocator implements that behavior.
 *
 *  Skia's SkBitmapRegionDecoder expects the memory that
 *  is allocated to be large enough to decode the entire region
 *  that is requested.  It will decode directly into the memory
 *  that is provided.
 *
 *  FIXME: BUG:25465958
 *  If the recycled bitmap is not large enough for the decode
 *  requested, meaning that a clip is required, we will allocate
 *  enough memory for Skia to perform the decode, and then copy
 *  from the decoded output into the recycled bitmap.
 *
 *  If the recycled bitmap is large enough for the decode requested,
 *  we will provide that memory for Skia to decode directly into.
 *
 *  This allocator should only be used for a single allocation.
 *  After we reuse the recycledBitmap once, it is dangerous to
 *  reuse it again, given that it still may be in use from our
 *  first allocation.
 */
class RecyclingClippingPixelAllocator : public SkBRDAllocator {
public:

    RecyclingClippingPixelAllocator(android::Bitmap* recycledBitmap,
            size_t recycledBytes);

    ~RecyclingClippingPixelAllocator();

    virtual bool allocPixelRef(SkBitmap* bitmap) override;

    /**
     *  Must be called!
     *
     *  In the event that the recycled bitmap is not large enough for
     *  the allocation requested, we will allocate memory on the heap
     *  instead.  As a final step, once we are done using this memory,
     *  we will copy the contents of the heap memory into the recycled
     *  bitmap's memory, clipping as necessary.
     */
    void copyIfNecessary();

    /**
     *  Indicates that this allocator does not allocate zero initialized
     *  memory.
     */
    SkCodec::ZeroInitialized zeroInit() const override { return SkCodec::kNo_ZeroInitialized; }

private:
    android::Bitmap* mRecycledBitmap;
    const size_t     mRecycledBytes;
    SkBitmap*        mSkiaBitmap;
    bool             mNeedsCopy;
};

class AshmemPixelAllocator : public SkBitmap::Allocator {
public:
    explicit AshmemPixelAllocator(JNIEnv* env);
    ~AshmemPixelAllocator() { };
    virtual bool allocPixelRef(SkBitmap* bitmap);
    android::Bitmap* getStorageObjAndReset() {
        return mStorage.release();
    };

private:
    JavaVM* mJavaVM;
    sk_sp<android::Bitmap> mStorage;
};


enum JNIAccess {
    kRO_JNIAccess,
    kRW_JNIAccess
};

class AutoJavaFloatArray {
public:
    AutoJavaFloatArray(JNIEnv* env, jfloatArray array,
                       int minLength = 0, JNIAccess = kRW_JNIAccess);
    ~AutoJavaFloatArray();

    float* ptr() const { return fPtr; }
    int    length() const { return fLen; }

private:
    JNIEnv*     fEnv;
    jfloatArray fArray;
    float*      fPtr;
    int         fLen;
    int         fReleaseMode;
};

class AutoJavaIntArray {
public:
    AutoJavaIntArray(JNIEnv* env, jintArray array, int minLength = 0);
    ~AutoJavaIntArray();

    jint* ptr() const { return fPtr; }
    int    length() const { return fLen; }

private:
    JNIEnv*     fEnv;
    jintArray fArray;
    jint*      fPtr;
    int         fLen;
};

class AutoJavaShortArray {
public:
    AutoJavaShortArray(JNIEnv* env, jshortArray array,
                       int minLength = 0, JNIAccess = kRW_JNIAccess);
    ~AutoJavaShortArray();

    jshort* ptr() const { return fPtr; }
    int    length() const { return fLen; }

private:
    JNIEnv*     fEnv;
    jshortArray fArray;
    jshort*      fPtr;
    int         fLen;
    int         fReleaseMode;
};

class AutoJavaByteArray {
public:
    AutoJavaByteArray(JNIEnv* env, jbyteArray array, int minLength = 0);
    ~AutoJavaByteArray();

    jbyte* ptr() const { return fPtr; }
    int    length() const { return fLen; }

private:
    JNIEnv*     fEnv;
    jbyteArray fArray;
    jbyte*      fPtr;
    int         fLen;
};

void doThrowNPE(JNIEnv* env);
void doThrowAIOOBE(JNIEnv* env); // Array Index Out Of Bounds Exception
void doThrowIAE(JNIEnv* env, const char* msg = NULL);   // Illegal Argument
void doThrowRE(JNIEnv* env, const char* msg = NULL);   // Runtime
void doThrowISE(JNIEnv* env, const char* msg = NULL);   // Illegal State
void doThrowOOME(JNIEnv* env, const char* msg = NULL);   // Out of memory
void doThrowIOE(JNIEnv* env, const char* msg = NULL);   // IO Exception

#define NPE_CHECK_RETURN_ZERO(env, object)    \
    do { if (NULL == (object)) { doThrowNPE(env); return 0; } } while (0)

#define NPE_CHECK_RETURN_VOID(env, object)    \
    do { if (NULL == (object)) { doThrowNPE(env); return; } } while (0)

#endif  // _ANDROID_GRAPHICS_GRAPHICS_JNI_H_
