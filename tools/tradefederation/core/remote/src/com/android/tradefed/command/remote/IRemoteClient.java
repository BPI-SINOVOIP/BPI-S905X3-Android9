/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tradefed.command.remote;

import java.util.List;

/**
 * Interface for sending remote TradeFederation commands.
 */
public interface IRemoteClient {

    /**
     * Send a 'list devices' request to remote TF
     *
     * @return a list of device serials and their state.
     * @throws RemoteException if command failed
     */
    public List<DeviceDescriptor> sendListDevices() throws RemoteException;

    /**
     * Send an 'allocate device' request to remote TF.
     *
     * @param serial
     * @throws RemoteException
     */
    public void sendAllocateDevice(String serial) throws RemoteException;

    /**
     * Send a 'free previously allocated device' request to remote TF
     *
     * @param serial
     * @throws RemoteException
     */
    public void sendFreeDevice(String serial) throws RemoteException;

    /**
     * Send an 'add command' request to remote TF. This will add the command to the queue of
     * commands waiting for devices, essentially scheduling it for future execution at some point in
     * time. There is no way for a client to get the result of a command added via this API.
     *
     * @see {@link ICommandScheduler#addCommand(String[], long)}
     * @param elapsedTimeMs the total time in ms that the command has been executing for. Used for
     *            scheduling prioritization, and will typically only be non-zero in handover
     *            situations.
     * @param commandArgs the command arguments, in [configname] [option] format
     * @throws RemoteException
     */
    public void sendAddCommand(long elapsedTimeMs, String... commandArgs) throws RemoteException;

    /**
     * Send an 'add command file' request to remote TF.
     *
     * @see {@link ICommandScheduler#addCommandFile()}
     * @param commandFile the file system path to the command file
     * @param extraArgs the list of extra arguments to add to every command parsed from file
     * @throws RemoteException
     */
    void sendAddCommandFile(String commandFile, List<String> extraArgs) throws RemoteException;

    /**
     * Send a 'execute this command on given device' request to remote TF.
     * <p/>
     * Unlike {@link #sendAddCommand(long, String...)}, this is used in cases where the remote
     * client caller is doing their own device allocation, and is instructing TF to directly
     * execute the command on a given device, instead of just scheduling the command for future
     * execution.
     * <p/>
     * {@link #sendGetLastCommandResult(String, ICommandResultHandler)} can be used to get the
     * command result.
     *
     * @param serial the device serial to execute on. Must have already been allocated
     * @param commandArgs the command arguments, in [configname] [option] format
     * @throws RemoteException
     */
    public void sendExecCommand(String serial, String[] commandArgs) throws RemoteException;

    /**
     * Poll the remote TF for the last command result on executed given device.
     *
     * @param serial the device serial to execute on. Must have already been allocated
     * @param handler the {@link ICommandResultHandler} to communicate results to
     * @throws RemoteException
     */
    public void sendGetLastCommandResult(String serial, ICommandResultHandler handler)
            throws RemoteException;

    /**
     * Send a 'close' request to remote TF.
     * <p/>
     * This requests shut down of the remote TF's connection, and it will stop
     * listening for remote requests.
     *
     * @deprecated will be removed in future. Except for handover cases, it should not be possible
     * for a client to shutdown TF's remote manager.
     * @throws RemoteException
     */
    @Deprecated
    public void sendClose() throws RemoteException;

    /**
     * Request to start a handover sequence to another TF.
     *
     * @param port the port of the remote TF to hand over to
     * @throws RemoteException
     */
    public void sendStartHandover(int port) throws RemoteException;

    /**
     * Inform remote TF that handover initiation is complete. Old TF has send all info about devices
     * and commands in use.
     *
     * @throws RemoteException
     */
    public void sendHandoverInitComplete() throws RemoteException;

    /**
     * Inform remote TF that handover sequence is now complete. Old TF has freed all devices and is
     * shutting down.
     *
     * @throws RemoteException
     */
    public void sendHandoverComplete() throws RemoteException;

    /**
     * Close the connection to the {@link RemoteManager}.
     */
    public void close();

}
