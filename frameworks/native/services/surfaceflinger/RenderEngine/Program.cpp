/*Gluint
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

#include <stdint.h>

#include <log/log.h>
#include <utils/String8.h>

#include <math/mat4.h>
#include "Description.h"
#include "Program.h"
#include "ProgramCache.h"

namespace android {

Program::Program(const ProgramCache::Key& /*needs*/, const char* vertex, const char* fragment)
      : mInitialized(false) {
    GLuint vertexId = buildShader(vertex, GL_VERTEX_SHADER);
    GLuint fragmentId = buildShader(fragment, GL_FRAGMENT_SHADER);
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexId);
    glAttachShader(programId, fragmentId);
    glBindAttribLocation(programId, position, "position");
    glBindAttribLocation(programId, texCoords, "texCoords");
    glLinkProgram(programId);

    GLint status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        ALOGE("Error while linking shaders:");
        GLint infoLen = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            GLchar log[infoLen];
            glGetProgramInfoLog(programId, infoLen, 0, &log[0]);
            ALOGE("%s", log);
        }
        glDetachShader(programId, vertexId);
        glDetachShader(programId, fragmentId);
        glDeleteShader(vertexId);
        glDeleteShader(fragmentId);
        glDeleteProgram(programId);
    } else {
        mProgram = programId;
        mVertexShader = vertexId;
        mFragmentShader = fragmentId;
        mInitialized = true;
        mProjectionMatrixLoc = glGetUniformLocation(programId, "projection");
        mTextureMatrixLoc = glGetUniformLocation(programId, "texture");
        mSamplerLoc = glGetUniformLocation(programId, "sampler");
        mColorLoc = glGetUniformLocation(programId, "color");
        mDisplayMaxLuminanceLoc = glGetUniformLocation(programId, "displayMaxLuminance");
        mInputTransformMatrixLoc = glGetUniformLocation(programId, "inputTransformMatrix");
        mOutputTransformMatrixLoc = glGetUniformLocation(programId, "outputTransformMatrix");

        // set-up the default values for our uniforms
        glUseProgram(programId);
        glUniformMatrix4fv(mProjectionMatrixLoc, 1, GL_FALSE, mat4().asArray());
        glEnableVertexAttribArray(0);
    }
}

Program::~Program() {}

bool Program::isValid() const {
    return mInitialized;
}

void Program::use() {
    glUseProgram(mProgram);
}

GLuint Program::getAttrib(const char* name) const {
    // TODO: maybe use a local cache
    return glGetAttribLocation(mProgram, name);
}

GLint Program::getUniform(const char* name) const {
    // TODO: maybe use a local cache
    return glGetUniformLocation(mProgram, name);
}

GLuint Program::buildShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        // Some drivers return wrong values for GL_INFO_LOG_LENGTH
        // use a fixed size instead
        GLchar log[512];
        glGetShaderInfoLog(shader, sizeof(log), 0, log);
        ALOGE("Error while compiling shader: \n%s\n%s", source, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

String8& Program::dumpShader(String8& result, GLenum /*type*/) {
    GLuint shader = GL_FRAGMENT_SHADER ? mFragmentShader : mVertexShader;
    GLint l;
    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &l);
    char* src = new char[l];
    glGetShaderSource(shader, l, nullptr, src);
    result.append(src);
    delete[] src;
    return result;
}

void Program::setUniforms(const Description& desc) {
    // TODO: we should have a mechanism here to not always reset uniforms that
    // didn't change for this program.

    if (mSamplerLoc >= 0) {
        glUniform1i(mSamplerLoc, 0);
        glUniformMatrix4fv(mTextureMatrixLoc, 1, GL_FALSE, desc.mTexture.getMatrix().asArray());
    }
    if (mColorLoc >= 0) {
        const float color[4] = {desc.mColor.r, desc.mColor.g, desc.mColor.b, desc.mColor.a};
        glUniform4fv(mColorLoc, 1, color);
    }
    if (mInputTransformMatrixLoc >= 0) {
        // If the input transform matrix is not identity matrix, we want to merge
        // the saturation matrix with input transform matrix so that the saturation
        // matrix is applied at the correct stage.
        mat4 inputTransformMatrix = mat4(desc.mInputTransformMatrix) * desc.mSaturationMatrix;
        glUniformMatrix4fv(mInputTransformMatrixLoc, 1, GL_FALSE, inputTransformMatrix.asArray());
    }
    if (mOutputTransformMatrixLoc >= 0) {
        // The output transform matrix and color matrix can be combined as one matrix
        // that is applied right before applying OETF.
        mat4 outputTransformMatrix = desc.mColorMatrix * desc.mOutputTransformMatrix;
        // If there is no input transform matrix, we want to merge the saturation
        // matrix with output transform matrix to avoid extra matrix multiplication
        // in shader.
        if (mInputTransformMatrixLoc < 0) {
            outputTransformMatrix *= desc.mSaturationMatrix;
        }
        glUniformMatrix4fv(mOutputTransformMatrixLoc, 1, GL_FALSE,
                           outputTransformMatrix.asArray());
    }
    if (mDisplayMaxLuminanceLoc >= 0) {
        glUniform1f(mDisplayMaxLuminanceLoc, desc.mDisplayMaxLuminance);
    }
    // these uniforms are always present
    glUniformMatrix4fv(mProjectionMatrixLoc, 1, GL_FALSE, desc.mProjectionMatrix.asArray());
}

} /* namespace android */
