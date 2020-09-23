/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

/*
 * This file is auto-generated.  DO NOT MODIFY.
 * Original file: frameworks/base/pppoe/java/android/net/pppoe/IPppoeManager.aidl
 */
package com.droidlogic.pppoe;
public interface IPppoeManager extends android.os.IInterface
{
/** Local-side IPC implementation stub class. */
public static abstract class Stub extends android.os.Binder implements com.droidlogic.pppoe.IPppoeManager
{
private static final java.lang.String DESCRIPTOR = "com.droidlogic.pppoe.IPppoeManager";
/** Construct the stub at attach it to the interface. */
public Stub()
{
this.attachInterface(this, DESCRIPTOR);
}
/**
 * Cast an IBinder object into an android.net.pppoe.IPppoeManager interface,
 * generating a proxy if needed.
 */
public static com.droidlogic.pppoe.IPppoeManager asInterface(android.os.IBinder obj)
{
if ((obj == null)) {
return null;
}
android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
if (((iin != null) && (iin instanceof com.droidlogic.pppoe.IPppoeManager))) {
return ((com.droidlogic.pppoe.IPppoeManager)iin);
}
return new com.droidlogic.pppoe.IPppoeManager.Stub.Proxy(obj);
}
@Override public android.os.IBinder asBinder()
{
return this;
}
@Override public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException
{
switch (code)
{
case INTERFACE_TRANSACTION:
{
reply.writeString(DESCRIPTOR);
return true;
}
case TRANSACTION_getDeviceNameList:
{
data.enforceInterface(DESCRIPTOR);
java.lang.String[] _result = this.getDeviceNameList();
reply.writeNoException();
reply.writeStringArray(_result);
return true;
}
case TRANSACTION_setPppoeState:
{
data.enforceInterface(DESCRIPTOR);
int _arg0;
_arg0 = data.readInt();
this.setPppoeState(_arg0);
reply.writeNoException();
return true;
}
case TRANSACTION_getPppoeState:
{
data.enforceInterface(DESCRIPTOR);
int _result = this.getPppoeState();
reply.writeNoException();
reply.writeInt(_result);
return true;
}
case TRANSACTION_UpdatePppoeDevInfo:
{
data.enforceInterface(DESCRIPTOR);
com.droidlogic.pppoe.PppoeDevInfo _arg0;
if ((0 != data.readInt())) {
_arg0 = com.droidlogic.pppoe.PppoeDevInfo.CREATOR.createFromParcel(data);
}
else {
_arg0 = null;
}
this.UpdatePppoeDevInfo(_arg0);
reply.writeNoException();
return true;
}
case TRANSACTION_isPppoeConfigured:
{
data.enforceInterface(DESCRIPTOR);
boolean _result = this.isPppoeConfigured();
reply.writeNoException();
reply.writeInt(((_result)?(1):(0)));
return true;
}
case TRANSACTION_getSavedPppoeConfig:
{
data.enforceInterface(DESCRIPTOR);
com.droidlogic.pppoe.PppoeDevInfo _result = this.getSavedPppoeConfig();
reply.writeNoException();
if ((_result != null)) {
reply.writeInt(1);
_result.writeToParcel(reply, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
}
else {
reply.writeInt(0);
}
return true;
}
case TRANSACTION_getTotalInterface:
{
data.enforceInterface(DESCRIPTOR);
int _result = this.getTotalInterface();
reply.writeNoException();
reply.writeInt(_result);
return true;
}
case TRANSACTION_setPppoeMode:
{
data.enforceInterface(DESCRIPTOR);
java.lang.String _arg0;
_arg0 = data.readString();
this.setPppoeMode(_arg0);
reply.writeNoException();
return true;
}
case TRANSACTION_isPppoeDeviceUp:
{
data.enforceInterface(DESCRIPTOR);
boolean _result = this.isPppoeDeviceUp();
reply.writeNoException();
reply.writeInt(((_result)?(1):(0)));
return true;
}
case TRANSACTION_getDhcpInfo:
{
data.enforceInterface(DESCRIPTOR);
android.net.DhcpInfo _result = this.getDhcpInfo();
reply.writeNoException();
if ((_result != null)) {
reply.writeInt(1);
_result.writeToParcel(reply, android.os.Parcelable.PARCELABLE_WRITE_RETURN_VALUE);
}
else {
reply.writeInt(0);
}
return true;
}
}
return super.onTransact(code, data, reply, flags);
}
private static class Proxy implements com.droidlogic.pppoe.IPppoeManager
{
private android.os.IBinder mRemote;
Proxy(android.os.IBinder remote)
{
mRemote = remote;
}
@Override public android.os.IBinder asBinder()
{
return mRemote;
}
public java.lang.String getInterfaceDescriptor()
{
return DESCRIPTOR;
}
@Override public java.lang.String[] getDeviceNameList() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
java.lang.String[] _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_getDeviceNameList, _data, _reply, 0);
_reply.readException();
_result = _reply.createStringArray();
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
@Override public void setPppoeState(int state) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
_data.writeInt(state);
mRemote.transact(Stub.TRANSACTION_setPppoeState, _data, _reply, 0);
_reply.readException();
}
finally {
_reply.recycle();
_data.recycle();
}
}
@Override public int getPppoeState() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
int _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_getPppoeState, _data, _reply, 0);
_reply.readException();
_result = _reply.readInt();
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
@Override public void UpdatePppoeDevInfo(com.droidlogic.pppoe.PppoeDevInfo info) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
if ((info != null)) {
_data.writeInt(1);
info.writeToParcel(_data, 0);
}
else {
_data.writeInt(0);
}
mRemote.transact(Stub.TRANSACTION_UpdatePppoeDevInfo, _data, _reply, 0);
_reply.readException();
}
finally {
_reply.recycle();
_data.recycle();
}
}
@Override public boolean isPppoeConfigured() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
boolean _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_isPppoeConfigured, _data, _reply, 0);
_reply.readException();
_result = (0!=_reply.readInt());
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
@Override public com.droidlogic.pppoe.PppoeDevInfo getSavedPppoeConfig() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
com.droidlogic.pppoe.PppoeDevInfo _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_getSavedPppoeConfig, _data, _reply, 0);
_reply.readException();
if ((0 != _reply.readInt())) {
_result = com.droidlogic.pppoe.PppoeDevInfo.CREATOR.createFromParcel(_reply);
}
else {
_result = null;
}
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
@Override public int getTotalInterface() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
int _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_getTotalInterface, _data, _reply, 0);
_reply.readException();
_result = _reply.readInt();
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
@Override public void setPppoeMode(java.lang.String mode) throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
try {
_data.writeInterfaceToken(DESCRIPTOR);
_data.writeString(mode);
mRemote.transact(Stub.TRANSACTION_setPppoeMode, _data, _reply, 0);
_reply.readException();
}
finally {
_reply.recycle();
_data.recycle();
}
}
@Override public boolean isPppoeDeviceUp() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
boolean _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_isPppoeDeviceUp, _data, _reply, 0);
_reply.readException();
_result = (0!=_reply.readInt());
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
@Override public android.net.DhcpInfo getDhcpInfo() throws android.os.RemoteException
{
android.os.Parcel _data = android.os.Parcel.obtain();
android.os.Parcel _reply = android.os.Parcel.obtain();
android.net.DhcpInfo _result;
try {
_data.writeInterfaceToken(DESCRIPTOR);
mRemote.transact(Stub.TRANSACTION_getDhcpInfo, _data, _reply, 0);
_reply.readException();
if ((0 != _reply.readInt())) {
_result = android.net.DhcpInfo.CREATOR.createFromParcel(_reply);
}
else {
_result = null;
}
}
finally {
_reply.recycle();
_data.recycle();
}
return _result;
}
}
static final int TRANSACTION_getDeviceNameList = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
static final int TRANSACTION_setPppoeState = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
static final int TRANSACTION_getPppoeState = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
static final int TRANSACTION_UpdatePppoeDevInfo = (android.os.IBinder.FIRST_CALL_TRANSACTION + 3);
static final int TRANSACTION_isPppoeConfigured = (android.os.IBinder.FIRST_CALL_TRANSACTION + 4);
static final int TRANSACTION_getSavedPppoeConfig = (android.os.IBinder.FIRST_CALL_TRANSACTION + 5);
static final int TRANSACTION_getTotalInterface = (android.os.IBinder.FIRST_CALL_TRANSACTION + 6);
static final int TRANSACTION_setPppoeMode = (android.os.IBinder.FIRST_CALL_TRANSACTION + 7);
static final int TRANSACTION_isPppoeDeviceUp = (android.os.IBinder.FIRST_CALL_TRANSACTION + 8);
static final int TRANSACTION_getDhcpInfo = (android.os.IBinder.FIRST_CALL_TRANSACTION + 9);
}
public java.lang.String[] getDeviceNameList() throws android.os.RemoteException;
public void setPppoeState(int state) throws android.os.RemoteException;
public int getPppoeState() throws android.os.RemoteException;
public void UpdatePppoeDevInfo(com.droidlogic.pppoe.PppoeDevInfo info) throws android.os.RemoteException;
public boolean isPppoeConfigured() throws android.os.RemoteException;
public com.droidlogic.pppoe.PppoeDevInfo getSavedPppoeConfig() throws android.os.RemoteException;
public int getTotalInterface() throws android.os.RemoteException;
public void setPppoeMode(java.lang.String mode) throws android.os.RemoteException;
public boolean isPppoeDeviceUp() throws android.os.RemoteException;
public android.net.DhcpInfo getDhcpInfo() throws android.os.RemoteException;
}
