package android.media;

import android.annotation.Nullable;
import android.content.Context;
import android.media.PlaybackParams;
import android.media.MediaPlayer.TrackInfo;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemProperties;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.util.Log;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.net.HttpCookie;
import java.util.List;
import java.util.Map;
import android.os.Parcel;

public class MediaPlayerExo {
    private final static String TAG = "MediaPlayerExo";
    private boolean DEBUG = false;

    private Looper mExoLooper;
    private Handler mEventHandler;
    private SurfaceHolder mSurfaceHolder;

    Class<?> exoplayerWrapper = null;
    static Class<?> staticExoplayerWrapper = null;
    Method initVideo = null;
    Method prepare = null;
    Method prepareAsync = null;
    Method start = null;
    Method pause = null;
    Method stop = null;
    Method release = null;
    Method reset = null;
    Method isPlaying = null;
    Method seekTo = null;
    Method getDuration = null;
    Method getCurrentPosition = null;
    Method getVideoHeight = null;
    Method getVideoWidth = null;
    Method setVolume = null;
    Method setVideoSurfaceHolder = null;
    Method setAudioStreamType = null;
    Method setSurface = null;
    Method setVideoScalingMode = null;
    Method setPlaybackParams = null;
    Method setStateListener = null;
    Method setParameter = null;
    Method setParameterString = null;
    Method setParameterParcel = null;
    Method getParcelParameter = null;
    Method getIntParameter = null;
    Method setLooping = null;
    Method getLooping = null;
    Method getTrackInfo = null;
    Method getBufferedPosition = null;
    static Method getIsFormatSupport = null;
    Object player = null;
    Class<?> stateListener = null;
    private MediaPlayer mMediaPlayer;

    public MediaPlayerExo(MediaPlayer mp, Looper looper, Handler handler) {
        mMediaPlayer = mp;
        mExoLooper = looper;
        mEventHandler = handler;
        try {
            exoplayerWrapper = Class.forName("com.google.android.exoplayer2aml.ExoPlayerWrapper");
            if (DEBUG) LOGI(TAG,"[MediaPlayerExo]exoplayerWrapper:" + exoplayerWrapper);
            Class[] paramTypes = { MediaPlayer.class, Looper.class, Handler.class };
            Object[] params = { mMediaPlayer, mExoLooper,  mEventHandler};
            Constructor<?> constructor = exoplayerWrapper.getConstructor(paramTypes);
            player = constructor.newInstance(params);
            initVideo = exoplayerWrapper.getMethod("initVideo",Context.class, Uri.class, Map.class, List.class);
            prepare = exoplayerWrapper.getMethod("prepare");
            prepareAsync = exoplayerWrapper.getMethod("prepareAsync");
            start = exoplayerWrapper.getMethod("start");
            pause = exoplayerWrapper.getMethod("pause");
            stop = exoplayerWrapper.getMethod("stop");
            release = exoplayerWrapper.getMethod("release");
            isPlaying = exoplayerWrapper.getMethod("isPlaying");
            reset = exoplayerWrapper.getMethod("reset");
            seekTo = exoplayerWrapper.getMethod("seekTo", int.class, long.class);
            getDuration = exoplayerWrapper.getMethod("getDuration");
            getCurrentPosition = exoplayerWrapper.getMethod("getCurrentPosition");
            getVideoHeight = exoplayerWrapper.getMethod("getVideoHeight");
            getVideoWidth = exoplayerWrapper.getMethod("getVideoWidth");
            setVolume = exoplayerWrapper.getMethod("setVolume", float.class, float.class);
            setVideoSurfaceHolder = exoplayerWrapper.getMethod("setVideoSurfaceHolder",SurfaceHolder.class);
            setAudioStreamType = exoplayerWrapper.getMethod("setAudioStreamType",int.class);
            setSurface = exoplayerWrapper.getMethod("setSurface",Surface.class);
            setVideoScalingMode = exoplayerWrapper.getMethod("setVideoScalingMode",int.class);
            setPlaybackParams = exoplayerWrapper.getMethod("setPlaybackParams",PlaybackParams.class);
            getIsFormatSupport = exoplayerWrapper.getMethod("getIsFormatSupport",Uri.class);
            setParameter = exoplayerWrapper.getMethod("setParameter", int.class, int.class);
            setParameterString = exoplayerWrapper.getMethod("setParameterString", int.class, String.class);
            setParameterParcel = exoplayerWrapper.getMethod("setParameterParcel", int.class, Parcel.class);
            getParcelParameter = exoplayerWrapper.getMethod("getParcelParameter", int.class);
            getIntParameter = exoplayerWrapper.getMethod("getIntParameter", int.class);
            setLooping = exoplayerWrapper.getMethod("setLooping", boolean.class);
            getLooping = exoplayerWrapper.getMethod("getLooping");
            getTrackInfo = exoplayerWrapper.getMethod("getTrackInfo");
            getBufferedPosition = exoplayerWrapper.getMethod("getBufferedPosition");
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    private void LOGI (String tag, String msg) {
        if (DEBUG || getDebugEnable()) { Log.i (tag, msg); }
    }

    private void LOGD (String tag, String msg) {
        if (DEBUG || getDebugEnable()) { Log.d (tag, msg); }
    }

    private void LOGE (String tag, String msg) {
        Log.e (tag, msg);
    }

    public static boolean getIsFormatSupport(Uri uri) {
        boolean ret = false;
        try {
            if (staticExoplayerWrapper == null)
                staticExoplayerWrapper = Class.forName("com.google.android.exoplayer2aml.ExoPlayerWrapper");
            getIsFormatSupport = staticExoplayerWrapper.getMethod("getIsFormatSupport",Uri.class);
            ret = (boolean)getIsFormatSupport.invoke(staticExoplayerWrapper, uri);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
        return ret;
    }
    

    private boolean getDebugEnable() {
        boolean ret = SystemProperties.getBoolean("sys.exoplayer.debug", false);
        return ret;
    }

    public void initVideo(Context context, Uri uri, @Nullable Map<String, String> headers, List<HttpCookie> cookies) {
        if (initVideo != null) {
            LOGI(TAG, "initVideo");
            try {
                initVideo.invoke(player, context, uri, headers, cookies);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void prepare() {
        if (prepare != null) {
            LOGI(TAG, "prepare");
            try {
                prepare.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void prepareAsync() {
        if (prepareAsync != null) {
            LOGI(TAG, "prepareAsync");
            try {
                prepareAsync.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void start() {
        if (start != null) {
            try {
                start.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void pause() {
        if (pause != null) {
            try {
                pause.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void stop() {
        if (stop != null) {
            try {
                stop.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void release() {
        if (release != null) {
            try {
                release.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void reset() {
        if (reset != null) {
            try {
                reset.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public boolean isPlaying() {
        boolean ret = false;
        if (isPlaying != null) {
            try {
                ret = (boolean) isPlaying.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return ret;
    }

    public void seekTo(int mode, long msec) {
        if (seekTo != null) {
            try {
                seekTo.invoke(player, mode, msec);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public int  getDuration() {
        int duration = 0;
        if (getDuration != null) {
            try {
                duration = (int) getDuration.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return duration;
    }

    public int getCurrentPosition() {
        int curreentPosition = 0;
        if (getCurrentPosition != null) {
            try {
                curreentPosition = (int) getCurrentPosition.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return curreentPosition;
    }

    public int getVideoHeight() {
        int videoHeight = 0;
        if (getVideoHeight != null) {
            try {
                videoHeight = (int) getVideoHeight.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return videoHeight;
    }

    public int getVideoWidth() {
        int videoWidth = 0;
        if (getVideoWidth != null) {
            try {
                videoWidth = (int) getVideoWidth.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return videoWidth;
    }

    public void setVolume(float leftvolume, float rightvolume) {
        if (setVolume != null) {
            try {
                setVolume.invoke(player, leftvolume, rightvolume);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void setVideoSurfaceHolder(SurfaceHolder surfaceHolder) {
        if (setVideoSurfaceHolder != null) {
            try {
                setVideoSurfaceHolder.invoke(player, surfaceHolder);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void setDisplay(SurfaceHolder surfaceHolder) {
        mSurfaceHolder = surfaceHolder;
        if (setVideoSurfaceHolder != null) {
            try {
                mSurfaceHolder = surfaceHolder;
                setVideoSurfaceHolder.invoke(player, surfaceHolder);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void setAudioStreamType(int type) {
        if (setAudioStreamType != null) {
            try {
                setAudioStreamType.invoke(player, type);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void setSurface(Surface surface) {
        if (setSurface != null) {
            try {
                setSurface.invoke(player, surface);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void setVideoScalingMode(int  mode) {
        if (setVideoScalingMode != null) {
            try {
                setVideoScalingMode.invoke(player, mode);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public void setPlaybackParams(PlaybackParams params) {
        if (setPlaybackParams != null) {
            try {
                setPlaybackParams.invoke(player, params);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public boolean setParameter(int key, int value) {
        boolean ret = false;
        if (setParameter != null) {
            LOGI(TAG, "setParameter");
            try {
                ret = (boolean)setParameter.invoke(player, key, value);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return ret;
    }

    public boolean setParameter(int key, String value) {
        boolean ret = false;
        if (setParameterString != null) {
            LOGI(TAG, "setParameterString");
            try {
                ret = (boolean)setParameterString.invoke(player, key, value);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return ret;
    }

    public boolean setParameter(int key, Parcel parcel) {
        boolean ret = false;
        if (setParameterParcel != null) {
            LOGI(TAG, "setParameterParcel");
            try {
                ret = (boolean)setParameterParcel.invoke(player, key, parcel);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return ret;
    }

    public Parcel getParcelParameter(int key) {
        Parcel parcel = null;
        if (getParcelParameter != null) {
            LOGI(TAG, "getParcelParameter");
            try {
                parcel = (Parcel)getParcelParameter.invoke(player, key);
            } catch (Exception ex) {
                LOGE(TAG, "getParcelParameter failed:" + ex);
                ex.printStackTrace();
            }
        }
        return parcel;
    }

    public int getIntParameter(int key) {
        int ret = 0;
        if (getIntParameter != null) {
            LOGI(TAG, "getIntParameter");
            try {
                ret = (int)getIntParameter.invoke(player, key);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return ret;
    }

    public void setLooping(boolean isLoop) {
        if (setLooping != null) {
            LOGI(TAG, "setLooping");
            try {
                setLooping.invoke(player, isLoop);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
    }

    public boolean getLooping() {
        boolean ret = false;
        if (getLooping != null) {
            LOGI(TAG, "getLooping");
            try {
                ret = (boolean)getLooping.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return ret;
    }

    public TrackInfo[] getTrackInfo() {
        if (getTrackInfo != null) {
            LOGI(TAG, "getTrackInfo");
            try {
                return (TrackInfo[])getTrackInfo.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return null;
    }

    public int getBufferedPosition() {
        int ret = 0;
        if (getBufferedPosition != null) {
            LOGI(TAG, "getBufferedPosition");
            try {
                ret = (int)getBufferedPosition.invoke(player);
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return ret;
    }
}
