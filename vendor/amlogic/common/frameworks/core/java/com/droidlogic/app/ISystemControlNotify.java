/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC ISystemControlNotify
 */

package com.droidlogic.app;

import android.os.Binder;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.IInterface;
import android.os.Parcel;

/**
 * Callback to find out when we have finished stopping a user.
 * {@hide}
 */
public interface ISystemControlNotify extends IInterface
{
    /** Local-side IPC implementation stub class. */
    public static abstract class Stub extends Binder implements ISystemControlNotify {
        private static final java.lang.String DESCRIPTOR = "com.droidlogic.app.ISystemControlNotify";

        /** Construct the stub at attach it to the interface. */
        public Stub()
        {
            this.attachInterface(this, DESCRIPTOR);
        }

        /**
         * Cast an IBinder object into an com.droidlogic.app.ISystemControlNotify interface,
         * generating a proxy if needed.
         */
        public static ISystemControlNotify asInterface(IBinder obj)
        {
            if ((obj == null)) {
                return null;
            }

            IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
            if (((iin != null) && (iin instanceof ISystemControlNotify))) {
                return ((ISystemControlNotify)iin);
            }
            return new ISystemControlNotify.Stub.Proxy(obj);
        }

        @Override
        public IBinder asBinder() {
            return this;
        }

        @Override
        public boolean onTransact(int code, Parcel data, Parcel reply, int flags) throws RemoteException {
            switch (code) {
                case INTERFACE_TRANSACTION:
                {
                    reply.writeString(DESCRIPTOR);
                    return true;
                }
                case TRANSACT_ONEVENT:
                {
                    data.enforceInterface(DESCRIPTOR);
                    int _arg0 = data.readInt();
                    this.onEvent(_arg0);
                    reply.writeNoException();
                    return true;
                }
            }
            return super.onTransact(code, data, reply, flags);
        }

        private static class Proxy implements ISystemControlNotify
        {
            private IBinder mRemote;

            Proxy(IBinder remote) {
                mRemote = remote;
            }

            @Override
            public IBinder asBinder() {
                return mRemote;
            }

            public String getInterfaceDescriptor() {
                return DESCRIPTOR;
            }

            @Override
            public void onEvent(int event) throws RemoteException {
                Parcel _data = Parcel.obtain();
                Parcel _reply = Parcel.obtain();

                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeInt(event);
                    mRemote.transact(Stub.TRANSACT_ONEVENT, _data, _reply, 0);
                    _reply.readException();
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }
            }
        }

        static final int TRANSACT_ONEVENT = (IBinder.FIRST_CALL_TRANSACTION + 0);
    }

    public void onEvent(int event) throws RemoteException;
}
