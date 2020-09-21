/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.car.cluster.sample;

import android.annotation.NonNull;
import android.content.Context;
import android.hardware.display.DisplayManager;
import android.hardware.display.DisplayManager.DisplayListener;
import android.hardware.display.VirtualDisplay;
import android.media.MediaCodec;
import android.media.MediaCodec.BufferInfo;
import android.media.MediaCodec.CodecException;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecProfileLevel;
import android.media.MediaFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.Surface;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.util.UUID;

/**
 * This class encapsulates all work related to managing networked virtual display.
 * <p>
 * It opens server socket and listens on port {@code PORT} for incoming connections. Once connection
 * is established it creates virtual display and media encoder and starts streaming video to that
 * socket.  If the receiving part is disconnected, it will keep port open and virtual display won't
 * be destroyed.
 */
public class NetworkedVirtualDisplay {
    private static final String TAG = "Cluster." + NetworkedVirtualDisplay.class.getSimpleName();

    private final String mUniqueId =  UUID.randomUUID().toString();

    private final DisplayManager mDisplayManager;
    private final int mWidth;
    private final int mHeight;
    private final int mDpi;

    private static final int PORT = 5151;
    private static final int FPS = 25;
    private static final int BITRATE = 6144000;
    private static final String MEDIA_FORMAT_MIMETYPE = MediaFormat.MIMETYPE_VIDEO_AVC;

    private static final int MSG_START = 0;
    private static final int MSG_STOP = 1;
    private static final int MSG_RESUBMIT_FRAME = 2;

    private VirtualDisplay mVirtualDisplay;
    private MediaCodec mVideoEncoder;
    private HandlerThread mThread = new HandlerThread("NetworkThread");
    private Handler mHandler;
    private ServerSocket mServerSocket;
    private OutputStream mOutputStream;
    private byte[] mBuffer = null;
    private int mLastFrameLength = 0;

    private final DebugCounter mCounter = new DebugCounter();

    NetworkedVirtualDisplay(Context context, int width, int height, int dpi) {
        mDisplayManager = context.getSystemService(DisplayManager.class);
        mWidth = width;
        mHeight = height;
        mDpi = dpi;

        DisplayListener displayListener = new DisplayListener() {
            @Override
            public void onDisplayAdded(int i) {
                final Display display = mDisplayManager.getDisplay(i);
                if (display != null && getDisplayName().equals(display.getName())) {
                    onVirtualDisplayReady(display);
                }
            }

            @Override
            public void onDisplayRemoved(int i) {}

            @Override
            public void onDisplayChanged(int i) {}
        };

        mDisplayManager.registerDisplayListener(displayListener, new Handler());
    }

    /**
     * Opens socket and creates virtual display asynchronously once connection established.  Clients
     * of this class may subscribe to
     * {@link android.hardware.display.DisplayManager#registerDisplayListener(
     * DisplayListener, Handler)} to be notified when virtual display is created.
     * Note, that this method should be called only once.
     *
     * @return Unique display name associated with the instance of this class.
     *
     * @see {@link Display#getName()}
     *
     * @throws IllegalStateException thrown if networked display already started
     */
    public String start() {
        if (mThread.isAlive()) {
            throw new IllegalStateException("Already started");
        }
        mThread.start();
        mHandler = new NetworkThreadHandler(mThread.getLooper());
        mHandler.sendMessage(Message.obtain(mHandler, MSG_START));

        return getDisplayName();
    }

    public void release() {
        stopCasting();

        if (mVirtualDisplay != null) {
            mVirtualDisplay.release();
            mVirtualDisplay = null;
        }
        mThread.quit();
    }

    private String getDisplayName() {
        return "Cluster-" + mUniqueId;
    }


    private VirtualDisplay createVirtualDisplay() {
        Log.i(TAG, "createVirtualDisplay " + mWidth + "x" + mHeight +"@" + mDpi);
        return mDisplayManager.createVirtualDisplay(getDisplayName(), mWidth, mHeight, mDpi,
                null, 0 /* flags */, null, null );
    }

    private void onVirtualDisplayReady(Display display) {
        Log.i(TAG, "onVirtualDisplayReady, display: " + display);
    }

    private void startCasting(Handler handler) {
        Log.i(TAG, "Start casting...");
        mVideoEncoder = createVideoStream(handler);

        if (mVirtualDisplay == null) {
            mVirtualDisplay = createVirtualDisplay();
        }
        mVirtualDisplay.setSurface(mVideoEncoder.createInputSurface());
        mVideoEncoder.start();

        Log.i(TAG, "Video encoder started");
    }

    private MediaCodec createVideoStream(Handler handler) {
        MediaCodec encoder;
        try {
            encoder = MediaCodec.createEncoderByType(MEDIA_FORMAT_MIMETYPE);
        } catch (IOException e) {
            Log.e(TAG, "Failed to create video encoder for " + MEDIA_FORMAT_MIMETYPE, e);
            return null;
        }

        encoder.setCallback(new MediaCodec.Callback() {
            @Override
            public void onInputBufferAvailable(@NonNull MediaCodec codec, int index) {
                Log.i(TAG, "onInputBufferAvailable, index: " + index);
            }

            @Override
            public void onOutputBufferAvailable(@NonNull MediaCodec codec, int index,
                    @NonNull BufferInfo info) {
                Log.i(TAG, "onOutputBufferAvailable, index: " + index);
                mCounter.outputBuffers++;
                doOutputBufferAvailable(index, info);
            }

            @Override
            public void onError(@NonNull MediaCodec codec, @NonNull CodecException e) {
                Log.e(TAG, "onError, codec: " + codec, e);
                mCounter.bufferErrors++;
            }

            @Override
            public void onOutputFormatChanged(@NonNull MediaCodec codec,
                    @NonNull MediaFormat format) {
                Log.i(TAG, "onOutputFormatChanged, codec: " + codec + ", format: " + format);

            }
        }, handler);

        configureVideoEncoder(encoder, mWidth, mHeight);
        return encoder;
    }

    private void doOutputBufferAvailable(int index, @NonNull BufferInfo info) {
        mHandler.removeMessages(MSG_RESUBMIT_FRAME);

        ByteBuffer encodedData = mVideoEncoder.getOutputBuffer(index);
        if (encodedData == null) {
            throw new RuntimeException("couldn't fetch buffer at index " + index);
        }

        if (info.size != 0) {
            encodedData.position(info.offset);
            encodedData.limit(info.offset + info.size);
            mLastFrameLength = encodedData.remaining();
            if (mBuffer == null || mBuffer.length < mLastFrameLength) {
                Log.i(TAG, "Allocating new buffer: " + mLastFrameLength);
                mBuffer = new byte[mLastFrameLength];
            }
            encodedData.get(mBuffer, 0, mLastFrameLength);
            mVideoEncoder.releaseOutputBuffer(index, false);

            sendFrame(mBuffer, mLastFrameLength);

            // If nothing happens in Virtual Display we won't receive new frames. If we won't keep
            // sending frames it could be a problem for the receiver because it needs certain
            // number of frames in order to start decoding.
            scheduleResendingLastFrame(1000 / FPS);
        } else {
            Log.e(TAG, "Skipping empty buffer");
            mVideoEncoder.releaseOutputBuffer(index, false);
        }
    }

    private void scheduleResendingLastFrame(long delayMs) {
        Message msg = mHandler.obtainMessage(MSG_RESUBMIT_FRAME);
        mHandler.sendMessageDelayed(msg, delayMs);
    }

    private void sendFrame(byte[] buf, int len) {
        try {
            mOutputStream.write(buf, 0, len);
            Log.i(TAG, "Bytes written: " + len);
        } catch (IOException e) {
            mCounter.clientsDisconnected++;
            mOutputStream = null;
            Log.e(TAG, "Failed to write data to socket, restart casting", e);
            restart();
        }
    }

    private void stopCasting() {
        Log.i(TAG, "Stopping casting...");
        if (mServerSocket != null) {
            try {
                mServerSocket.close();
            } catch (IOException e) {
                Log.w(TAG, "Failed to close server socket, ignoring", e);
            }
            mServerSocket = null;
        }

        if (mVirtualDisplay != null) {
            // We do not want to destroy virtual display (as it will also destroy all the
            // activities on that display, instead we will turn off the display by setting
            // a null surface.
            Surface surface = mVirtualDisplay.getSurface();
            if (surface != null) surface.release();
            mVirtualDisplay.setSurface(null);
        }

        if (mVideoEncoder != null) {
            // Releasing encoder as stop/start didn't work well (couldn't create or reuse input
            // surface).
            mVideoEncoder.stop();
            mVideoEncoder.release();
            mVideoEncoder = null;
        }
        Log.i(TAG, "Casting stopped");
    }

    private synchronized void restart() {
        // This method could be called from different threads when receiver has disconnected.
        if (mHandler.hasMessages(MSG_START)) return;

        mHandler.sendMessage(Message.obtain(mHandler, MSG_STOP));
        mHandler.sendMessage(Message.obtain(mHandler, MSG_START));
    }

    private class NetworkThreadHandler extends Handler {

        NetworkThreadHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_START:
                    if (mServerSocket == null) {
                        mServerSocket = openServerSocket();
                    }
                    Log.i(TAG, "Server socket opened");

                    mOutputStream = waitForReceiver(mServerSocket);
                    if (mOutputStream == null) {
                        sendMessage(Message.obtain(this, MSG_START));
                        break;
                    }
                    mCounter.clientsConnected++;

                    startCasting(this);
                    break;

                case MSG_STOP:
                    stopCasting();
                    break;

                case MSG_RESUBMIT_FRAME:
                    if (mServerSocket != null && mOutputStream != null) {
                        Log.i(TAG, "Resending the last frame again. Buffer: " + mLastFrameLength);
                        sendFrame(mBuffer, mLastFrameLength);
                    }
                    // We will keep sending last frame every second as a heartbeat.
                    scheduleResendingLastFrame(1000L);
                    break;
            }
        }
    }

    private static void configureVideoEncoder(MediaCodec codec, int width, int height) {
        MediaFormat format = MediaFormat.createVideoFormat(MEDIA_FORMAT_MIMETYPE, width, height);

        format.setInteger(MediaFormat.KEY_COLOR_FORMAT,
                MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);
        format.setInteger(MediaFormat.KEY_BIT_RATE, BITRATE);
        format.setInteger(MediaFormat.KEY_FRAME_RATE, FPS);
        format.setInteger(MediaFormat.KEY_CAPTURE_RATE, FPS);
        format.setInteger(MediaFormat.KEY_CHANNEL_COUNT, 1);
        format.setFloat(MediaFormat.KEY_I_FRAME_INTERVAL, 1); // 1 second between I-frames
        format.setInteger(MediaFormat.KEY_LEVEL, CodecProfileLevel.AVCLevel31);
        format.setInteger(MediaFormat.KEY_PROFILE,
                MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline);

        codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
    }

    private OutputStream waitForReceiver(ServerSocket serverSocket) {
        try {
            Log.i(TAG, "Listening for incoming connections on port: " + PORT);
            Socket socket = serverSocket.accept();

            Log.i(TAG, "Receiver connected: " + socket);
            listenReceiverDisconnected(socket.getInputStream());

            return socket.getOutputStream();
        } catch (IOException e) {
            Log.e(TAG, "Failed to accept connection");
            return null;
        }
    }

    private void listenReceiverDisconnected(InputStream inputStream) {
        new Thread(() -> {
            try {
                if (inputStream.read() == -1) throw new IOException();
            } catch (IOException e) {
                Log.w(TAG, "Receiver has disconnected", e);
            }
            restart();
        }).start();
    }

    private static ServerSocket openServerSocket() {
        try {
            return new ServerSocket(PORT);
        } catch (IOException e) {
            Log.e(TAG, "Failed to create server socket", e);
            throw new RuntimeException(e);
        }
    }

    @Override
    public String toString() {
        return getClass() + "{"
                + mServerSocket
                +", receiver connected: " + (mOutputStream != null)
                +", encoder: " + mVideoEncoder
                +", virtualDisplay" + mVirtualDisplay
                + "}";
    }

    private static class DebugCounter {
        long outputBuffers;
        long bufferErrors;
        long clientsConnected;
        long clientsDisconnected;

        @Override
        public String toString() {
            return getClass().getSimpleName() + "{"
                    + "outputBuffers=" + outputBuffers
                    + ", bufferErrors=" + bufferErrors
                    + ", clientsConnected=" + clientsConnected
                    + ", clientsDisconnected= " + clientsDisconnected
                    + "}";
        }
    }
}
