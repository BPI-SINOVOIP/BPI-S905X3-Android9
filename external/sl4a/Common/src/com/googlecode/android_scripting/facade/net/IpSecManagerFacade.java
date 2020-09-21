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

package com.googlecode.android_scripting.facade.net;

import android.app.Service;
import android.content.Context;
import android.net.IpSecAlgorithm;
import android.net.IpSecManager;
import android.net.IpSecManager.ResourceUnavailableException;
import android.net.IpSecManager.SecurityParameterIndex;
import android.net.IpSecManager.SpiUnavailableException;
import android.net.IpSecManager.UdpEncapsulationSocket;
import android.net.IpSecTransform;
import android.net.IpSecTransform.Builder;
import android.net.NetworkUtils;

import com.google.common.io.BaseEncoding;
import com.googlecode.android_scripting.Log;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcOptional;
import com.googlecode.android_scripting.rpc.RpcParameter;

import java.io.FileDescriptor;
import java.io.IOException;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.util.HashMap;

/*
 * Access IpSecManager functions.
 */
public class IpSecManagerFacade extends RpcReceiver {

    private final IpSecManager mIpSecManager;
    private final Service mService;
    private final Context mContext;
    private static HashMap<String, SecurityParameterIndex> sSpiHashMap =
            new HashMap<String, SecurityParameterIndex>();
    private static HashMap<String, IpSecTransform> sTransformHashMap =
            new HashMap<String, IpSecTransform>();
    private static HashMap<String, UdpEncapsulationSocket> sUdpEncapHashMap =
            new HashMap<String, UdpEncapsulationSocket>();

    public IpSecManagerFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mContext = mService.getBaseContext();
        mIpSecManager = (IpSecManager) mService.getSystemService(Context.IPSEC_SERVICE);
    }

    private IpSecTransform createTransportModeTransform(
            String encAlgo,
            byte[] cryptKey,
            String authAlgo,
            byte[] authKey,
            Integer truncBits,
            SecurityParameterIndex spi,
            InetAddress addr,
            UdpEncapsulationSocket udpEncapSocket) {
        Builder builder = new Builder(mContext);
        builder = builder.setEncryption(new IpSecAlgorithm(encAlgo, cryptKey));
        builder =
                builder.setAuthentication(
                        new IpSecAlgorithm(authAlgo, authKey, truncBits.intValue()));
        if (udpEncapSocket != null) {
            builder = builder.setIpv4Encapsulation(udpEncapSocket, udpEncapSocket.getPort());
        }
        try {
            return builder.buildTransportModeTransform(addr, spi);
        } catch (SpiUnavailableException | IOException | ResourceUnavailableException e) {
            Log.e("IpSec: Cannot create Transport mode transform" + e.toString());
        }
        return null;
    }

    private SecurityParameterIndex allocateSpi(InetAddress inetAddr) {
        try {
            return mIpSecManager.allocateSecurityParameterIndex(inetAddr);
        } catch (ResourceUnavailableException e) {
            Log.e("IpSec: Reserve SPI failure " + e.toString());
        }
        return null;
    }

    private SecurityParameterIndex allocateSpi(InetAddress inetAddr, int requestedSpi) {
        try {
            return mIpSecManager.allocateSecurityParameterIndex(inetAddr, requestedSpi);
        } catch (SpiUnavailableException | ResourceUnavailableException e) {
            Log.e("IpSec: Reserve SPI failure " + e.toString());
        }
        return null;
    }

    private UdpEncapsulationSocket openUdpEncapSocket() {
        UdpEncapsulationSocket udpEncapSocket = null;
        try {
            return mIpSecManager.openUdpEncapsulationSocket();
        } catch (ResourceUnavailableException | IOException e) {
            Log.e("IpSec: Failed to open udp encap socket " + e.toString());
        }
        return null;
    }

    private UdpEncapsulationSocket openUdpEncapSocket(int port) {
        try {
            return mIpSecManager.openUdpEncapsulationSocket(port);
        } catch (ResourceUnavailableException | IOException e) {
            Log.e("IpSec: Failed to open udp encap socket " + e.toString());
        }
        return null;
    }

    private String getSpiId(SecurityParameterIndex spi) {
        return "SPI:" + spi.hashCode();
    }

    private String getTransformId(IpSecTransform transform) {
        return "TRANSFORM:" + transform.hashCode();
    }

    private String getUdpEncapSockId(UdpEncapsulationSocket socket) {
        return "UDPENCAPSOCK:" + socket.hashCode();
    }

    /**
     * Apply transport mode transform to FileDescriptor
     * @param socketFd : Hash key of FileDescriptor object
     * @param direction : In or Out direction to apply transform to
     * @param id : Hash key of the transform
     * @return True if transform is applied successfully
     */
    @Rpc(description = "Apply transport mode transform to FileDescriptor", returns = "True/False")
    public Boolean ipSecApplyTransportModeTransformFileDescriptor(
            String socketFd,
            Integer direction,
            String id) {
        if (socketFd == null) {
            Log.e("IpSec: Received null FileDescriptor key");
            return false;
        }
        FileDescriptor fd = SocketFacade.getFileDescriptor(socketFd);
        IpSecTransform transform = sTransformHashMap.get(id);
        if (transform == null) {
            Log.e("IpSec: Transform does not exist for the requested id");
            return false;
        }
        try {
            mIpSecManager.applyTransportModeTransform(fd, direction.intValue(), transform);
        } catch (IOException e) {
            Log.e("IpSec: Cannot apply transform to socket " + e.toString());
            return false;
        }
        return true;
    }

    /**
     * Remove transport mode transform from a FileDescriptor
     * @param socketFd : Hash key of FileDescriptor object
     * @returns True if transform is removed successfully
     */
    @Rpc(description = "Remove transport mode transform to FileDescriptor", returns = "True/False")
    public Boolean ipSecRemoveTransportModeTransformsFileDescriptor(String socketFd) {
        if (socketFd == null) {
            Log.e("IpSec: Received null FileDescriptor key");
            return false;
        }
        FileDescriptor fd = SocketFacade.getFileDescriptor(socketFd);
        try {
            mIpSecManager.removeTransportModeTransforms(fd);
            return true;
        } catch (IOException e) {
            Log.e("IpSec: Failed to remove transform " + e.toString());
        }
        return false;
    }

    /**
     * Apply transport mode transform to DatagramSocket
     * @param socketId : Hash key of DatagramSocket
     * @param direction : In or Out direction to apply transform to
     * @param transformId : Hash key of Transform to apply
     * @return True if transform is applied successfully
     */
    @Rpc(description = "Apply transport mode transform to DatagramSocket", returns = "True/False")
    public Boolean ipSecApplyTransportModeTransformDatagramSocket(
            String socketId,
            Integer direction,
            String transformId) {
        if (socketId == null) {
            Log.e("IpSec: Received null DatagramSocket key");
            return false;
        }
        DatagramSocket socket = SocketFacade.getDatagramSocket(socketId);
        IpSecTransform transform = sTransformHashMap.get(transformId);
        if (transform == null) {
            Log.e("IpSec: Transform does not exist for the requested id");
            return false;
        }
        try {
            mIpSecManager.applyTransportModeTransform(socket, direction.intValue(), transform);
        } catch (IOException e) {
            Log.e("IpSec: Cannot apply transform to socket " + e.toString());
            return false;
        }
        return true;
    }

    /**
     * Remove transport mode transform from DatagramSocket
     * @param socketId : Hash key of DatagramSocket
     * @return True if removing transform is successful
     */
    @Rpc(description = "Remove transport mode tranform from DatagramSocket", returns = "True/False")
    public Boolean ipSecRemoveTransportModeTransformsDatagramSocket(String socketId) {
        if (socketId == null) {
            Log.e("IpSec: Received null DatagramSocket key");
            return false;
        }
        DatagramSocket socket = SocketFacade.getDatagramSocket(socketId);
        try {
            mIpSecManager.removeTransportModeTransforms(socket);
            return true;
        } catch (IOException e) {
            Log.e("IpSec: Failed to remove transform " + e.toString());
        }
        return false;
    }

    /**
     * Apply transport mode transform to DatagramSocket
     * @param socketId : Hash key of Socket
     * @param direction : In or Out direction to apply transform to
     * @param transformId : Hash key of Transform to apply
     * @return True if transform is applied successfully
     */
    @Rpc(description = "Apply transport mode transform to Socket", returns = "True/False")
    public Boolean ipSecApplyTransportModeTransformSocket(
            String socketId,
            Integer direction,
            String transformId) {
        if (socketId == null) {
            Log.e("IpSec: Received null Socket key");
            return false;
        }
        Socket socket = SocketFacade.getSocket(socketId);
        IpSecTransform transform = sTransformHashMap.get(transformId);
        if (transform == null) {
            Log.e("IpSec: Transform does not exist for the requested id");
            return false;
        }
        try {
            mIpSecManager.applyTransportModeTransform(socket, direction.intValue(), transform);
        } catch (IOException e) {
            Log.e("IpSec: Cannot apply transform to socket " + e.toString());
            return false;
        }
        return true;
    }

    /**
     * Remove transport mode transform from Socket
     * @param socketId : Hash key of DatagramSocket
     * @return True if removing transform is successful
     */
    @Rpc(description = "Remove transport mode tranform from Socket", returns = "True/False")
    public Boolean ipSecRemoveTransportModeTransformsSocket(String socketId) {
        if (socketId == null) {
            Log.e("IpSec: Received null Socket key");
            return false;
        }
        Socket socket = SocketFacade.getSocket(socketId);
        try {
            mIpSecManager.removeTransportModeTransforms(socket);
            return true;
        } catch (IOException e) {
            Log.e("IpSec: Failed to remove transform " + e.toString());
        }
        return false;
    }

    @Rpc(description = "Create a transform mode transform", returns = "Hash of transform object")
    public String ipSecCreateTransportModeTransform(
            String encAlgo,
            String cryptKeyHex,
            String authAlgo,
            String authKeyHex,
            Integer truncBits,
            String spiId,
            String addr,
            String udpEncapSockId) {
        IpSecTransform transform = null;
        InetAddress inetAddr = NetworkUtils.numericToInetAddress(addr);
        UdpEncapsulationSocket udpEncapSocket = sUdpEncapHashMap.get(udpEncapSockId);
        SecurityParameterIndex spi = sSpiHashMap.get(spiId);
        if (spi == null) {
            Log.e("IpSec: SPI does not exist for the requested spiId");
            return null;
        }
        byte[] cryptKey = BaseEncoding.base16().decode(cryptKeyHex.toUpperCase());
        byte[] authKey = BaseEncoding.base16().decode(authKeyHex.toUpperCase());
        transform = createTransportModeTransform(encAlgo, cryptKey, authAlgo, authKey, truncBits,
                spi, inetAddr, udpEncapSocket);
        if (transform == null) return null;
        String id = getTransformId(transform);
        sTransformHashMap.put(id, transform);
        return id;
    }

    @Rpc(description = "Get transform status", returns = "True if transform exists")
    public Boolean ipSecGetTransformStatus(String id) {
        IpSecTransform transform = sTransformHashMap.get(id);
        if (transform == null) {
            Log.e("IpSec: Transform does not exist for the requested id");
            return false;
        }
        return true;
    }

    @Rpc(description = "Destroy transport mode transform")
    public void ipSecDestroyTransportModeTransform(String id) {
        IpSecTransform transform = sTransformHashMap.get(id);
        if (transform == null) {
            Log.e("IpSec: Transform does not exist for the requested id");
            return;
        }
        transform.close();
        sTransformHashMap.remove(id);
    }

    @Rpc(description = "Open UDP encap socket", returns = "Hash of UDP encap socket object")
    public String ipSecOpenUdpEncapsulationSocket(
            @RpcParameter(name = "port") @RpcOptional Integer port) {
        UdpEncapsulationSocket udpEncapSocket = null;
        if (port == null) {
            udpEncapSocket = openUdpEncapSocket();
        } else {
            udpEncapSocket = openUdpEncapSocket(port.intValue());
        }
        if (udpEncapSocket == null) return null;
        String id = getUdpEncapSockId(udpEncapSocket);
        sUdpEncapHashMap.put(id, udpEncapSocket);
        return id;
    }

    @Rpc(description = "Close UDP encapsulation socket", returns = "True if socket is closed")
    public Boolean ipSecCloseUdpEncapsulationSocket(String id) {
        try {
            UdpEncapsulationSocket udpEncapSocket = sUdpEncapHashMap.get(id);
            udpEncapSocket.close();
            sUdpEncapHashMap.remove(id);
            return true;
        } catch (IOException e) {
            Log.e("IpSec: Failed to close udp encap socket " + e.toString());
        }
        return false;
    }

    @Rpc(description = "Allocate a Security Parameter Index", returns = "Hash of SPI object")
    public String ipSecAllocateSecurityParameterIndex(
            @RpcParameter(name = "addr") String addr,
            @RpcParameter(name = "requestedSpi") @RpcOptional Integer requestedSpi) {
        InetAddress inetAddr = NetworkUtils.numericToInetAddress(addr);
        SecurityParameterIndex spi = null;
        if (requestedSpi == null) {
            spi = allocateSpi(inetAddr);
        } else {
            spi = allocateSpi(inetAddr, requestedSpi.intValue());
        }
        if (spi == null) return null;
        String id = getSpiId(spi);
        sSpiHashMap.put(id, spi);
        return id;
    }

    @Rpc(description = "Get Security Parameter Index", returns = "Returns SPI value")
    public Integer ipSecGetSecurityParameterIndex(String id) {
        SecurityParameterIndex spi = sSpiHashMap.get(id);
        if (spi == null) {
            Log.d("IpSec: SPI does not exist for the requested id");
            return 0;
        }
        return spi.getSpi();
    }

    @Rpc(description = "Release a Security Parameter Index")
    public void ipSecReleaseSecurityParameterIndex(String id) {
        SecurityParameterIndex spi = sSpiHashMap.get(id);
        if (spi == null) {
            Log.d("IpSec: SPI does not exist for the requested id");
            return;
        }
        spi.close();
        sSpiHashMap.remove(id);
    }

    @Override
    public void shutdown() {}
}
