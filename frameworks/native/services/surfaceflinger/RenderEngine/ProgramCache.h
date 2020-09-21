/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SF_RENDER_ENGINE_PROGRAMCACHE_H
#define SF_RENDER_ENGINE_PROGRAMCACHE_H

#include <GLES2/gl2.h>

#include <utils/KeyedVector.h>
#include <utils/Singleton.h>
#include <utils/TypeHelpers.h>

#include "Description.h"

namespace android {

class Description;
class Formatter;
class Program;
class String8;

/*
 * This class generates GLSL programs suitable to handle a given
 * Description. It's responsible for figuring out what to
 * generate from a Description.
 * It also maintains a cache of these Programs.
 */
class ProgramCache : public Singleton<ProgramCache> {
public:
    /*
     * Key is used to retrieve a Program in the cache.
     * A Key is generated from a Description.
     */
    class Key {
        friend class ProgramCache;
        typedef uint32_t key_t;
        key_t mKey;

    public:
        enum {
            BLEND_SHIFT = 0,
            BLEND_MASK = 1 << BLEND_SHIFT,
            BLEND_PREMULT = 1 << BLEND_SHIFT,
            BLEND_NORMAL = 0 << BLEND_SHIFT,

            OPACITY_SHIFT = 1,
            OPACITY_MASK = 1 << OPACITY_SHIFT,
            OPACITY_OPAQUE = 1 << OPACITY_SHIFT,
            OPACITY_TRANSLUCENT = 0 << OPACITY_SHIFT,

            ALPHA_SHIFT = 2,
            ALPHA_MASK = 1 << ALPHA_SHIFT,
            ALPHA_LT_ONE = 1 << ALPHA_SHIFT,
            ALPHA_EQ_ONE = 0 << ALPHA_SHIFT,

            TEXTURE_SHIFT = 3,
            TEXTURE_MASK = 3 << TEXTURE_SHIFT,
            TEXTURE_OFF = 0 << TEXTURE_SHIFT,
            TEXTURE_EXT = 1 << TEXTURE_SHIFT,
            TEXTURE_2D = 2 << TEXTURE_SHIFT,

            INPUT_TRANSFORM_MATRIX_SHIFT = 5,
            INPUT_TRANSFORM_MATRIX_MASK = 1 << INPUT_TRANSFORM_MATRIX_SHIFT,
            INPUT_TRANSFORM_MATRIX_OFF = 0 << INPUT_TRANSFORM_MATRIX_SHIFT,
            INPUT_TRANSFORM_MATRIX_ON = 1 << INPUT_TRANSFORM_MATRIX_SHIFT,

            OUTPUT_TRANSFORM_MATRIX_SHIFT = 6,
            OUTPUT_TRANSFORM_MATRIX_MASK = 1 << OUTPUT_TRANSFORM_MATRIX_SHIFT,
            OUTPUT_TRANSFORM_MATRIX_OFF = 0 << OUTPUT_TRANSFORM_MATRIX_SHIFT,
            OUTPUT_TRANSFORM_MATRIX_ON = 1 << OUTPUT_TRANSFORM_MATRIX_SHIFT,

            INPUT_TF_SHIFT = 7,
            INPUT_TF_MASK = 3 << INPUT_TF_SHIFT,
            INPUT_TF_LINEAR = 0 << INPUT_TF_SHIFT,
            INPUT_TF_SRGB = 1 << INPUT_TF_SHIFT,
            INPUT_TF_ST2084 = 2 << INPUT_TF_SHIFT,
            INPUT_TF_HLG = 3 << INPUT_TF_SHIFT,

            OUTPUT_TF_SHIFT = 9,
            OUTPUT_TF_MASK = 3 << OUTPUT_TF_SHIFT,
            OUTPUT_TF_LINEAR = 0 << OUTPUT_TF_SHIFT,
            OUTPUT_TF_SRGB = 1 << OUTPUT_TF_SHIFT,
            OUTPUT_TF_ST2084 = 2 << OUTPUT_TF_SHIFT,
            OUTPUT_TF_HLG = 3 << OUTPUT_TF_SHIFT,

            Y410_BT2020_SHIFT = 11,
            Y410_BT2020_MASK = 1 << Y410_BT2020_SHIFT,
            Y410_BT2020_OFF = 0 << Y410_BT2020_SHIFT,
            Y410_BT2020_ON = 1 << Y410_BT2020_SHIFT,
        };

        inline Key() : mKey(0) {}
        inline Key(const Key& rhs) : mKey(rhs.mKey) {}

        inline Key& set(key_t mask, key_t value) {
            mKey = (mKey & ~mask) | value;
            return *this;
        }

        inline bool isTexturing() const { return (mKey & TEXTURE_MASK) != TEXTURE_OFF; }
        inline int getTextureTarget() const { return (mKey & TEXTURE_MASK); }
        inline bool isPremultiplied() const { return (mKey & BLEND_MASK) == BLEND_PREMULT; }
        inline bool isOpaque() const { return (mKey & OPACITY_MASK) == OPACITY_OPAQUE; }
        inline bool hasAlpha() const { return (mKey & ALPHA_MASK) == ALPHA_LT_ONE; }
        inline bool hasInputTransformMatrix() const {
            return (mKey & INPUT_TRANSFORM_MATRIX_MASK) == INPUT_TRANSFORM_MATRIX_ON;
        }
        inline bool hasOutputTransformMatrix() const {
            return (mKey & OUTPUT_TRANSFORM_MATRIX_MASK) == OUTPUT_TRANSFORM_MATRIX_ON;
        }
        inline bool hasTransformMatrix() const {
            return hasInputTransformMatrix() || hasOutputTransformMatrix();
        }
        inline int getInputTF() const { return (mKey & INPUT_TF_MASK); }
        inline int getOutputTF() const { return (mKey & OUTPUT_TF_MASK); }

        // When HDR and non-HDR contents are mixed, or different types of HDR contents are
        // mixed, we will do a tone mapping process to tone map the input content to output
        // content. Currently, the following conversions handled, they are:
        // * SDR -> HLG
        // * SDR -> PQ
        // * HLG -> PQ
        inline bool needsToneMapping() const {
            int inputTF = getInputTF();
            int outputTF = getOutputTF();

            // Return false when converting from SDR to SDR.
            if (inputTF == Key::INPUT_TF_SRGB && outputTF == Key::OUTPUT_TF_LINEAR) {
                return false;
            }
            if (inputTF == Key::INPUT_TF_LINEAR && outputTF == Key::OUTPUT_TF_SRGB) {
                return false;
            }

            inputTF >>= Key::INPUT_TF_SHIFT;
            outputTF >>= Key::OUTPUT_TF_SHIFT;
            return inputTF != outputTF;
        }
        inline bool isY410BT2020() const { return (mKey & Y410_BT2020_MASK) == Y410_BT2020_ON; }

        // this is the definition of a friend function -- not a method of class Needs
        friend inline int strictly_order_type(const Key& lhs, const Key& rhs) {
            return (lhs.mKey < rhs.mKey) ? 1 : 0;
        }
    };

    ProgramCache();
    ~ProgramCache();

    // Generate shaders to populate the cache
    void primeCache(bool hasWideColor);

    // useProgram lookup a suitable program in the cache or generates one
    // if none can be found.
    void useProgram(const Description& description);

private:
    // compute a cache Key from a Description
    static Key computeKey(const Description& description);
    // Generate EOTF based from Key.
    static void generateEOTF(Formatter& fs, const Key& needs);
    // Generate necessary tone mapping methods for OOTF.
    static void generateToneMappingProcess(Formatter& fs, const Key& needs);
    // Generate OOTF based from Key.
    static void generateOOTF(Formatter& fs, const Key& needs);
    // Generate OETF based from Key.
    static void generateOETF(Formatter& fs, const Key& needs);
    // generates a program from the Key
    static Program* generateProgram(const Key& needs);
    // generates the vertex shader from the Key
    static String8 generateVertexShader(const Key& needs);
    // generates the fragment shader from the Key
    static String8 generateFragmentShader(const Key& needs);

    // Key/Value map used for caching Programs. Currently the cache
    // is never shrunk.
    DefaultKeyedVector<Key, Program*> mCache;
};

ANDROID_BASIC_TYPES_TRAITS(ProgramCache::Key)

} /* namespace android */

#endif /* SF_RENDER_ENGINE_PROGRAMCACHE_H */
