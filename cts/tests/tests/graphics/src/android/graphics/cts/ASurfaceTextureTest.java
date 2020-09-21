/*
 * Copyright 2017 The Android Open Source Project
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

package android.graphics.cts;

import static android.opengl.EGL14.*;

import android.graphics.Canvas;
import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.support.test.filters.SmallTest;
import android.util.Log;

import android.view.Surface;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

@SmallTest
@RunWith(BlockJUnit4ClassRunner.class)
public class ASurfaceTextureTest {

  static {
    System.loadLibrary("ctsgraphics_jni");
  }

  private static final String TAG = ANativeWindowTest.class.getSimpleName();
  private static final boolean DEBUG = false;

  private EGLDisplay mEglDisplay = EGL_NO_DISPLAY;
  private EGLConfig mEglConfig = null;
  private EGLSurface mEglPbuffer = EGL_NO_SURFACE;
  private EGLContext mEglContext = EGL_NO_CONTEXT;

  @Before
  public void setup() throws Throwable {
    mEglDisplay = EGL14.eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay == EGL_NO_DISPLAY) {
      throw new RuntimeException("no EGL display");
    }
    int[] major = new int[1];
    int[] minor = new int[1];
    if (!EGL14.eglInitialize(mEglDisplay, major, 0, minor, 0)) {
      throw new RuntimeException("error in eglInitialize");
    }

    // If we could rely on having EGL_KHR_surfaceless_context and EGL_KHR_context_no_config, we
    // wouldn't have to create a config or pbuffer at all.

    int[] numConfigs = new int[1];
    EGLConfig[] configs = new EGLConfig[1];
    if (!EGL14.eglChooseConfig(mEglDisplay,
        new int[] {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_NONE},
        0, configs, 0, 1, numConfigs, 0)) {
      throw new RuntimeException("eglChooseConfig failed");
    }
    mEglConfig = configs[0];

    mEglPbuffer = EGL14.eglCreatePbufferSurface(mEglDisplay, mEglConfig,
        new int[] {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE}, 0);
    if (mEglPbuffer == EGL_NO_SURFACE) {
      throw new RuntimeException("eglCreatePbufferSurface failed");
    }

    mEglContext = EGL14.eglCreateContext(mEglDisplay, mEglConfig, EGL_NO_CONTEXT,
        new int[] {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE}, 0);
    if (mEglContext == EGL_NO_CONTEXT) {
      throw new RuntimeException("eglCreateContext failed");
    }

    if (!EGL14.eglMakeCurrent(mEglDisplay, mEglPbuffer, mEglPbuffer, mEglContext)) {
      throw new RuntimeException("eglMakeCurrent failed");
    }
  }

  @Test
  public void testBasic() {
    // create a detached SurfaceTexture
    SurfaceTexture consumer = new SurfaceTexture(false);
    consumer.setDefaultBufferSize(640, 480);
    nBasicTests(consumer);
  }

  private static native void nBasicTests(SurfaceTexture consumer);
}
