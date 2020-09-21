package com.ctc;

import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.PowerManager;
//import android.os.SystemProperties;
import android.widget.Button;
import android.widget.TextView; 
import android.widget.EditText;
import android.view.KeyEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Surface;
import android.view.View;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.Log; 

import android.os.Handler;
import android.os.Message;
import android.view.Gravity;
import android.graphics.Typeface;

import java.lang.reflect.Method;
import java.lang.reflect.Field;
import java.lang.reflect.Constructor;

public class MediaProcessorDemoActivity extends Activity {
	private static String TAG="MediaProcessorDemoActivity";
	public String result_s = "success";
	public int result_i = 0;
	public boolean result_b = false;
	private Surface mySurface = null;
	private SurfaceHolder myHolder = null;
	private SurfaceView mySurfaceView = null;
	private PropertieList propList = new PropertieList();
	String url = getUrl(); 
	String url1 = getUrl1();
    public static Object getProperties(String key, String def) {
            String defVal = def;
            try {
                Class properClass = Class.forName("android.os.SystemProperties");
                Method getMethod = properClass.getMethod("get",String.class,String.class);
                defVal = (String)getMethod.invoke(null,key,def);
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                Log.d(TAG,"getProperties(" + key + "," + def + ") ret= "+defVal);
                return defVal;
            }

    }
    private static String getUrl() {
        return (String)getProperties("iptv.demo.url",
            "/sdcard1/iptv_test.ts");
    }
	private static String getUrl1() {
		return null;/*SystemProperties.get("iptv.demo.url1",
			"/storage/external_storage/sdcard1/iptv_test.ts");*/
	}
	private int playBufferSize = 32;
	private Button playButton = null;
	private Button pause = null;
	private Button resume = null; 
	private Button seek = null;
	private Button videoshow = null;
	private Button videohide = null;
	private Button fast = null;
	private Button stopfast = null;
	private Button stop = null;
	private Button getVolume = null;
	private Button setVolume = null;
	private Button setRatio = null;
	private Button getAudioBalance = null;
	private Button setAudioBalance = null;
	private Button getVideoPixels = null;
	private Button isSoftFit = null;
	private Button getPlayMode = null;
	private Button setEPGSize = null;
	private Button switchAudioTrack = null;
	private Button switchSubtitle = null;
	private TextView Function = null;
	private TextView Return_t = null;
	private TextView Result = null;
	private TextView resultView = null;
    private EditText urlEditText = null;
	
	private int flag = 0;
	private int PLAYER_INIT = 0;
	private int PLAYER_PALY = 1;
	private int PLAYER_STOP = 2;
	private int player_status = 0;
	private Handler MainHandler = null;


	
	private int getBufferSize() {
		return propList.getPlayBufferSize(32);
	}
	
	class drawSurface implements Runnable{
		public String url;
		public void run() {
			Log.i(TAG, "nativeWriteData1 start");
			nativeWriteData(this.url, playBufferSize, 0);
			Log.i(TAG, "nativeWriteData1 end");
		} 
	}

	class drawSurface1 implements Runnable{
		public String url1;
		public void run()
		{
			nativeWriteData(this.url1, playBufferSize, 1);
		}
	}
	
	public boolean onKeyDown(int keyCode, KeyEvent msg) {
		Log.d(TAG, "onKeyDown()"); 
		if(keyCode == KeyEvent.KEYCODE_POWER){
			return true;
		}
		return super.onKeyDown(keyCode, msg);
	}

	@Override
	public void onPause(){
		super.onPause(); 
		Log.i(TAG, "onPause");

		player_status = PLAYER_STOP;
		Log.d(TAG, "before nativeStop, time is: " + System.currentTimeMillis());

		nativeStop(0);  
		Log.d(TAG, "before nativeSetEPGSize, time is: " + System.currentTimeMillis());

        nativeSetEPGSize(1280, 720, 0);
		Log.d(TAG, "after nativeSetEPGSize, time is: " + System.currentTimeMillis());

		Date a = new Date();
		SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
		Log.i(TAG, "release start" + sdf.format(a));  
		if(flag != 1) {
			nativeDelete();
			flag = 0;
		}

		Date b = new Date();
		Log.i(TAG, "release end" + sdf.format(b));
	}

	@Override
	public void onStop(){
		super.onStop();
		Log.i(TAG, "onStop");
	}

	private IntentFilter mFilter = null;

	@Override
	public void onResume() {
		super.onResume();
		Log.d(getClass().getName(), "onResume()");
	}

    private void startPlay(String playUrl) {
        if (nativeInit(playUrl, 0) == 0)
            Log.i(TAG, "Init success!");
        else {
            Log.i(TAG, "Init: error!");
            return;
        }

        Log.i("create surface:", "next ");
        if (nativeCreateSurface(mySurface, 1280, 720, 0) == 0)
            Log.i("create surface:", "success");

        //nativeSetEPGSize(1280, 720, 0);
        //nativeSetVideoWindow(420, 100, 640, 480, 0);

        nativeStartPlay(0);
        Log.i("play", "success");

        drawSurface playData = new drawSurface();
        playData.url = playUrl;
        Thread player = new Thread(playData);
        player.start();

        MainHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                Log.d(TAG, "get msg "+msg.what);
                super.handleMessage(msg);
            }
        };

        player_status = PLAYER_PALY;
    }

	/** Called when the activity is first created. */ 
	@Override 
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main); 

		playBufferSize = getBufferSize();

		mFilter = new IntentFilter();
		mFilter.addAction(Intent.ACTION_SCREEN_OFF);
		mFilter.addAction(Intent.ACTION_SCREEN_ON);
		mFilter.addAction("com.android.smart.terminal.iptv.power");
		mFilter.setPriority(IntentFilter.SYSTEM_HIGH_PRIORITY);

		Log.i(TAG, "onCreate");
		mySurfaceView = (SurfaceView)this.findViewById(R.id.SurfaceView01);
		myHolder = mySurfaceView.getHolder();
        //myHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        mySurfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                mySurface = holder.getSurface();
                Log.d(TAG, "surfaceCreated: " + mySurface);
            }

			@Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

            }

			@Override
            public void surfaceDestroyed(SurfaceHolder holder) {

            }
        });

		Function = (TextView)findViewById(R.id.Function);
		Return_t = (TextView)findViewById(R.id.Return_t);
		Result = (TextView)findViewById(R.id.Result);
        urlEditText = (EditText)findViewById(R.id.urlText);

        //play
		playButton = (Button)findViewById(R.id.play);
		playButton.setOnClickListener(new Button.OnClickListener(){
			public void onClick(View v) {
                String playUrl = url;
                String urlText = urlEditText.getText().toString();
                Log.i(TAG, "urlText: " + urlText);
                if (urlText != null) {
                    playUrl = urlText;
                }
                startPlay(playUrl);
            }
        });

		//pause
		pause = (Button)findViewById(R.id.pause);
		pause.setOnClickListener(new Button.OnClickListener(){
			public void onClick(View v) {
				result_b = nativePause(0);
				Function.setText("Pause");
				if (result_b == true) {
					Return_t.setText("true");
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:Pause", "return:true", "result:success");
				} else {
					Return_t.setText("false"); 
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:Pause", "return:false", "result:error");
				}
			}
		});

		//resume
		resume = (Button)findViewById(R.id.resume);
		resume.setOnClickListener(new Button.OnClickListener(){
			public void onClick(View v) {
				result_b = nativeResume(0);
				Function.setText("Resume");
				if (result_b == true) {
					Return_t.setText("true");
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:Resume", "return:true", "result:success");
				} else {
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:Resume", "return:false", "result:error");
				}
			}
		});

		//seek
		seek = (Button)findViewById(R.id.seek);
		seek.setOnClickListener(new Button.OnClickListener(){ 
			public void onClick(View v) {
				result_b = nativeSeek(0);
				Function.setText("Seek");  
				if (result_b == true) {
					Return_t.setText("true");
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:Seek", "return:true", "result:success"); 
				} else {
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:Seek", "return:false", "result:error"); 
				}
			}
		});

		//fast
		fast = (Button)findViewById(R.id.fast);
		fast.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				result_b = nativeFast(0); 
				Function.setText("Fast");
				if (result_b == true) {
					Return_t.setText("true");
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:Fast", "return:true", "result:success"); 
				} else {
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:Fast", "return:false", "result:error");
				}
			}
		});

		//stopfast
		stopfast = (Button)findViewById(R.id.stopfast);
		stopfast.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				result_b = nativeStopFast(0);
				Function.setText("StopFast");
				if (result_b == true) {
					result_s = String.valueOf(result_b);
					Return_t.setText(result_s);
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:StopFast", "return:true", "result:success"); 
				} else {
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:StopFast", "return:false", "result:error"); 
				}
			}
		});

		//videoshow
		videoshow = (Button)findViewById(R.id.videoshow);
		videoshow.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				result_i = nativeVideoShow(0);
				Function.setText("VideoShow");
				if (result_i == 0) {
					result_s = String.valueOf(result_i);
					Return_t.setText(result_s);
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:VideoShow", "return:"+result_s, "result:success");
				} else {
					result_s = String.valueOf(result_i);
					Return_t.setText(result_s);
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:VideoShow", "return:"+result_s, "result:error");
				}
			} 
		});

		//videohide 
		videohide = (Button)findViewById(R.id.videohide);
		videohide.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				result_i = nativeVideoHide(0);
				Function.setText("VideoHide");
				if (result_i == 0) {
					result_s = String.valueOf(result_i);
					Return_t.setText(result_s);
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:VideoHide", "return:"+result_s, "result:success");
				} else {
					result_s = String.valueOf(result_i);
					Return_t.setText(result_s);
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:VideoHide", "return:"+result_s, "result:error");
				}
			}
		});

		//stop
		stop = (Button)findViewById(R.id.stop);
		stop.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				result_b = nativeStop(0);
				Function.setText("Stop");
				if (result_b == true) {
					result_s = String.valueOf(result_b);
					Return_t.setText(result_s);
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:Stop", "return:true", "result:success");
				} else {
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:Stop", "return:false", "result:error");
				}
			}
		}); 

		//getVolume
		getVolume = (Button)findViewById(R.id.getVolume);
		getVolume.setOnClickListener(new Button.OnClickListener() {
			public void onClick(View v) {
				result_i = nativeGetVolume(0);
				Function.setText("GetVolume");
				result_s = String.valueOf(result_i);
				Return_t.setText(result_s);
				Result.setTextColor(Color.BLUE);
				Result.setText("success");
				nativeWriteFile("function:GetVolume", "return:"+result_s, "result:success");
			}
		}); 

		//setVolume
		setVolume = (Button)findViewById(R.id.setVolume);
		setVolume.setOnClickListener(new Button.OnClickListener() { 
			public void onClick(View v) {
				result_b = nativeSetVolume(60,0);
				Function.setText("SetVolume");
				if (result_b == true) {
					result_s = String.valueOf(result_b);
					Return_t.setText(result_s);
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:SetVolume", "return:true", "result:success");
				} else {
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:SetVolume", "return:false", "result:error");
				}
			}
		});

		//setRatio
		setRatio = (Button)findViewById(R.id.setRatio);
		setRatio.setOnClickListener(new Button.OnClickListener() { 
			public void onClick(View v) {
				result_b = nativeSetRatio(1,0);
				Function.setText("SetRatio");
				if (result_b == true) {
					result_s = String.valueOf(result_b);
					Return_t.setText(result_s);
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:SetRatio", "return:true", "result:success");
				} else {
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:SetRatio", "return:false", "result:error");
				}
			}
		});

		//getAudioBalance
		getAudioBalance = (Button)findViewById(R.id.getAudioBalance);
		getAudioBalance.setOnClickListener(new Button.OnClickListener(){ 
			public void onClick(View v) {
				result_i = nativeGetAudioBalance(0);
				Function.setText("GetAudioBalance");
				result_s = String.valueOf(result_i);
				Return_t.setText(result_s);
				Result.setTextColor(Color.BLUE);
				Result.setText("success");
				nativeWriteFile("function:GetAudioBalance", "return:"+result_s, "result:success");
			}
		}); 

		//setAudioBalance
		setAudioBalance = (Button)findViewById(R.id.setAudioBalance);
		setAudioBalance.setOnClickListener(new Button.OnClickListener(){ 
			public void onClick(View v) {
				result_i = nativeGetAudioBalance(0);
				result_b = nativeSetAudioBalance(result_i>1?(result_i-1):3,0);
				Function.setText("SetAudioBalance");
				if (result_b == true) {
					result_s = String.valueOf(result_b);
					Return_t.setText(result_s);
					Result.setTextColor(Color.BLUE);
					Result.setText("success");
					nativeWriteFile("function:SetAudioBalance", "return:"+result_s, "result:success");
				} else { 
					Return_t.setText("false");
					Result.setTextColor(Color.RED);
					Result.setText("error");
					nativeWriteFile("function:GetAudioBalance", "return:false", "result:error");
				}
			}
		}); 

		//getVideoPixels
		getVideoPixels = (Button)findViewById(R.id.getVideoPixels);
		getVideoPixels.setOnClickListener(new Button.OnClickListener(){ 
			public void onClick(View v) {
				nativeGetVideoPixels(0);
				Function.setText("GetVideoPixels");
				Return_t.setText("void");
				Result.setTextColor(Color.BLUE);
				Result.setText("success");
				nativeWriteFile("function:GetVideoPixels", "return:void", "result:success");
			}
		}); 

		//isSoftFit
		isSoftFit = (Button)findViewById(R.id.isSoftFit);
		isSoftFit.setOnClickListener(new Button.OnClickListener(){  
			public void onClick(View v) {
                /*result_b = nativeIsSoftFit(0);
        		Function.setText("IsSoftFit");
        		if (result_b == true)
        		{
        			result_s = String.valueOf(result_b);
        			Return_t.setText(result_s);
        			Result.setTextColor(Color.BLUE);
        			Result.setText("soft fit");
        			nativeWriteFile("function:IsSoftFit", "return:true", "result:soft fit");
        		}
        		else
        		{
        			Return_t.setText("false");
        			Result.setTextColor(Color.RED);
        			Result.setText("hardware fit");
        			nativeWriteFile("function:IsSoftFit", "return:false", "result:hardware fit");
                }*/
				if (nativeInit(url1, 1) == 0)
				  Log.i("Init:", "success");
				else {
				  Log.i("Init:", "error");
				}
				Log.d(getClass().getName(), "onResume()");

				Log.i("create surface:", "next");

				nativeSetEPGSize(1280, 720, 1);
				nativeSetVideoWindow(600, 320, 320, 240, 1);

				nativeStartPlay(1);
				Log.i("play", "success"); 

				drawSurface1 playData1 = new drawSurface1();
				playData1.url1 = url1;
				Thread player1 = new Thread(playData1);
				player1.start();
			}
		}); 

		//getPlayMode
		getPlayMode = (Button)findViewById(R.id.getPlayMode);
		getPlayMode.setOnClickListener(new Button.OnClickListener(){ 
			public void onClick(View v) {
				result_i = nativeGetPlayMode(0);
				Function.setText("GetPlayMode");
				result_s = String.valueOf(result_i);
				Return_t.setText(result_s);
				Result.setTextColor(Color.BLUE);
				Result.setText("success");
				nativeWriteFile("function:GetPlayMode", "return:"+result_s, "result:success");
			}
		}); 

		//setEPGSize
		setEPGSize = (Button)findViewById(R.id.setEPGSize);
		setEPGSize.setOnClickListener(new Button.OnClickListener(){ 
			public void onClick(View v) {
				nativeSetEPGSize(640, 530, 0);
				Function.setText("SetEPGSize");
				Return_t.setText("void");
				Result.setTextColor(Color.BLUE);
				Result.setText("success");
				nativeWriteFile("function:SetEPGSize", "return:void", "result:success"); 
			}
		});

        //SwitchAudioTrack
        switchAudioTrack = (Button)findViewById(R.id.switchAudioTrack);
        switchAudioTrack.setOnClickListener(new Button.OnClickListener() { 
            public void onClick(View v) {
                nativeSwitchAudioTrack(0);
            }
        });
    }

    static {
        System.loadLibrary("CTC_MediaProcessorjni");   
    }

    private static native int nativeCreateSurface(Surface mySurface, int width, int heigth, int use_omx_decoder);
    private static native int nativeInit(String url, int use_omx_decoder);
    private static native int nativeWriteData(String url, int bufsize, int use_omx_decoder);
    private static native int nativeSetVideoWindow(int x, int y, int width, int height, int use_omx_decoder);
    private static native boolean nativeStartPlay(int use_omx_decoder);
    private static native int nativeGetPlayMode(int use_omx_decoder);
    private static native boolean nativePause(int use_omx_decoder);
    private static native boolean nativeResume(int use_omx_decoder);
    private static native boolean nativeSeek(int use_omx_decoder);
    private static native int nativeVideoShow(int use_omx_decoder);
    private static native int nativeVideoHide(int use_omx_decoder);
    private static native boolean nativeFast(int use_omx_decoder);
    private static native boolean nativeStopFast(int use_omx_decoder);
    private static native boolean nativeStop(int use_omx_decoder);
    private static native int nativeGetVolume(int use_omx_decoder);
    private static native boolean nativeSetVolume(int volume, int use_omx_decoder);
    private static native boolean nativeSetRatio(int nRatio, int use_omx_decoder);
    private static native int nativeGetAudioBalance(int use_omx_decoder);
    private static native boolean nativeSetAudioBalance(int nAudioBalance, int use_omx_decoder);
    private static native void nativeGetVideoPixels(int use_omx_decoder);
    private static native boolean nativeIsSoftFit(int use_omx_decoder);
    private static native boolean nativeDelete();
    private static native void nativeSetEPGSize(int w, int h, int use_omx_decoder);
    private static native void nativeWriteFile(String functionName, String returnValue, String resultValue);
    private static native int nativeGetCurrentPlayTime(int use_omx_decoder);
    private static native void nativeSwitchAudioTrack(int use_omx_decoder);
    private static native void nativeInitSubtitle(int use_omx_decoder);
    private static native void nativeSwitchSubtitle(int sub_pid, int use_omx_decoder);
}
