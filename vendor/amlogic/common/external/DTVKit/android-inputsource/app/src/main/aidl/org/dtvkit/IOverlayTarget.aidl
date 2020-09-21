// IOverlayTarget.aidl
package org.dtvkit;

interface IOverlayTarget {
    void draw(int src_width, int src_height, int dst_x, int dst_y, int dst_width, int dst_height, inout byte[] data);
}
