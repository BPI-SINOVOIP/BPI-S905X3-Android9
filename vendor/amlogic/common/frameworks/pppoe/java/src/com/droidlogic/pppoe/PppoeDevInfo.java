/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

package com.droidlogic.pppoe;


import android.os.Parcel;
import android.os.Parcelable;
import android.os.Parcelable.Creator;

public class PppoeDevInfo implements Parcelable {
    private String dev_name;
    private String ipaddr;
    private String netmask;
    private String route;
    private String dns;
    private String mode;
    public static final String PPPOE_CONN_MODE_DHCP = "dhcp";
    public static final String PPPOE_CONN_MODE_MANUAL = "manual";

    public PppoeDevInfo () {
        dev_name = null;
        ipaddr = null;
        dns = null;
        route = null;
        netmask = null;
        mode = PPPOE_CONN_MODE_DHCP;
    }

    public void setIfName(String ifname) {
        this.dev_name = ifname;
    }

    public String getIfName() {
        return this.dev_name;
    }

    public void setIpAddress(String ip) {
        this.ipaddr = ip;
    }

    public String getIpAddress( ) {
        return this.ipaddr;
    }
    public void setNetMask(String ip) {
        this.netmask = ip;
    }

    public String getNetMask( ) {
        return this.netmask;
    }

    public void setRouteAddr(String route) {
        this.route = route;
    }

    public String getRouteAddr() {
        return this.route;
    }

    public void setDnsAddr(String dns) {
        this.dns = dns;
    }

    public String getDnsAddr( ) {
        return this.dns;
    }

    public boolean setConnectMode(String mode) {
        if (mode.equals(PPPOE_CONN_MODE_DHCP) || mode.equals(PPPOE_CONN_MODE_MANUAL)) {
            this.mode = mode;
            return true;
        }
        return false;
    }

    public String getConnectMode() {
        return this.mode;
    }

    public int describeContents() {
        // TODO Auto-generated method stub
        return 0;
    }

    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.dev_name);
        dest.writeString(this.ipaddr);
        dest.writeString(this.netmask);
        dest.writeString(this.route);
        dest.writeString(this.dns);
        dest.writeString(this.mode);
    }
      /** Implement the Parcelable interface {@hide} */
    public static final Creator<PppoeDevInfo> CREATOR =
    new Creator<PppoeDevInfo>() {
        public PppoeDevInfo createFromParcel(Parcel in) {
            PppoeDevInfo info = new PppoeDevInfo();
            info.setIfName(in.readString());
            info.setIpAddress(in.readString());
            info.setNetMask(in.readString());
            info.setRouteAddr(in.readString());
            info.setDnsAddr(in.readString());
            info.setConnectMode(in.readString());
            return info;
        }

        public PppoeDevInfo[] newArray(int size) {
            return new PppoeDevInfo[size];
        }
    };
}
