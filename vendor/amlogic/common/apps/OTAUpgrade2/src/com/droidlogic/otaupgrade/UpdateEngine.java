/******************************************************************
*
*Copyright (C) 2016 Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.otaupgrade;

import android.content.Context;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.Log;

public class UpdateEngine {
    private static final String TAG = "UpdateEngine";
    int TRANSACTION_PAYLOAD                    = IBinder.FIRST_CALL_TRANSACTION;
    int TRANSACTION_BIND               = IBinder.FIRST_CALL_TRANSACTION + 1;
    int TRANSACTION_SUSPEND               = IBinder.FIRST_CALL_TRANSACTION + 2;
    int TRANSACTION_RESUME             = IBinder.FIRST_CALL_TRANSACTION + 3;
    int TRANSACTION_CANCEL             = IBinder.FIRST_CALL_TRANSACTION + 4;
    int TRANSACTION_RESET_STATUS             = IBinder.FIRST_CALL_TRANSACTION + 5;
    private static final String UPDATE_TOKEN         = "android.os.IUpdateEngine";
    public static final int REMOTE_EXCEPTION        = -0xffff;
    private Context mContxt;
    private IBinder mIBinder = null;

    public UpdateEngine(Context cxt){
        mContxt=cxt;
        try {
            Object object = Class.forName("android.os.ServiceManager")
                    .getMethod("getService", new Class[] { String.class })
                    .invoke(null, new Object[] { "android.os.UpdateEngineService" });
            mIBinder = (IBinder)object;
        }
        catch (Exception ex) {
            ex.printStackTrace();
            Log.e(TAG, "image player init fail:" + ex);
        }
    }
    public int applyPayload(String url, long payload_offset, long payload_size, String[] headerKeyValuePairs){
        if ( null != mIBinder ) {
            try {
                Parcel data=Parcel.obtain();
                Parcel reply=Parcel.obtain();
                data.writeInterfaceToken(UPDATE_TOKEN);
                data.writeString(url);
                data.writeLong(payload_offset);
                data.writeLong(payload_size);
                data.writeStringArray(headerKeyValuePairs);
                mIBinder.transact(TRANSACTION_PAYLOAD,
                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            } catch (RemoteException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        }
         return REMOTE_EXCEPTION;
    }
    int bind(IBinder callback){
        if ( null != mIBinder ) {
            try {
                Parcel data=Parcel.obtain();
                Parcel reply=Parcel.obtain();
                data.writeInterfaceToken(UPDATE_TOKEN);
                data.writeStrongBinder(callback);
                mIBinder.transact(TRANSACTION_BIND,
                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            } catch (RemoteException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        }
        return REMOTE_EXCEPTION;
    }
    public int suspend(){
        if ( null != mIBinder ) {
            try {
                Parcel data=Parcel.obtain();
                Parcel reply=Parcel.obtain();
                data.writeInterfaceToken(UPDATE_TOKEN);
                mIBinder.transact(TRANSACTION_SUSPEND,
                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            } catch (RemoteException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        }
        return REMOTE_EXCEPTION;
    }
    public int resume(){
        if ( null != mIBinder ) {
            try {
                Parcel data=Parcel.obtain();
                Parcel reply=Parcel.obtain();
                data.writeInterfaceToken(UPDATE_TOKEN);
                mIBinder.transact(TRANSACTION_RESUME,
                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            } catch (RemoteException e) {
                e.printStackTrace();
            }

        }
        return REMOTE_EXCEPTION;
    }
    public int cancel(){
        if ( null != mIBinder ) {
            try {
                Parcel data=Parcel.obtain();
                Parcel reply=Parcel.obtain();
                data.writeInterfaceToken(UPDATE_TOKEN);
                mIBinder.transact(TRANSACTION_CANCEL,
                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            } catch (RemoteException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        }
        return REMOTE_EXCEPTION;
    }
    public int resetStatus(){
        if ( null != mIBinder ) {
            try {
                Parcel data=Parcel.obtain();
                Parcel reply=Parcel.obtain();
                data.writeInterfaceToken(UPDATE_TOKEN);
                mIBinder.transact(TRANSACTION_RESET_STATUS,
                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            } catch (RemoteException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }

        }
        return REMOTE_EXCEPTION;
    }
}
