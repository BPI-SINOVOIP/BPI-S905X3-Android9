package com.droidlogic.droidlivetv.favlistsettings;

import android.app.Service;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.util.Log;

import java.util.Map;

import com.droidlogic.droidlivetv.favlistsettings.IInterationCallback;
import com.droidlogic.droidlivetv.favlistsettings.IInterationService;

public class InterationService extends Service {
    private static final String TAG = "InterationService";
    private static final boolean DEBUG = true;
    private IBinder binder = new LocalBinder();
    private IInterationService clinet = null;
    private InterationServiceConnection mInterationServiceConnection = new InterationServiceConnection();
    private boolean mIsBounded = false;

    private RemoteCallbackList<IInterationCallback> mRemoteCallbackList = new RemoteCallbackList<IInterationCallback>();

    @Override
    public IBinder onBind(Intent intent) {
        Log.i(TAG, "onBind");
        Map ss;
        return binder;
    }

    private void broadCastItem(String result) {
        Log.d(TAG, "broadCastItem begin");
        int callBackSize = mRemoteCallbackList.beginBroadcast();
        if (callBackSize == 0) {
            return;
        }
        for (int i = 0; i < callBackSize; i++) {
            try {
                mRemoteCallbackList.getBroadcastItem(i).onReceiveInterationCallback(result);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        mRemoteCallbackList.finishBroadcast();
        Log.d(TAG, "broadCastItem finish");
    }

    @Override
    public void onCreate() {
        Log.i(TAG, "onCreate");
        super.onCreate();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.i(TAG, "onStartCommand");
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Log.i(TAG, "onUnbind");
        return true;
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "onDestory");
        super.onDestroy();
    }

    @Override
    public void onRebind(Intent intent) {
        Log.i(TAG, "onRebind");
        super.onRebind(intent);
    }

    public class LocalBinder extends IInterationService.Stub {
        public InterationService getService() {
            return InterationService.this;
        }

        @Override
        public void registerInterationCallback(IInterationCallback callback) throws RemoteException {
            mRemoteCallbackList.register(callback);
            InterationService.this.broadCastItem("registerInterationCallback = " + callback.toString());
        }

        @Override
        public void unRegisterInterationCallback(IInterationCallback callback) throws RemoteException {
            mRemoteCallbackList.unregister(callback);
            InterationService.this.broadCastItem("unRegisterInterationCallback = " + callback.toString());
        }

        @Override
        public void sendInterationCallback(String value) {
            InterationService.this.broadCastItem(value);
        }
    }

    private final class InterationServiceConnection implements ServiceConnection {
        public void onServiceConnected(ComponentName name, IBinder service) {
            clinet = IInterationService.Stub.asInterface(service);
            mIsBounded = true;
        }
        public void onServiceDisconnected(ComponentName name) {
            mIsBounded = false;
            clinet = null;
        }
    }

    private void bindInterationService() {
        Intent intent = new Intent("droidlogic.intent.action.InterationService");
        bindService(intent, mInterationServiceConnection, Service.BIND_AUTO_CREATE);
    }

    private void unbindInterationService() {
        mIsBounded = false;
        unbindService(mInterationServiceConnection);
    }
}
