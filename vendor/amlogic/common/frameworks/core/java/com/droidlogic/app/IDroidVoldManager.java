/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC IDroidVoldManager
 */

package com.droidlogic.app;

public interface IDroidVoldManager extends android.os.IInterface {
    /** Local-side IPC implementation stub class. */
    public static abstract class Stub extends android.os.Binder implements com.droidlogic.app.IDroidVoldManager {
        private static final java.lang.String DESCRIPTOR = "com.droidlogic.app.IDroidVoldManager";

        /** Construct the stub at attach it to the interface. */
        public Stub() {
            this.attachInterface(this, DESCRIPTOR);
        }

        /**
         * Cast an IBinder object into an com.droidlogic.app.IDroidVoldManager interface,
         * generating a proxy if needed.
         */

        public static com.droidlogic.app.IDroidVoldManager asInterface(android.os.IBinder obj) {
            if ((obj == null)) {
                return null;
            }

            android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
            if (((iin != null) && (iin instanceof com.droidlogic.app.IDroidVoldManager))) {
                return ((com.droidlogic.app.IDroidVoldManager)iin);
            }
            return new com.droidlogic.app.IDroidVoldManager.Stub.Proxy(obj);
        }

        @Override
        public android.os.IBinder asBinder() {
            return this;
        }

        @Override
        public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags)
                throws android.os.RemoteException {
            switch (code) {
                case INTERFACE_TRANSACTION: {
                    reply.writeString(DESCRIPTOR);
                    return true;
                }

                case TRANSACTION_formatVolume: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _arg0;
                    _arg0 = data.readString();

                    int _result = this.formatVolume(_arg0);
                    reply.writeNoException();
                    reply.writeInt(_result);
                    return true;
                }

                case TRANSACTION_shutdown: {
                    data.enforceInterface(DESCRIPTOR);
                    /*android.os.storage.IStorageShutdownObserver _arg0;
                    _arg0 = android.os.storage.IStorageShutdownObserver.Stub.asInterface(data.readStrongBinder());
                    this.shutdown(_arg0);
                    reply.writeNoException();*/
                    return true;
                }

                case TRANSACTION_getVolumeList: {
                    data.enforceInterface(DESCRIPTOR);
                    int _arg0;
                    _arg0 = data.readInt();
                    java.lang.String _arg1;
                    _arg1 = data.readString();
                    int _arg2;
                    _arg2 = data.readInt();
                    android.os.storage.StorageVolume[] _result = this.getVolumeList(_arg0, _arg1, _arg2);
                    reply.writeNoException();
                    reply.writeTypedArray(_result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
                    return true;
                }

                case TRANSACTION_mkdirs: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _arg0;
                    _arg0 = data.readString();
                    java.lang.String _arg1;
                    _arg1 = data.readString();
                    int _result = this.mkdirs(_arg0, _arg1);
                    reply.writeNoException();
                    reply.writeInt(_result);
                    return true;
                }

                case TRANSACTION_getDisks: {
                    data.enforceInterface(DESCRIPTOR);
                    /*android.os.storage.DiskInfo[] _result = this.getDisks();
                    reply.writeNoException();
                    reply.writeTypedArray(_result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);*/
                    return true;
                }


                case TRANSACTION_getVolumes: {
                    data.enforceInterface(DESCRIPTOR);
                    int _arg0;
                    _arg0 = data.readInt();
                    /*android.os.storage.VolumeInfo[] _result = this.getVolumes(_arg0);
                    reply.writeNoException();
                    reply.writeTypedArray(_result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);*/
                    return true;
                }


                case TRANSACTION_getVolumeRecords: {
                    data.enforceInterface(DESCRIPTOR);
                    int _arg0;
                    _arg0 = data.readInt();
                    /*android.os.storage.VolumeRecord[] _result = this.getVolumeRecords(_arg0);
                    reply.writeNoException();
                    reply.writeTypedArray(_result, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);*/
                    return true;
                }

                case TRANSACTION_mount: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _arg0;
                    _arg0 = data.readString();
                    this.mount(_arg0);
                    reply.writeNoException();
                    return true;
                }

                case TRANSACTION_unmount: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _arg0;
                    _arg0 = data.readString();
                    this.unmount(_arg0);
                    reply.writeNoException();
                    return true;
                }

                case TRANSACTION_format: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _arg0;
                    _arg0 = data.readString();
                    this.format(_arg0);
                    reply.writeNoException();
                    return true;
                }

                case TRANSACTION_getVolumeInfoDiskFlags: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _arg0;
                    _arg0 = data.readString();
                    int _result = this.getVolumeInfoDiskFlags(_arg0);
                    reply.writeNoException();
                    reply.writeInt(_result);
                    return true;
                }
            }

            return super.onTransact(code, data, reply, flags);
        }

        private static class Proxy implements com.droidlogic.app.IDroidVoldManager {
            private android.os.IBinder mRemote;

            Proxy(android.os.IBinder remote) {
                mRemote = remote;
            }

            @Override
            public android.os.IBinder asBinder() {
                return mRemote;
            }

            public java.lang.String getInterfaceDescriptor() {
                return DESCRIPTOR;
            }

            @Override
            public int formatVolume(java.lang.String mountPoint) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                int _result;

                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeString(mountPoint);
                    mRemote.transact(Stub.TRANSACTION_formatVolume, _data, _reply, 0);
                    _reply.readException();
                    _result = _reply.readInt();
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }

                return _result;
            }
/*
            @Override
            public void shutdown(android.os.storage.IStorageShutdownObserver observer) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();

                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeStrongBinder((((observer!=null))?(observer.asBinder()):(null)));
                    mRemote.transact(Stub.TRANSACTION_shutdown, _data, _reply, 0);
                    _reply.readException();
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }
            }
*/

            @Override
            public android.os.storage.StorageVolume[] getVolumeList(int uid, java.lang.String packageName, int flags)
                    throws android.os.RemoteException {
                    android.os.Parcel _data = android.os.Parcel.obtain();
                    android.os.Parcel _reply = android.os.Parcel.obtain();
                    android.os.storage.StorageVolume[] _result;

                    try {
                        _data.writeInterfaceToken(DESCRIPTOR);
                        _data.writeInt(uid);
                        _data.writeString(packageName);
                        _data.writeInt(flags);
                        mRemote.transact(Stub.TRANSACTION_getVolumeList, _data, _reply, 0);
                        _reply.readException();
                        _result = _reply.createTypedArray(android.os.storage.StorageVolume.CREATOR);
                    } finally {
                        _reply.recycle();
                        _data.recycle();
                    }

                    return _result;
            }

            @Override
            public int mkdirs(java.lang.String callingPkg, java.lang.String path) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                int _result;

                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeString(callingPkg);
                    _data.writeString(path);
                    mRemote.transact(Stub.TRANSACTION_mkdirs, _data, _reply, 0);
                    _reply.readException();
                    _result = _reply.readInt();
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }

                return _result;
            }
/*
            @Override
            public android.os.storage.DiskInfo[] getDisks() throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                android.os.storage.DiskInfo[] _result;
                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    mRemote.transact(Stub.TRANSACTION_getDisks, _data, _reply, 0);
                    _reply.readException();
                    _result = _reply.createTypedArray(android.os.storage.DiskInfo.CREATOR);
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }

                return _result;
            }

            @Override
            public android.os.storage.VolumeInfo[] getVolumes(int flags) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                android.os.storage.VolumeInfo[] _result;
                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeInt(flags);
                    mRemote.transact(Stub.TRANSACTION_getVolumes, _data, _reply, 0);
                    _reply.readException();
                    _result = _reply.createTypedArray(android.os.storage.VolumeInfo.CREATOR);
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }

                return _result;
            }

            @Override
            public android.os.storage.VolumeRecord[] getVolumeRecords(int flags) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                android.os.storage.VolumeRecord[] _result;

                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeInt(flags);
                    mRemote.transact(Stub.TRANSACTION_getVolumeRecords, _data, _reply, 0);
                    _reply.readException();
                    _result = _reply.createTypedArray(android.os.storage.VolumeRecord.CREATOR);
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }

                return _result;
            }
*/
            @Override
            public void mount(java.lang.String volId) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeString(volId);
                    mRemote.transact(Stub.TRANSACTION_mount, _data, _reply, 0);
                    _reply.readException();
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }
            }

            @Override
            public void unmount(java.lang.String volId) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeString(volId);
                    mRemote.transact(Stub.TRANSACTION_unmount, _data, _reply, 0);
                    _reply.readException();
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }
            }

            @Override
            public void format(java.lang.String volId) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeString(volId);
                    mRemote.transact(Stub.TRANSACTION_format, _data, _reply, 0);
                    _reply.readException();
                } finally {
                    _reply.recycle();
                    _data.recycle();
                }
            }
            @Override public int getVolumeInfoDiskFlags(java.lang.String mountPoint) throws android.os.RemoteException {
                android.os.Parcel _data = android.os.Parcel.obtain();
                android.os.Parcel _reply = android.os.Parcel.obtain();
                int _result;
                try {
                    _data.writeInterfaceToken(DESCRIPTOR);
                    _data.writeString(mountPoint);
                    mRemote.transact(Stub.TRANSACTION_getVolumeInfoDiskFlags, _data, _reply, 0);
                    _reply.readException();
                    _result = _reply.readInt();
                }
                finally {
                    _reply.recycle();
                    _data.recycle();
                }
                return _result;
            }
        }

        static final int TRANSACTION_formatVolume = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
        static final int TRANSACTION_shutdown = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
        static final int TRANSACTION_getVolumeList = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
        static final int TRANSACTION_mkdirs = (android.os.IBinder.FIRST_CALL_TRANSACTION + 3);
        static final int TRANSACTION_getDisks = (android.os.IBinder.FIRST_CALL_TRANSACTION + 4);
        static final int TRANSACTION_getVolumes = (android.os.IBinder.FIRST_CALL_TRANSACTION + 5);
        static final int TRANSACTION_getVolumeRecords = (android.os.IBinder.FIRST_CALL_TRANSACTION + 6);
        static final int TRANSACTION_mount = (android.os.IBinder.FIRST_CALL_TRANSACTION + 7);
        static final int TRANSACTION_unmount = (android.os.IBinder.FIRST_CALL_TRANSACTION + 8);
        static final int TRANSACTION_format = (android.os.IBinder.FIRST_CALL_TRANSACTION + 9);
        static final int TRANSACTION_getVolumeInfoDiskFlags = (android.os.IBinder.FIRST_CALL_TRANSACTION + 10);
    }

    public int formatVolume(java.lang.String mountPoint) throws android.os.RemoteException;
    //public void shutdown(android.os.storage.IStorageShutdownObserver observer) throws android.os.RemoteException;
    public android.os.storage.StorageVolume[] getVolumeList(int uid, java.lang.String packageName, int flags) throws android.os.RemoteException;
    public int mkdirs(java.lang.String callingPkg, java.lang.String path) throws android.os.RemoteException;
    //public android.os.storage.DiskInfo[] getDisks() throws android.os.RemoteException;
    //public android.os.storage.VolumeInfo[] getVolumes(int flags) throws android.os.RemoteException;
    //public android.os.storage.VolumeRecord[] getVolumeRecords(int flags) throws android.os.RemoteException;
    public void mount(java.lang.String volId) throws android.os.RemoteException;
    public void unmount(java.lang.String volId) throws android.os.RemoteException;
    public void format(java.lang.String volId) throws android.os.RemoteException;
    public int getVolumeInfoDiskFlags(java.lang.String mountPoint) throws android.os.RemoteException;
}
