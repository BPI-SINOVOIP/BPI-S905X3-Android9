/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */
/**
 * @Package com.droidlogic.miracast
 * @Description Copyright (c) Inspur Group Co., Ltd. Unpublished Inspur Group
 *              Co., Ltd. Proprietary & Confidential This source code and the
 *              algorithms implemented therein constitute confidential
 *              information and may comprise trade secrets of Inspur or its
 *              associates, and any use thereof is subject to the terms and
 *              conditions of the Non-Disclosure Agreement pursuant to which
 *              this source code was originally received.
 */
package com.droidlogic.miracast;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WpsInfo;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pDeviceList;
import android.net.wifi.p2p.WifiP2pManager;
import android.net.wifi.p2p.WifiP2pGroup;
import android.net.wifi.p2p.WifiP2pWfdInfo;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pManager.ActionListener;
import android.net.wifi.p2p.WifiP2pManager.Channel;
import android.net.wifi.p2p.WifiP2pManager.ChannelListener;
import android.net.wifi.p2p.WifiP2pManager.PeerListListener;
import android.net.wifi.p2p.WifiP2pManager.ConnectionInfoListener;
import android.net.wifi.p2p.WifiP2pManager.GroupInfoListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.Message;
import android.os.SystemProperties;
import android.os.FileObserver;
import android.os.FileUtils;
import android.os.UserHandle;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.text.TextUtils;

import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Display;
import android.view.MotionEvent;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Button;
import android.widget.Toast;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.AdapterView;
import android.provider.Settings;
import android.graphics.drawable.AnimationDrawable;

import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;
import android.content.ComponentName;

/**
 * @ClassName WiFiDirectMainActivity
 * @Description TODO
 * @Date 2015-12-04
 * @Email
 * @Author
 * @Version V2.2
 */
public class WiFiDirectMainActivity extends Activity implements
    ChannelListener, PeerListListener, ConnectionInfoListener, GroupInfoListener
{
    public static final String       TAG                    = "amlWifiDirect";
    public static final boolean     DEBUG              = true;
    public static final String       HRESOLUTION_DISPLAY     = "display_resolution_hd";
    public static final String       WIFI_P2P_IP_ADDR_CHANGED_ACTION = "android.net.wifi.p2p.IPADDR_INFORMATION";
    public static final String       WIFI_P2P_PEER_IP_EXTRA       = "IP_EXTRA";
    public static final String       WIFI_P2P_PEER_MAC_EXTRA       = "MAC_EXTRA";
    private static final String      MIRACAST_PREF          = "miracast_prefences";
    private static final String      IP_ADDR                = "ip_addr";
    public static final String ENCODING = "UTF-8";
    private static final String VERSION_FILE = "version";

    //private static final String ACTION_NETWORK_SETTINGS = "android.settings.NETWORK_SETTINGS";
    private static final String ACTION_NETWORK_SETTINGS_PACKAGE = "com.android.tv.settings";
    private static final String ACTION_NETWORK_SETTINGS_ACTIVITY = "com.android.tv.settings.MainSettings";
    public static final String  DNSMASQ_PATH = "/data/misc/dhcp";
    /***Miracast cert begin***/
    public boolean mForceStopScan = false;
    public boolean mStartConnecting = false;
    public boolean mManualInitWfdSession = false;
    public int mConnectImageNum;
    public long mConnectImageLastTime;
    public static final String       MIRACAST_CERT     = "miracast_cert";
    /***Miracast cert end***/

    private WifiP2pManager           manager;
    private WifiManager                mWifiManager;
    private boolean                  isWifiP2pEnabled       = false;
    private String                   mPort;
    private String                   mIP;
    private Handler                  mHandler               = new Handler();
    private static final int DIALOG_RENAME = 3;
    private final IntentFilter       intentFilter           = new IntentFilter();
    private Channel                  channel;
    private BroadcastReceiver        mReceiver              = null;
    private PowerManager.WakeLock    mWakeLock;
    private ImageView                mConnectStatus;
    private TextView                 mConnectWarn;
    private TextView                 mConnectDesc;
    private TextView                 mPeerList;
    private Button                 mClick2Settings;
    private boolean                  retryChannel           = false;
    private WifiP2pDevice            mDevice                = null;
    private ArrayList<WifiP2pDevice> peers                  = new ArrayList<WifiP2pDevice>();
    private ProgressDialog progressDialog = null;
    private OnClickListener mRenameListener;
    private EditText mDeviceNameText;
    private TextView mDeviceNameShow;
    private TextView mDeviceTitle;
    private String mSavedDeviceName;
    private int mNetId = -1;
    private SharedPreferences mPref;
    private SharedPreferences.Editor mEditor;
    private MenuItem mDisplayResolution;
    /***Miracast cert begin***/
    private Spinner mSpinner, mSpinDevice;
    private ArrayList<String> mFunList;
    private ArrayList<String> mDeviceList;
    private Button mBtnChannel;
    private EditText mEdtListenChannel, mEdtOperChannel;
    private TextView mFunlistText, mListChannelText, mOperChannelText, mDeviceListText;
    private InputMethodManager imm;
    private WifiP2pDevice mSelectDevice = null;
     /***Miracast cert end***/
    private boolean mFirstInit = false;

/********************Miracast cert begin**********************/
    private void initFunlist() {
        mFunList = new ArrayList<String>();
        mFunList.add(" ");
        mFunList.add("Start scan");
        mFunList.add("Stop scan");
        mFunList.add("Start listen");
        mFunList.add("Stop listen");
        mFunList.add("Enable autonomous GO");
        mFunList.add("Disable autonomous GO");
        mFunList.add("Connect to peer by display_pin");
        mFunList.add("Connect to peer by push_button");
        mFunList.add("Connect to peer by enter_pin");
        mFunList.add("Set wfd session manual mode");
        mFunList.add("Set wfd session auto mode");
        mFunList.add("Continue remaining wfd session under manual mode");
        mFunList.add("Enable wfd");
        mFunList.add("Disable wfd");
    }

    private void setCertComponentVisible(boolean value) {
        if (value) {
            mBtnChannel.setVisibility(View.VISIBLE);
            mEdtListenChannel.setVisibility(View.VISIBLE);
            mEdtOperChannel.setVisibility(View.VISIBLE);
            mFunlistText.setVisibility(View.VISIBLE);
            mDeviceListText.setVisibility(View.VISIBLE);
            mListChannelText.setVisibility(View.VISIBLE);
            mOperChannelText.setVisibility(View.VISIBLE);
            mSpinner.setVisibility(View.VISIBLE);
            mSpinDevice.setVisibility(View.VISIBLE);
        } else {
            mBtnChannel.setVisibility(View.INVISIBLE);
            mEdtListenChannel.setVisibility(View.INVISIBLE);
            mEdtOperChannel.setVisibility(View.INVISIBLE);
            mFunlistText.setVisibility(View.INVISIBLE);
            mDeviceListText.setVisibility(View.INVISIBLE);
            mListChannelText.setVisibility(View.INVISIBLE);
            mOperChannelText.setVisibility(View.INVISIBLE);
            mSpinner.setVisibility(View.INVISIBLE);
            mSpinDevice.setVisibility(View.INVISIBLE);
        }
    }

    private void  initCert() {
        initFunlist();
        mStartConnecting = false;
        mDeviceList = new ArrayList<String>();
        mDeviceList.add(" ");

        mSpinner = (Spinner)findViewById(R.id.spinner1);
        imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, mFunList);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mSpinner.setAdapter(adapter);
        mSpinner.setPrompt("Please function!");

        mSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> arg0, View arg1,
                int arg2, long arg3) {
                switch (arg2) {
                    case 1: // Start scan
                        tryDiscoverPeers();
                        break;

                    case 2: // Stop scan
                        stopPeerDiscovery();
                        break;

                    case 3: // Start listen
                        startListen();
                        break;

                    case 4:// Stop listen
                        stopListen();
                        break;

                    case 5:// Enable autonomous GO
                        startAutonomousGroup();
                        break;

                    case 6:// Disable autonomous GO
                        stopAutonomousGroup();
                        break;

                    case 7:// Connect to peer by display_pin
                        if (mSelectDevice == null) {
                            Toast.makeText(WiFiDirectMainActivity.this, "device is null, please select device from deviceList!", Toast.LENGTH_LONG).show();
                        } else {
                            startConnect(mSelectDevice, 1);
                        }
                        break;

                    case 8:// Connect to peer by push_button
                        if (mSelectDevice == null) {
                            Toast.makeText(WiFiDirectMainActivity.this, "device is null, please select device from deviceList!", Toast.LENGTH_LONG).show();
                        } else {
                            startConnect(mSelectDevice, 0);
                        }
                        break;

                    case 9:// Set wfd session manual mode
                        if (mSelectDevice == null) {
                            Toast.makeText(WiFiDirectMainActivity.this, "device is null, please select device from deviceList!", Toast.LENGTH_LONG).show();
                        } else {
                            startConnect(mSelectDevice, 2);
                        }
                        break;

                    case 10:// Set wfd session auto mode
                        setWfdSessionMode(true);
                        break;

                    case 11:// Set wfd session auto mode
                        setWfdSessionMode(false);
                        break;

                    case 12:// Continue remaining wfd session under manual mode
                        startWfdSession();
                        break;

                    case 13:// Enable wfd
                        changeRole(true);
                        break;

                    case 14:// Disable wfd
                        changeRole(false);
                        break;

                    default:
                        break;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        mDeviceListText = (TextView)findViewById(R.id.text_device);
        mSpinDevice = (Spinner)findViewById(R.id.spinner_device);
        ArrayAdapter<String> adapter1 = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, mDeviceList);
        adapter1.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mSpinDevice.setAdapter(adapter1);
        mSpinDevice.setPrompt("Please select device");
        mSpinDevice.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
                if (arg2 != 0) {
                     mSelectDevice = peers.get(arg2 - 1);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> arg0) {
            }
        });

        mFunlistText = (TextView)findViewById(R.id.textView2);
        mListChannelText = (TextView)findViewById(R.id.tv_channel);
        mOperChannelText = (TextView)findViewById(R.id.tv_oper_channel);
        mBtnChannel = (Button)findViewById(R.id.btn_channel);
        mEdtListenChannel = (EditText)findViewById(R.id.edt_listen_channel);
        mEdtOperChannel = (EditText)findViewById(R.id.edt_oper_channel);

        mBtnChannel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                imm.toggleSoftInput(0, InputMethodManager.HIDE_NOT_ALWAYS);
                if (mEdtListenChannel.getText().toString().equals("") ||
                    mEdtOperChannel.getText().toString().equals("")) {
                    Toast.makeText(WiFiDirectMainActivity.this, "listen channel or oper channel is empty, please fill it!", Toast.LENGTH_LONG).show();
                } else {
                    int lc = Integer.parseInt(mEdtListenChannel.getText().toString());
                    int oc = Integer.parseInt(mEdtOperChannel.getText().toString());
                    setWifiP2pChannels(lc, oc);
                }
            }
        });
    }

    private static String describeWifiP2pDevice(WifiP2pDevice device) {
        return device != null ? device.toString().replace('\n', ',') : "null";
    }

    private void requestPeers() {
        manager.requestPeers(channel, new PeerListListener() {
            @Override
            public void onPeersAvailable(WifiP2pDeviceList peerLists) {
                Log.d(TAG, "Received list of peers.");
                peers.clear();

                for (WifiP2pDevice device : peerLists.getDeviceList()) {
                    Log.d(TAG, "  " + describeWifiP2pDevice(device));
                    peers.add(device);
                }

                String list = WiFiDirectMainActivity.this.getResources().getString(R.string.peer_list);

                for (int i = 0; i < peers.size(); i++) {
                    list += "    " + peers.get(i).deviceName;
                    Log.d(TAG, "onPeersAvailable peerDevice:" + peers.get(i).deviceName+ ", status:" + peers.get(i).status + " (0-CONNECTED,3-AVAILABLE)");
                    mPeerList.setText(list);
                }
            }
        });
    }


    private void tryDiscoverPeers() {
        mForceStopScan = false;
        manager.discoverPeers(channel, new ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Discover peers succeed.  Requesting peers now.");
                requestPeers();
            }

            @Override
            public void onFailure(int reason) {
                Log.d(TAG, "Discover peers failed with reason " + reason + ".");
            }
        });
    }

    public void stopPeerDiscovery() {
        Log.d(TAG, "stopPeerDiscover , do ForceStopScan....");
        mForceStopScan = true;
        //Log.d(TAG, "WPM.stopPeerDiscovery", new Throwable());
        manager.stopPeerDiscovery(channel, new ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Stop peer discovery succeed.");
            }

            @Override
            public void onFailure(int reason) {
                Log.d(TAG, "Stop peer discovery failed with reason " + reason + ".");
            }
        });
    }

    private void startListen() {
        manager.listen(channel, true, new ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "start listen peers succeed.");
            }

            @Override
            public void onFailure(int reason) {
                Log.d(TAG, "start listen peers failed with reason " + reason + ".");
            }
        });
    }

    private void stopListen() {
        manager.listen(channel, false, new ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "stop listen peers succeed.");
            }

            @Override
            public void onFailure(int reason) {
                Log.d(TAG, "stop listen peers failed with reason " + reason + ".");
            }
        });
    }

    private void startAutonomousGroup() {
        mForceStopScan = true;
        manager.createGroup(channel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "createGroup Success");
            }

            @Override
            public void onFailure(int reasonCode) {
                Log.d(TAG, "createGroup Failure");
            }
        });
    }

    private void stopAutonomousGroup() {
        mForceStopScan = false;
        manager.removeGroup(channel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "removeGroup Success");
            }

            @Override
            public void onFailure(int reasonCode) {
                Log.d(TAG, "removeGroup Failure");
            }
        });
    }

    private void startConnect(WifiP2pDevice device, int config) {
        if (config > WpsInfo.KEYPAD)
            Log.d(TAG, "startConnect config invalid.");

        if (config == WpsInfo.PBC)
            Log.d(TAG, "startConnect PBC configuration");
        else if (config == WpsInfo.DISPLAY)
            Log.d(TAG, "startConnect DISPLAY configuration");
        else if (config == WpsInfo.KEYPAD)
            Log.d(TAG, "startConnect KEYPAD configuration");

        WifiP2pConfig wifip2pconfig = new WifiP2pConfig();
        WpsInfo wpsConfig = new WpsInfo();
        wpsConfig.setup = config;
        wifip2pconfig.wps = wpsConfig;
        wifip2pconfig.deviceAddress = device.deviceAddress;
        wifip2pconfig.groupOwnerIntent = 15;
        mStartConnecting = true;

        manager.connect(channel, wifip2pconfig, new ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "startConnect Success.");
            }

            @Override
            public void onFailure(int reason) {
                mStartConnecting = false;
                Log.d(TAG, "startConnect failed with reason " + reason + ".");
            }
        });
    }

    private void setWfdSessionMode(boolean mode) {
        Log.d(TAG, "setWfdSessionMode " + mode);
        mManualInitWfdSession = mode;
    }

    private void startWfdSession() {
        if (mManualInitWfdSession) {
            Log.d(TAG, "start Manual WfdSession ");
            setConnect();
            Log.d(TAG, "start miracast");
            Intent intent = new Intent(WiFiDirectMainActivity.this, SinkActivity.class);
            Bundle bundle = new Bundle();
            bundle.putString(SinkActivity.KEY_PORT, mPort);
            bundle.putString(SinkActivity.KEY_IP, mIP);
            bundle.putBoolean(HRESOLUTION_DISPLAY, mPref.getBoolean(HRESOLUTION_DISPLAY, true));
            intent.putExtras(bundle);
            startActivity(intent);
        } else {
            Log.d(TAG, "error Auto WfdSessionMode,return");
            return;
        }
    }

    private void setWifiP2pChannels(int lc, int oc) {
        Log.d(TAG, "setWifiP2pChannels lc:" + lc + " oc:" + oc);
        manager.setWifiP2pChannels(channel, lc, oc, new ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "setWifiP2pChannels succeeded.  ");
            }
            @Override
            public void onFailure(int reason) {
                Log.d(TAG, "setWifiP2pChannels failed with reason " + reason + ".");
            }
        });
    }

/********************Miracast cert end**********************/

    /** register the BroadcastReceiver with the intent values to be matched */
    @Override
    public void onResume()
    {
        Log.d(TAG, "onResume====>");
        Log.d(TAG, "removeGroup...");
        super.onResume();
        manager.removeGroup(channel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "removeGroup Success");
            }

            @Override
            public void onFailure(int reasonCode) {
                Log.d(TAG, "removeGroup Failure");
            }
        });

        /* enable backlight */
        PowerManager pm = (PowerManager) getSystemService (Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock (PowerManager.SCREEN_BRIGHT_WAKE_LOCK
                                    | PowerManager.ON_AFTER_RELEASE, TAG);
        mWakeLock.acquire();
        if (mFirstInit) {
            Intent intent = new Intent(WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
            removeStickyBroadcastAsUser(intent, UserHandle.ALL);
        } else {
            mFirstInit = true;
        }
        initCert();//miracast certification
        mConnectStatus = (ImageView) findViewById(R.id.show_connect);
        mConnectImageNum = 5;
        mConnectStatus.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mPref.getBoolean(MIRACAST_CERT, false)) {
                    if (mConnectImageNum != 5) {
                        if (mConnectImageLastTime < (SystemClock.uptimeMillis()-500)) {
                            mConnectImageNum = 5;
                        }
                    }
                    mConnectImageNum--;
                    mConnectImageLastTime = SystemClock.uptimeMillis();
                    if (mConnectImageNum <= 0) {
                        mConnectImageNum = 5;
                        mEditor.putBoolean(MIRACAST_CERT, false);
                        mEditor.commit();
                        setCertComponentVisible(false);
                    } else {
                        Log.d(TAG, "already miracast cert mode, return!!!");
                    }
                    return;
                }
                if (mConnectImageNum != 5) {
                    if (mConnectImageLastTime < (SystemClock.uptimeMillis()-500)) {
                        mConnectImageNum = 5;
                    }
                }
                mConnectImageNum--;
                mConnectImageLastTime = SystemClock.uptimeMillis();
                if (mConnectImageNum <= 0) {
                    mConnectImageNum = 5;
                    mEditor.putBoolean(MIRACAST_CERT, true);
                    mEditor.commit();
                    setCertComponentVisible(true);
                }
                Log.d(TAG, "mConnectImageNum: " + mConnectImageNum);
            }
        });
        mConnectDesc = (TextView) findViewById (R.id.show_connect_desc);
        mConnectWarn = (TextView) findViewById (R.id.show_desc_more);
        mClick2Settings = (Button) findViewById (R.id.settings_btn);
        mConnectDesc.setFocusable (true);
        mConnectDesc.requestFocus();
        mPeerList = (TextView) findViewById (R.id.peer_devices);
        mClick2Settings.setOnClickListener (new View.OnClickListener()
        {
            @Override
            public void onClick (View v)
            {
                //WiFiDirectMainActivity.this.startActivity (new Intent (
                //            ACTION_NETWORK_SETTINGS) );
                Intent intent = new Intent(Intent.ACTION_VIEW);
                intent.setComponent(new ComponentName(ACTION_NETWORK_SETTINGS_PACKAGE, ACTION_NETWORK_SETTINGS_ACTIVITY));
                startActivity(intent);
            }
        });
        if (!isNetAvailiable() )
        {
            mConnectWarn.setText (WiFiDirectMainActivity.this.getResources()
                                  .getString (R.string.p2p_off_warning) );
            mConnectWarn.setVisibility (View.VISIBLE);
            mClick2Settings.setVisibility (View.VISIBLE);
            mConnectDesc.setFocusable (false);
        }
        mDeviceNameShow = (TextView) findViewById (R.id.device_dec);
        mDeviceTitle = (TextView) findViewById (R.id.device_title);
        if (mDevice != null)
        {
            mSavedDeviceName = mDevice.deviceName;
            mDeviceNameShow.setText (mSavedDeviceName);
        }
        else
        {
            mDeviceTitle.setVisibility (View.INVISIBLE);
        }
        resetData();
        if (!mPref.getBoolean(MIRACAST_CERT, false))
            setCertComponentVisible(false);
        else
            setCertComponentVisible(true);
        registerReceiver(mReceiver, intentFilter);
        tryDiscoverPeers();
    }

    private final Runnable startSearchRunnable = new Runnable()
    {
        @Override
        public void run()
        {
            if (!mForceStopScan) {
                Log.d(TAG, "ForceStopScan = false ; startSearchRunnable.");
                startSearch();
            }
        }
    };

    private final Runnable searchAgainRunnable = new Runnable() {
    @Override
        public void run() {
            if (!mForceStopScan && peers.isEmpty()) {
                if (DEBUG)
                    Log.d(TAG, "mForceStopScan is false, no peers, search again.");
                startSearch();
            }
        }
    };

    public void startSearchTimer() {
        if (DEBUG) Log.d(TAG, " startSearchTimer 6s");
            mHandler.postDelayed(startSearchRunnable, 6000);
    }

    public void cancelSearchTimer(){
        if (DEBUG) Log.d(TAG, " cancelSearchTimer");
            mHandler.removeCallbacks(startSearchRunnable);
        if (DEBUG) Log.d(TAG, " cancel searchAgainRunnable");
            mHandler.removeCallbacks(searchAgainRunnable);
    }

    @Override
    public void onContentChanged()
    {
        super.onContentChanged();
    }

    public void setDevice (WifiP2pDevice device)
    {
        mDevice = device;
        if (mDevice != null)
        {
            if (mDeviceTitle != null)
            {
                mDeviceTitle.setVisibility (View.VISIBLE);
            }
            mSavedDeviceName = mDevice.deviceName;
            if (mDeviceNameShow != null)
            {
                mDeviceNameShow.setText (mSavedDeviceName);
            }
        }
        Log.e(TAG, "====setDevice.mDevice.status: " + mDevice.status);
        if ((WifiP2pDevice.CONNECTED == mDevice.status)
            || (WifiP2pDevice.INVITED == mDevice.status))
        {
            cancelSearchTimer();
        }
        if (DEBUG)
        {
            Log.d (TAG, "localDevice name:" + mDevice.deviceName + ", status:" + mDevice.status + " (0-CONNECTED,3-AVAILABLE)");
        }
    }

    public void startSearch()
    {
        if (DEBUG)
        {
            Log.d (TAG, "startSearch wifiP2pEnabled:" + isWifiP2pEnabled);
        }
        if (mHandler.hasCallbacks(startSearchRunnable))
            cancelSearchTimer();
        if (!isWifiP2pEnabled)
        {
            if (manager != null && channel != null)
            {
                mConnectWarn.setVisibility (View.VISIBLE);
                mConnectWarn.setText (WiFiDirectMainActivity.this.getResources()
                                      .getString (R.string.p2p_off_warning) );
                mClick2Settings.setVisibility (View.VISIBLE);
                mConnectDesc.setFocusable (false);
            }
            return;
        }
        if (!SystemProperties.getBoolean("wfd.p2p.go", false)) {
            //Log.d(TAG, "WPM.discoverPeers", new Throwable());
            manager.discoverPeers(channel, new WifiP2pManager.ActionListener() {
                @Override
                public void onSuccess() {
                    Log.d(TAG, "discoverPeers init success");
                }

                @Override
                public void onFailure(int reasonCode) {
                    Log.d(TAG, "discoverPeers init failure, reasonCode:" + reasonCode);
                }
            });
        }
    }
    public void onQuery (MenuItem item)
    {
        if (mDisplayResolution == null)
        {
            return;
        }
        Resources res = WiFiDirectMainActivity.this.getResources();
        switch (item.getItemId() )
        {
        case R.id.setting_sd:
            mDisplayResolution.setTitle (res.getString (R.string.setting_definition)
                                         + " : " + res.getString (R.string.setting_definition_sd) );
            mEditor.putBoolean (HRESOLUTION_DISPLAY, false);
            mEditor.commit();
            break;
        case R.id.setting_hd:
            mDisplayResolution.setTitle (res.getString (R.string.setting_definition)
                                         + " : " + res.getString (R.string.setting_definition_hd) );
            mEditor.putBoolean (HRESOLUTION_DISPLAY, true);
            mEditor.commit();
            break;
        }
    }

    @Override
    public void onPause()
    {
        Log.d(TAG, "onPause====>");
        super.onPause();
        unregisterReceiver(mReceiver);
        mWakeLock.release();
        stopPeerDiscovery();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown keyCode:" + keyCode + " event:" + event);
        switch (keyCode) {
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            case KeyEvent.KEYCODE_VOLUME_MUTE:
            case KeyEvent.KEYCODE_BACK:
                break;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyUp keyCode:" + keyCode + " event:" + event);
        switch (keyCode) {
           case KeyEvent.KEYCODE_1:
               tryDiscoverPeers();
               break;
           case KeyEvent.KEYCODE_2:
               stopPeerDiscovery();
               break;
           case KeyEvent.KEYCODE_3:
               startListen();
               break;
           case KeyEvent.KEYCODE_4:
               stopListen();
               break;
           case KeyEvent.KEYCODE_5:
               changeRole(true);
               break;
           case KeyEvent.KEYCODE_6:
               changeRole(false);
               break;
           case KeyEvent.KEYCODE_7:
               startAutonomousGroup();
               break;
           case KeyEvent.KEYCODE_8:
               stopAutonomousGroup();
               break;
           case KeyEvent.KEYCODE_9:
               int tmp_lc = SystemProperties.getInt("wfd.p2p.lc", 1);
               int tmp_oc = SystemProperties.getInt("wfd.p2p.oc", 1);
               setWifiP2pChannels(tmp_lc, tmp_oc);
               break;
           case KeyEvent.KEYCODE_0:
               //int tmp_cfg = SystemProperties.getInt("wfd.p2p.wps", 0);
               //startWps(tmp_cfg);
               break;
           case KeyEvent.KEYCODE_A:
               setWfdSessionMode(!mManualInitWfdSession);
               break;
           case KeyEvent.KEYCODE_B:
               startWfdSession();
               break;
           }
           return super.onKeyUp(keyCode, event);
     }
    /*
     * (non-Javadoc)
     *
     * @see
     * android.net.wifi.p2p.WifiP2pManager.ChannelListener#onChannelDisconnected
     * ()
     */
    @Override
    public void onChannelDisconnected()
    {
        if (manager != null && !retryChannel)
        {
            Toast.makeText (this, WiFiDirectMainActivity.this.getResources().getString (R.string.channel_try),
                            Toast.LENGTH_LONG).show();
            resetData();
            retryChannel = true;
            manager.initialize (this, getMainLooper(), this);
        }
        else
        {
            Toast.makeText (
                this,
                WiFiDirectMainActivity.this.getResources().getString (R.string.channel_close),
                Toast.LENGTH_LONG).show();
            //retryChannel = false;
        }
    }

    public void resetData()
    {
        mConnectStatus.setBackgroundResource (R.drawable.wifi_connect);
        String sFinal1 = String.format (getString (R.string.connect_ready), getString (R.string.device_name) );
        mConnectDesc.setText (sFinal1);
        peers.clear();

        String list = WiFiDirectMainActivity.this.getResources().getString(R.string.peer_list);
        mPeerList.setText(list);
    }

    public void setConnect()
    {
        mConnectDesc.setText (getString (R.string.connected_info) );
        mConnectStatus.setBackgroundResource (R.drawable.wifi_yes);
    }

    public void setIsWifiP2pEnabled (boolean enable)
    {
        this.isWifiP2pEnabled = enable;
        Log.d(TAG,"setIsWifiP2pEnabled isWifiP2pEnabled = " + isWifiP2pEnabled);
        String sFinal1 = String.format(getString(R.string.connect_ready),getString(R.string.device_name));
        mConnectDesc.setText(sFinal1);
        if (enable)
        {
            mConnectWarn.setVisibility(View.INVISIBLE);
            mClick2Settings.setVisibility(View.GONE);
            mConnectDesc.setFocusable(false);
        }
        else
        {
            mConnectWarn.setText(WiFiDirectMainActivity.this.getResources()
                        .getString(R.string.p2p_off_warning));
            mConnectWarn.setVisibility(View.VISIBLE);
            mClick2Settings.setVisibility (View.VISIBLE);
            mConnectDesc.setFocusable (true);
        }
    }

    public void startMiracast (String ip, String port)
    {
        mPort = port;
        mIP = ip;
        if (mManualInitWfdSession) {
            Log.d(TAG,"waiting startMiracast");
            return;
        }
        setConnect();
        Log.d(TAG, "start miracast");
        Intent intent = new Intent (WiFiDirectMainActivity.this, SinkActivity.class);
        Bundle bundle = new Bundle();
        bundle.putString (SinkActivity.KEY_PORT, mPort);
        bundle.putString (SinkActivity.KEY_IP, mIP);
        bundle.putBoolean (HRESOLUTION_DISPLAY, mPref.getBoolean (HRESOLUTION_DISPLAY, true) );
        intent.putExtras (bundle);
        startActivity (intent);
    }

    private void fixRtspFail()
    {
        if (manager != null /*&& mNetId != -1*/)
        {
            manager.removeGroup (channel, null);
            if (mNetId != -1)
                manager.deletePersistentGroup(channel, mNetId, null);
        }
    }

    @Override
    protected void onCreate (Bundle savedInstanceState)
    {
        super.onCreate (savedInstanceState);
        setContentView (R.layout.connect_layout);
        // add necessary intent values to be matched.
        intentFilter.addAction (WifiP2pManager.WIFI_P2P_STATE_CHANGED_ACTION);
        intentFilter.addAction (WifiP2pManager.WIFI_P2P_PEERS_CHANGED_ACTION);
        intentFilter.addAction (WifiP2pManager.WIFI_P2P_CONNECTION_CHANGED_ACTION);
        intentFilter.addAction (WifiP2pManager.WIFI_P2P_THIS_DEVICE_CHANGED_ACTION);
        intentFilter.addAction (WifiP2pManager.WIFI_P2P_DISCOVERY_CHANGED_ACTION);
        intentFilter.addAction (WIFI_P2P_IP_ADDR_CHANGED_ACTION);
        manager = (WifiP2pManager) getSystemService (Context.WIFI_P2P_SERVICE);
        mWifiManager = (WifiManager) getSystemService (Context.WIFI_SERVICE);
        channel = manager.initialize (this, getMainLooper(), null);
        mRenameListener = new OnClickListener()
        {
            @Override
            public void onClick (DialogInterface dialog, int which)
            {
                if (which == DialogInterface.BUTTON_POSITIVE)
                {
                    if (manager != null)
                    {
                        manager.setDeviceName (channel,
                                               mDeviceNameText.getText().toString(),
                                               new WifiP2pManager.ActionListener()
                        {
                            public void onSuccess()
                            {
                                mSavedDeviceName = mDeviceNameText.getText().toString();
                                mDeviceNameShow.setText (mSavedDeviceName);
                                if (DEBUG)
                                {
                                    Log.d (TAG, " device rename success");
                                }
                            }
                            public void onFailure (int reason)
                            {
                                Toast.makeText (WiFiDirectMainActivity.this,
                                                R.string.wifi_p2p_failed_rename_message,
                                                Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                }
            }
        };

        mReceiver = new WiFiDirectBroadcastReceiver (manager, channel, this);
        mPref = PreferenceManager.getDefaultSharedPreferences (this);
        mEditor = mPref.edit();
        changeRole(true);
    }

    @Override
    protected void onDestroy()
    {
        changeRole(false);
        Log.d (TAG, "onDestroy do stopPeerDiscovery");
        setIsWifiP2pEnabled (false);
        resetData();
        mFirstInit = false;
        super.onDestroy();
    }

    @Override
    public void onWindowFocusChanged (boolean hasFocus)
    {
        super.onWindowFocusChanged (hasFocus);
        mConnectStatus.setBackgroundResource (R.drawable.wifi_connect);
        AnimationDrawable anim = (AnimationDrawable) mConnectStatus.getBackground();
        anim.start();
    }

    /*
     * (non-Javadoc)
     *
     * @see
     * android.net.wifi.p2p.WifiP2pManager.PeerListListener#onPeersAvailable
     * (android.net.wifi.p2p.WifiP2pDeviceList)
     */
    @Override
    public void onPeersAvailable (WifiP2pDeviceList devicelist)
    {
        String list = WiFiDirectMainActivity.this.getResources().getString(R.string.peer_list);
        if (progressDialog != null && progressDialog.isShowing())
        {
            progressDialog.dismiss();
        }

        Log.d(TAG, "===onPeersAvailable===");
        peers.clear();
        mDeviceList.clear();
        mDeviceList.add(" ");
        peers.addAll(devicelist.getDeviceList());
        freshView();
        for (int i = 0; i < peers.size(); i++)
        {
            if (!peers.get(i).wfdInfo.isWfdEnabled())
            {
                Log.d(TAG, "peerDevice:" + peers.get(i).deviceName + " is not a wfd device");
                continue;
            }
            if (!peers.get(i).wfdInfo.isSessionAvailable())
            {
                Log.d(TAG, "peerDevice:" + peers.get(i).deviceName + " is an unavailable wfd session");
                continue;
            }

            if ((WifiP2pDevice.INVITED == peers.get(i).status)
                || (WifiP2pDevice.CONNECTED == peers.get(i).status))
                cancelSearchTimer();
            list += "    " + peers.get(i).deviceName;
            mDeviceList.add(peers.get(i).deviceName);
            if (DEBUG)
                Log.d(TAG, "peerDevice:" + peers.get(i).deviceName + ", status:" + peers.get(i).status + " (0-CONNECTED,3-AVAILABLE)");
        }
        mPeerList.setText(list);
        Log.d(TAG, "===onPeersAvailable===");

        if (!mForceStopScan)
            mHandler.postDelayed(searchAgainRunnable, 5000);
    }

    public void onGroupInfoAvailable (WifiP2pGroup group)
    {
        if (group != null)
        {
            Log.d (TAG, "onGroupInfoAvailable true : " + group);
            mNetId = group.getNetworkId();
        }
        else
        {
            Log.d (TAG, "onGroupInfoAvailable false");
            mNetId = -1;
        }
    }

    /**
     * @Description TODO
     */
    private void freshView()
    {
        for (int i = 0; i < peers.size(); i++)
        {
            if (peers.get (i).status == WifiP2pDevice.CONNECTED)
            {
                mConnectDesc.setText (getString (R.string.connecting_desc)
                                      + peers.get (i).deviceName);
                break;
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu ( Menu menu )
    {
        getMenuInflater().inflate ( R.menu.action_items, menu );
        mDisplayResolution = menu.findItem (R.id.setting_definition);
        if (mPref != null)
        {
            boolean high = mPref.getBoolean (HRESOLUTION_DISPLAY, true);
            Resources res = WiFiDirectMainActivity.this.getResources();
            if (high)
            {
                mDisplayResolution.setTitle (res.getString (R.string.setting_definition)
                                             + " : " + res.getString (R.string.setting_definition_hd) );
            }
            else
            {
                mDisplayResolution.setTitle (res.getString (R.string.setting_definition)
                                             + " : " + res.getString (R.string.setting_definition_sd) );
            }
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected ( MenuItem item )
    {
        int itemId = item.getItemId();
        switch ( itemId )
        {
        case R.id.about_version:
            AlertDialog.Builder builder = new Builder (WiFiDirectMainActivity.this);
            if (getResources().getConfiguration().locale.getCountry().equals ("CN") )
            {
                builder.setMessage (getFromAssets (VERSION_FILE + "_cn") );
            }
            else
            {
                builder.setMessage (getFromAssets (VERSION_FILE) );
            }
            builder.setTitle (R.string.about_version);
            builder.setPositiveButton (R.string.close_dlg, new DialogInterface.OnClickListener()
            {
                @Override
                public void onClick (DialogInterface dialog, int which)
                {
                    dialog.dismiss();
                }
            });
            builder.create().show();
            break;
        case R.id.atn_direct_discover:
            mPeerList.setText (R.string.peer_list);
            startSearch();
            return true;
        case R.id.setting_name:
            showDialog (DIALOG_RENAME);
            return true;
        default:
            break;
        }
        return super.onOptionsItemSelected ( item );
    }

    public String getFromAssets (String fileName)
    {
        String result = "";
        InputStream in = null;
        try {
            in = getResources().getAssets().open(fileName);
            int lenght = in.available();
            byte[]  buffer = new byte[lenght];
            in.read(buffer);
            result = new String(buffer, ENCODING);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                if (in != null)
                    in.close();
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return result;
    }

    private boolean isNetAvailiable()
    {
        if (mWifiManager != null) {
            int state = mWifiManager.getWifiState();
            if (WifiManager.WIFI_STATE_ENABLING == state
                || WifiManager.WIFI_STATE_ENABLED == state) {
                Log.d(TAG, "WIFI enabled");
                return true;
            } else {
                Log.d(TAG, "WIFI disabled");
                return false;
            }
        }
        return false;
    }

    public void onConnectionInfoAvailable (WifiP2pInfo info)
    {
        if (DEBUG) {
            Log.d (TAG, "onConnectionInfoAvailable info:" + info);
        }
    }

    @Override
    public Dialog onCreateDialog (int id)
    {
        if (id == DIALOG_RENAME)
        {
            mDeviceNameText = new EditText (this);
            if (mSavedDeviceName != null)
            {
                mDeviceNameText.setText (mSavedDeviceName);
                mDeviceNameText.setSelection (mSavedDeviceName.length() );
            }
            else if (mDevice != null && !TextUtils.isEmpty (mDevice.deviceName) )
            {
                mDeviceNameText.setText (mDevice.deviceName);
                mDeviceNameText.setSelection (0, mDevice.deviceName.length() );
            }
            mSavedDeviceName = null;
            AlertDialog dialog = new AlertDialog.Builder (this)
            .setTitle (R.string.change_name)
            .setView (mDeviceNameText)
            .setPositiveButton (WiFiDirectMainActivity.this.getResources().getString (R.string.dlg_ok), mRenameListener)
            .setNegativeButton (WiFiDirectMainActivity.this.getResources().getString (R.string.dlg_cancel), null)
            .create();
            return dialog;
        }
        return null;
    }

    private void changeRole (boolean isSink)
    {
        WifiP2pWfdInfo wfdInfo = new WifiP2pWfdInfo();

        if (isSink) {
            wfdInfo.setWfdEnabled(true);
            wfdInfo.setDeviceType(WifiP2pWfdInfo.PRIMARY_SINK);
            wfdInfo.setSessionAvailable(true);
            wfdInfo.setControlPort(7236);
            wfdInfo.setMaxThroughput(50);
        } else {
            wfdInfo.setWfdEnabled(false);
        }

        manager.setWFDInfo(channel, wfdInfo, new ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Successfully set WFD info.");
            }

            @Override
            public void onFailure(int reason) {
                Log.d(TAG, "Failed to set WFD info with reason " + reason + ".");
                try {
                    Thread.sleep(500);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                WiFiDirectMainActivity.this.changeRole(true);
            }
        });
    }

    public void discoveryStop()
    {
        if (progressDialog != null && progressDialog.isShowing() )
        {
            progressDialog.dismiss();
        }
    }

}
