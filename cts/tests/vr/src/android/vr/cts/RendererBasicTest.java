/*
 * Copyright (C) 2016 The Android Open Source Project
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
package android.vr.cts;

import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.concurrent.CountDownLatch;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class RendererBasicTest implements GLSurfaceView.Renderer {
    private String vertexShaderCode =
              "attribute vec4 vPosition; \n"
            + "void main(){              \n" +
                 "gl_Position = vPosition; \n"
            + "}                         \n";

    private String fragmentShaderCode = "precision mediump float;  \n"
            + "void main(){              \n"
            + " gl_FragColor = vec4 (0.63671875, 0.76953125, 0.22265625, 1.0); \n"
            + "}  \n";


    FloatBuffer floatBuffer;
    int mProgram;
    int maPositionHandle;
    float[] mColorOne = new float[4];

    int[] mShaderCount = null;
    int mError;

    // child may need to manipulate them directly
    protected CountDownLatch mLatch;
    protected boolean mDrawn = false;

    public RendererBasicTest(CountDownLatch latch) {
        mLatch = latch;
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
    }

    public int loadShader(int type, String shaderCode) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, shaderCode);
        GLES20.glCompileShader(shader);
        return shader;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        if (mDrawn) {
            return;
        }
        mDrawn = true;
        doOnDrawFrame(gl);
        mLatch.countDown();
    }

    public void doOnDrawFrame(GL10 gl) {
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
        GLES20.glUseProgram(mProgram);

        GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT,
                false, 12, floatBuffer);
        GLES20.glEnableVertexAttribArray(maPositionHandle);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 3);
        mShaderCount = new int[1];
        int[] shaders = new int[10];
        GLES20.glGetAttachedShaders(mProgram, 10, mShaderCount, 0, shaders, 0);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        GLES20.glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        initShapes();
        int vertexShaderOne = loadShader(GLES20.GL_VERTEX_SHADER, vertexShaderCode);
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentShaderCode);
        mProgram =  GLES20.glCreateProgram();
        GLES20.glAttachShader(mProgram, vertexShaderOne);
        GLES20.glAttachShader(mProgram, fragmentShader);
        GLES20.glLinkProgram(mProgram);
        int[] linkStatus = new int[1];
        GLES20.glGetProgramiv(mProgram, GLES20.GL_LINK_STATUS, linkStatus, 0);
        if (linkStatus[0] != GLES20.GL_TRUE) {
           //do nothing
        }

    }

    public void initShapes(){
        float triangleCoords[] = {   -0.5f, -0.25f, 0,
                 0.5f, -0.25f, 0,
                 0.0f,  0.559016994f, 0};
        ByteBuffer byteBuffer = ByteBuffer.allocateDirect(triangleCoords.length * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        floatBuffer = byteBuffer.asFloatBuffer();
        floatBuffer.put(triangleCoords);
        floatBuffer.position(0);
    }
}
