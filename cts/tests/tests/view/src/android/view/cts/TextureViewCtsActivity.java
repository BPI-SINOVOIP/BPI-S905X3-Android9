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

package android.view.cts;

import static android.opengl.GLES20.GL_COLOR_BUFFER_BIT;
import static android.opengl.GLES20.glClear;
import static android.opengl.GLES20.glClearColor;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.ColorSpace;
import android.graphics.Matrix;
import android.graphics.SurfaceTexture;
import android.opengl.GLUtils;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.FrameLayout;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

public class TextureViewCtsActivity extends Activity implements SurfaceTextureListener {
    private final static long TIME_OUT_MS = 10000;
    private final Object mLock = new Object();

    private View mPreview;
    private TextureView mTextureView;
    private HandlerThread mGLThreadLooper;
    private Handler mGLThread;
    private CountDownLatch mEnterAnimationFence = new CountDownLatch(1);

    private SurfaceTexture mSurface;
    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private int mSurfaceUpdatedCount;

    private int mEglColorSpace = 0;
    private boolean mIsEGLWideGamut = false;
    private boolean mEGLExtensionUnsupported = false;

    static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    static final int EGL_OPENGL_ES2_BIT = 4;
    static final int EGL_GL_COLORSPACE_KHR = 0x309D;
    static final int EGL_COLOR_COMPONENT_TYPE_EXT = 0x3339;
    static final int EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT = 0x333B;

    private EGL10 mEgl;
    private EGLDisplay mEglDisplay;
    private EGLConfig mEglConfig;
    private EGLContext mEglContext;
    private EGLSurface mEglSurface;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mGLThreadLooper == null) {
            mGLThreadLooper = new HandlerThread("GLThread");
            mGLThreadLooper.start();
            mGLThread = new Handler(mGLThreadLooper.getLooper());
        }

        View preview = new View(this);
        preview.setBackgroundColor(Color.WHITE);
        mPreview = preview;
        mTextureView = new TextureView(this);
        mTextureView.setSurfaceTextureListener(this);

        FrameLayout content = new FrameLayout(this);
        content.setBackgroundColor(Color.BLACK);
        content.addView(mTextureView,
                LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        content.addView(mPreview,
                LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);

        setContentView(content);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            runOnGLThread(this::doFinishGL);
        } catch (Throwable t) {
            throw new RuntimeException(t);
        }
    }

    @Override
    public void onEnterAnimationComplete() {
        super.onEnterAnimationComplete();
        mEnterAnimationFence.countDown();
    }

    public void waitForEnterAnimationComplete() throws TimeoutException, InterruptedException {
        if (!mEnterAnimationFence.await(TIME_OUT_MS, TimeUnit.MILLISECONDS)) {
            throw new TimeoutException();
        }
    }

    public boolean setWideColorGamut() throws Throwable {
        CountDownLatch fence = new CountDownLatch(1);
        RunSignalAndCatch wrapper = new RunSignalAndCatch(() -> {
            this.getWindow().setColorMode(ActivityInfo.COLOR_MODE_WIDE_COLOR_GAMUT);
        }, fence);
        runOnUiThread(wrapper);
        if (!fence.await(TIME_OUT_MS, TimeUnit.MILLISECONDS)) {
            throw new TimeoutException();
        }
        if (wrapper.error != null) {
            throw wrapper.error;
        }
        return this.getWindow().getColorMode() == ActivityInfo.COLOR_MODE_WIDE_COLOR_GAMUT;
    }

    public Bitmap getContents(Bitmap.Config config, ColorSpace colorSpace) throws Throwable {
        CountDownLatch fence = new CountDownLatch(1);
        final Bitmap bitmap = Bitmap.createBitmap(this.getWindow().getDecorView().getWidth(),
                this.getWindow().getDecorView().getHeight(), config, true, colorSpace);
        RunSignalAndCatch wrapper = new RunSignalAndCatch(() -> {
            this.getTextureView().getBitmap(bitmap);
        }, fence);
        runOnUiThread(wrapper);
        if (!fence.await(TIME_OUT_MS, TimeUnit.MILLISECONDS)) {
            throw new TimeoutException();
        }
        if (wrapper.error != null) {
            throw wrapper.error;
        }
        return bitmap;
    }

    private class RunSignalAndCatch implements Runnable {
        public Throwable error;
        private Runnable mRunnable;
        private CountDownLatch mFence;

        RunSignalAndCatch(Runnable run, CountDownLatch fence) {
            mRunnable = run;
            mFence = fence;
        }

        @Override
        public void run() {
            try {
                mRunnable.run();
            } catch (Throwable t) {
                error = t;
            } finally {
                mFence.countDown();
            }
        }
    }

    private void runOnGLThread(Runnable r) throws Throwable {
        CountDownLatch fence = new CountDownLatch(1);
        RunSignalAndCatch wrapper = new RunSignalAndCatch(r, fence);
        mGLThread.post(wrapper);
        if (!fence.await(TIME_OUT_MS, TimeUnit.MILLISECONDS)) {
            throw new TimeoutException();
        }
        if (wrapper.error != null) {
            throw wrapper.error;
        }
    }

    public TextureView getTextureView() {
        return mTextureView;
    }

    public void waitForSurface() throws InterruptedException {
        synchronized (mLock) {
            while (mSurface == null) {
                mLock.wait(TIME_OUT_MS);
            }
        }
    }

    public boolean initGLExtensionUnsupported() {
        return mEGLExtensionUnsupported;
    }

    public void initGl() throws Throwable {
        initGl(0, false);
    }

    public void initGl(int eglColorSpace, boolean useHalfFloat) throws Throwable {
        if (mEglSurface != null) {
            if (eglColorSpace != mEglColorSpace || useHalfFloat != mIsEGLWideGamut) {
                throw new RuntimeException("Cannot change config after initialization");
            }
            return;
        }
        mEglColorSpace = eglColorSpace;
        mIsEGLWideGamut = useHalfFloat;
        mEGLExtensionUnsupported = false;
        runOnGLThread(mDoInitGL);
    }

    public void drawColor(int color) throws Throwable {
        drawColor(Color.red(color) / 255.0f,
                Color.green(color) / 255.0f,
                Color.blue(color) / 255.0f,
                Color.alpha(color) / 255.0f);
    }

    public void drawColor(float red, float green, float blue, float alpha) throws Throwable {
        runOnGLThread(() -> {
            glClearColor(red, green, blue, alpha);
            glClear(GL_COLOR_BUFFER_BIT);
            if (!mEgl.eglSwapBuffers(mEglDisplay, mEglSurface)) {
                throw new RuntimeException("Cannot swap buffers");
            }
        });
    }

    interface DrawFrame {
        void drawFrame(int width, int height);
    }

    public void drawFrame(Matrix transform, DrawFrame callback) throws Throwable {
        CountDownLatch fence = new CountDownLatch(1);
        runOnUiThread(() -> {
            mTextureView.setTransform(transform);
            fence.countDown();
        });
        waitForEnterAnimationComplete();
        waitForSurface();
        initGl();
        fence.await();
        int surfaceUpdateCount = mSurfaceUpdatedCount;
        runOnGLThread(() -> {
            callback.drawFrame(mSurfaceWidth, mSurfaceHeight);
            if (!mEgl.eglSwapBuffers(mEglDisplay, mEglSurface)) {
                throw new RuntimeException("Cannot swap buffers");
            }
        });
        waitForSurfaceUpdateCount(surfaceUpdateCount + 1);
    }

    private static final Matrix IDENTITY = new Matrix();

    public void drawFrame(DrawFrame callback) throws Throwable {
        drawFrame(IDENTITY, callback);
    }

    public int waitForSurfaceUpdateCount(int updateCount) throws InterruptedException {
        synchronized (mLock) {
            while (updateCount > mSurfaceUpdatedCount) {
                mLock.wait(TIME_OUT_MS);
            }
            return mSurfaceUpdatedCount;
        }
    }

    public void removeCover() {
        mPreview.setVisibility(View.GONE);
    }

    private void doFinishGL() {
        if (mEglSurface != null) {
            mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
            mEglSurface = null;
        }
        if (mEglContext != null) {
            mEgl.eglDestroyContext(mEglDisplay, mEglContext);
            mEglContext = null;
        }
        if (mEglDisplay != null) {
            mEgl.eglTerminate(mEglDisplay);
        }
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        synchronized (mLock) {
            mSurface = surface;
            mSurfaceWidth = width;
            mSurfaceHeight = height;
            mLock.notifyAll();
        }
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        synchronized (mLock) {
            mSurface = null;
            mLock.notifyAll();
        }
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
        synchronized (mLock) {
            mSurfaceUpdatedCount++;
            mLock.notifyAll();
        }
    }

    private Runnable mDoInitGL = new Runnable() {
        @Override
        public void run() {
            mEgl = (EGL10) EGLContext.getEGL();

            mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
            if (mEglDisplay == EGL10.EGL_NO_DISPLAY) {
                throw new RuntimeException("eglGetDisplay failed "
                        + GLUtils.getEGLErrorString(mEgl.eglGetError()));
            }

            int[] version = new int[2];
            if (!mEgl.eglInitialize(mEglDisplay, version)) {
                throw new RuntimeException("eglInitialize failed " +
                        GLUtils.getEGLErrorString(mEgl.eglGetError()));
            }

            // check extensions but still attempt to run the test, if the test fails then we check
            // mEGLExtensionUnsupported to determine if the failure was expected.
            String extensions = mEgl.eglQueryString(mEglDisplay, EGL10.EGL_EXTENSIONS);
            if (mEglColorSpace != 0) {
                String eglColorSpaceString = null;
                switch (mEglColorSpace) {
                    case TextureViewTest.EGL_GL_COLORSPACE_DISPLAY_P3_EXT:
                        eglColorSpaceString = "EGL_EXT_gl_colorspace_display_p3";
                        break;
                    case TextureViewTest.EGL_GL_COLORSPACE_SRGB_KHR:
                        eglColorSpaceString = "EGL_KHR_gl_colorspace";
                        break;
                    case TextureViewTest.EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT:
                        eglColorSpaceString = "EGL_EXT_gl_colorspace_scrgb_linear";
                        break;
                    default:
                        throw new RuntimeException("Unknown eglColorSpace: " + mEglColorSpace);
                }
                if (!extensions.contains(eglColorSpaceString)) {
                    mEGLExtensionUnsupported = true;
                }
            }
            if (mIsEGLWideGamut && !extensions.contains("EXT_pixel_format_float")) {
                mEGLExtensionUnsupported = true;
            }

            mEglConfig = chooseEglConfig();
            if (mEglConfig == null) {
                throw new RuntimeException("eglConfig not initialized");
            }

            mEglContext = createContext(mEgl, mEglDisplay, mEglConfig);

            mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, mEglConfig,
                    mSurface, (mEglColorSpace == 0) ? null :
                            new int[] { EGL_GL_COLORSPACE_KHR, mEglColorSpace, EGL10.EGL_NONE });

            if (mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE) {
                int error = mEgl.eglGetError();
                throw new RuntimeException("createWindowSurface failed "
                        + GLUtils.getEGLErrorString(error));
            }

            if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
                throw new RuntimeException("eglMakeCurrent failed "
                        + GLUtils.getEGLErrorString(mEgl.eglGetError()));
            }
        }
    };

    EGLContext createContext(EGL10 egl, EGLDisplay eglDisplay, EGLConfig eglConfig) {
        int[] attrib_list = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
        return egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
    }

    private EGLConfig chooseEglConfig() {
        int[] configsCount = new int[1];
        EGLConfig[] configs = new EGLConfig[1];
        int[] configSpec = getConfig();
        if (!mEgl.eglChooseConfig(mEglDisplay, configSpec, configs, 1, configsCount)) {
            throw new IllegalArgumentException("eglChooseConfig failed " +
                    GLUtils.getEGLErrorString(mEgl.eglGetError()));
        } else if (configsCount[0] > 0) {
            return configs[0];
        }
        return null;
    }

    private int[] getConfig() {
        if (mIsEGLWideGamut) {
            return new int[]{
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL_COLOR_COMPONENT_TYPE_EXT, EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT,
                    EGL10.EGL_RED_SIZE, 16,
                    EGL10.EGL_GREEN_SIZE, 16,
                    EGL10.EGL_BLUE_SIZE, 16,
                    EGL10.EGL_ALPHA_SIZE, 16,
                    EGL10.EGL_DEPTH_SIZE, 0,
                    EGL10.EGL_STENCIL_SIZE, 0,
                    EGL10.EGL_NONE
            };
        } else {
            return new int[]{
                    EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                    EGL10.EGL_RED_SIZE, 8,
                    EGL10.EGL_GREEN_SIZE, 8,
                    EGL10.EGL_BLUE_SIZE, 8,
                    EGL10.EGL_ALPHA_SIZE, 8,
                    EGL10.EGL_DEPTH_SIZE, 0,
                    EGL10.EGL_STENCIL_SIZE, 0,
                    EGL10.EGL_NONE
            };
        }
    }
}
