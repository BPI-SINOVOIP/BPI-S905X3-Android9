/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.bips.p2p;

import android.net.Uri;

import com.android.bips.BuiltInPrintService;
import com.android.bips.discovery.DiscoveredPrinter;
import com.android.bips.discovery.P2pDiscovery;

import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.InterfaceAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.BitSet;
import java.util.regex.Pattern;

/**
 * Provide tools for conversions and querying P2P status
 */
public class P2pUtils {
    private static final Pattern IPV4_PATTERN =
            Pattern.compile("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
            + "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

    /** Return true if path is known to be on the named network interface */
    static boolean isOnInterface(String interfaceName, Uri path) {
        NetworkInterface networkInterface = toNetworkInterface(interfaceName);
        Inet4Address inet4Address = toInet4Address(path);
        return networkInterface != null && inet4Address != null
                && isOnInterface(networkInterface, inet4Address);
    }

    /** If possible, find the x.x.x.x host part of path, without performing any name resolution */
    private static Inet4Address toInet4Address(Uri path) {
        if (!IPV4_PATTERN.matcher(path.getHost()).find()) {
            return null;
        }
        try {
            return (Inet4Address) InetAddress.getByName(path.getHost());
        } catch (UnknownHostException ignored) {
        }
        return null;
    }

    /** Return {@link NetworkInterface} corresponding to the supplied name, or null */
    private static NetworkInterface toNetworkInterface(String name) {
        if (name == null) {
            return null;
        }
        try {
            return NetworkInterface.getByName(name);
        } catch (SocketException e) {
            return null;
        }
    }

    /**
     * Return true if the printer's path is P2P
     */
    public static boolean isP2p(DiscoveredPrinter printer) {
        return printer.path.getScheme().equals(P2pDiscovery.SCHEME_P2P);
    }

    /**
     * Return true if the printer's path is on the currently connected P2P interface
     */
    public static boolean isOnConnectedInterface(BuiltInPrintService service,
            DiscoveredPrinter printer) {
        String connectedInterface = service.getP2pMonitor().getConnectedInterface();
        return connectedInterface != null
                && P2pUtils.isOnInterface(connectedInterface, printer.path);
    }

    /** Return true if the supplied remote address is on the network interface */
    static boolean isOnInterface(NetworkInterface iface, Inet4Address address) {
        long addressLong = toLong(address);
        for (InterfaceAddress ifaceAddress : iface.getInterfaceAddresses()) {
            if (!(ifaceAddress.getAddress() instanceof Inet4Address)) {
                continue;
            }
            Inet4Address networkAddress = (Inet4Address) ifaceAddress.getAddress();

            BitSet bitSet = new BitSet(32);
            bitSet.set(32 - ifaceAddress.getNetworkPrefixLength(), 32);
            long netMask = bitSet.toLongArray()[0];

            if ((toLong(networkAddress) & netMask) == (addressLong & netMask)) {
                return true;
            }
        }
        return false;
    }

    private static long toLong(Inet4Address address) {
        byte[] bytes = address.getAddress();
        return ((bytes[0] & 0xFFL) << 24) + ((bytes[1] & 0xFFL) << 16)
                + ((bytes[2] & 0xFFL) << 8) + (bytes[3] & 0xFFL);
    }
}
