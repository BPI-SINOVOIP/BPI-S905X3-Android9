"""
A wrapper around the Direct Rendering Manager (DRM) library, which itself is a
wrapper around the Direct Rendering Interface (DRI) between the kernel and
userland.

Since we are masochists, we use ctypes instead of cffi to load libdrm and
access several symbols within it. We use Python's file descriptor and mmap
wrappers.

At some point in the future, cffi could be used, for approximately the same
cost in lines of code.
"""

from ctypes import *
import mmap
import os
import subprocess

# drmModeConnection enum
DRM_MODE_CONNECTED         = 1
DRM_MODE_DISCONNECTED      = 2
DRM_MODE_UNKNOWNCONNECTION = 3

DRM_MODE_CONNECTOR_Unknown     = 0
DRM_MODE_CONNECTOR_VGA         = 1
DRM_MODE_CONNECTOR_DVII        = 2
DRM_MODE_CONNECTOR_DVID        = 3
DRM_MODE_CONNECTOR_DVIA        = 4
DRM_MODE_CONNECTOR_Composite   = 5
DRM_MODE_CONNECTOR_SVIDEO      = 6
DRM_MODE_CONNECTOR_LVDS        = 7
DRM_MODE_CONNECTOR_Component   = 8
DRM_MODE_CONNECTOR_9PinDIN     = 9
DRM_MODE_CONNECTOR_DisplayPort = 10
DRM_MODE_CONNECTOR_HDMIA       = 11
DRM_MODE_CONNECTOR_HDMIB       = 12
DRM_MODE_CONNECTOR_TV          = 13
DRM_MODE_CONNECTOR_eDP         = 14
DRM_MODE_CONNECTOR_VIRTUAL     = 15
DRM_MODE_CONNECTOR_DSI         = 16

# This constant is not defined in any one header; it is the pieced-together
# incantation for the ioctl that performs dumb mappings. I would love for this
# to not have to be here, but it can't be imported from any header easily.
DRM_IOCTL_MODE_MAP_DUMB = 0xc01064b3

# This define should be equal to O_CLOEXEC, which should be available in
# python's os module, but isn't until version 3.3.  If we version up, we can
# set this to os.O_CLOEXEC.
DRM_CLOEXEC = 02000000


class DrmVersion(Structure):
    """
    The version of a DRM node.
    """

    _fields_ = [
        ("version_major", c_int),
        ("version_minor", c_int),
        ("version_patchlevel", c_int),
        ("name_len", c_int),
        ("name", c_char_p),
        ("date_len", c_int),
        ("date", c_char_p),
        ("desc_len", c_int),
        ("desc", c_char_p),
    ]

    _l = None

    def __repr__(self):
        return "%s %d.%d.%d (%s) (%s)" % (self.name,
                                          self.version_major,
                                          self.version_minor,
                                          self.version_patchlevel,
                                          self.desc,
                                          self.date,)

    def __del__(self):
        if self._l:
            self._l.drmFreeVersion(self)


class DrmModeResources(Structure):
    """
    Resources associated with setting modes on a DRM node.
    """

    _fields_ = [
        ("count_fbs", c_int),
        ("fbs", POINTER(c_uint)),
        ("count_crtcs", c_int),
        ("crtcs", POINTER(c_uint)),
        ("count_connectors", c_int),
        ("connectors", POINTER(c_uint)),
        ("count_encoders", c_int),
        ("encoders", POINTER(c_uint)),
        ("min_width", c_int),
        ("max_width", c_int),
        ("min_height", c_int),
        ("max_height", c_int),
    ]

    _fd = None
    _l = None

    def __repr__(self):
        return "<DRM mode resources>"

    def __del__(self):
        if self._l:
            self._l.drmModeFreeResources(self)

    def _wakeup_screen(self):
        """
        Send a synchronous dbus message to power on screen.
        """
        # Get and process reply to make this synchronous.
        subprocess.check_output([
            "dbus-send", "--type=method_call", "--system", "--print-reply",
            "--dest=org.chromium.PowerManager", "/org/chromium/PowerManager",
            "org.chromium.PowerManager.HandleUserActivity", "int32:0"
        ])

    def getValidCrtc(self):
        for i in xrange(0, self.count_crtcs):
            crtc_id = self.crtcs[i]
            crtc = self._l.drmModeGetCrtc(self._fd, crtc_id).contents
            if crtc.mode_valid:
                return crtc
        return None

    def getCrtc(self, crtc_id):
        """
        Obtain the CRTC at a given index.

        @param crtc_id: The CRTC to get.
        """
        if crtc_id:
            return self._l.drmModeGetCrtc(self._fd, crtc_id).contents
        return self.getValidCrtc()

    def getCrtcRobust(self, crtc_id=None):
        crtc = self.getCrtc(crtc_id)
        if crtc is None:
            self._wakeup_screen()
            crtc = self.getCrtc(crtc_id)
        if crtc is not None:
            crtc._fd = self._fd
            crtc._l = self._l
        return crtc


class DrmModeModeInfo(Structure):
    """
    A DRM modesetting mode info.
    """

    _fields_ = [
        ("clock", c_uint),
        ("hdisplay", c_ushort),
        ("hsync_start", c_ushort),
        ("hsync_end", c_ushort),
        ("htotal", c_ushort),
        ("hskew", c_ushort),
        ("vdisplay", c_ushort),
        ("vsync_start", c_ushort),
        ("vsync_end", c_ushort),
        ("vtotal", c_ushort),
        ("vscan", c_ushort),
        ("vrefresh", c_uint),
        ("flags", c_uint),
        ("type", c_uint),
        ("name", c_char * 32),
    ]


class DrmModeCrtc(Structure):
    """
    A DRM modesetting CRTC.
    """

    _fields_ = [
        ("crtc_id", c_uint),
        ("buffer_id", c_uint),
        ("x", c_uint),
        ("y", c_uint),
        ("width", c_uint),
        ("height", c_uint),
        ("mode_valid", c_int),
        ("mode", DrmModeModeInfo),
        ("gamma_size", c_int),
    ]

    _fd = None
    _l = None

    def __repr__(self):
        return "<CRTC (%d)>" % self.crtc_id

    def __del__(self):
        if self._l:
            self._l.drmModeFreeCrtc(self)

    def hasFb(self):
        """
        Whether this CRTC has an associated framebuffer.
        """

        return self.buffer_id != 0

    def fb(self):
        """
        Obtain the framebuffer, if one is associated.
        """

        if self.hasFb():
            fb = self._l.drmModeGetFB(self._fd, self.buffer_id).contents
            fb._fd = self._fd
            fb._l = self._l
            return fb
        else:
            raise RuntimeError("CRTC %d doesn't have a framebuffer!" %
                               self.crtc_id)


class DrmModeEncoder(Structure):
    """
    A DRM modesetting encoder.
    """

    _fields_ = [
        ("encoder_id", c_uint),
        ("encoder_type", c_uint),
        ("crtc_id", c_uint),
        ("possible_crtcs", c_uint),
        ("possible_clones", c_uint),
    ]

    _fd = None
    _l = None

    def __repr__(self):
        return "<Encoder (%d)>" % self.encoder_id

    def __del__(self):
        if self._l:
            self._l.drmModeFreeEncoder(self)


class DrmModeConnector(Structure):
    """
    A DRM modesetting connector.
    """

    _fields_ = [
        ("connector_id", c_uint),
        ("encoder_id", c_uint),
        ("connector_type", c_uint),
        ("connector_type_id", c_uint),
        ("connection", c_uint), # drmModeConnection enum
        ("mmWidth", c_uint),
        ("mmHeight", c_uint),
        ("subpixel", c_uint), # drmModeSubPixel enum
        ("count_modes", c_int),
        ("modes", POINTER(DrmModeModeInfo)),
        ("count_propts", c_int),
        ("props", POINTER(c_uint)),
        ("prop_values", POINTER(c_ulonglong)),
        ("count_encoders", c_int),
        ("encoders", POINTER(c_uint)),
    ]

    _fd = None
    _l = None

    def __repr__(self):
        return "<Connector (%d)>" % self.connector_id

    def __del__(self):
        if self._l:
            self._l.drmModeFreeConnector(self)

    def isInternal(self):
        return (self.connector_type == DRM_MODE_CONNECTOR_LVDS or
                self.connector_type == DRM_MODE_CONNECTOR_eDP or
                self.connector_type == DRM_MODE_CONNECTOR_DSI)

    def isConnected(self):
        return self.connection == DRM_MODE_CONNECTED


class drm_mode_map_dumb(Structure):
    """
    Request a mapping of a modesetting buffer.

    The map will be "dumb;" it will be accessible via mmap() but very slow.
    """

    _fields_ = [
        ("handle", c_uint),
        ("pad", c_uint),
        ("offset", c_ulonglong),
    ]


class DrmModeFB(Structure):
    """
    A DRM modesetting framebuffer.
    """

    _fields_ = [
        ("fb_id", c_uint),
        ("width", c_uint),
        ("height", c_uint),
        ("pitch", c_uint),
        ("bpp", c_uint),
        ("depth", c_uint),
        ("handle", c_uint),
    ]

    _l = None
    _map = None

    def __repr__(self):
        s = "<Framebuffer (%dx%d (pitch %d bytes), %d bits/pixel, depth %d)"
        vitals = s % (self.width,
                      self.height,
                      self.pitch,
                      self.bpp,
                      self.depth,)
        if self._map:
            tail = " (mapped)>"
        else:
            tail = ">"
        return vitals + tail

    def __del__(self):
        if self._l:
            self._l.drmModeFreeFB(self)

    def map(self, size):
        """
        Map the framebuffer.
        """

        if self._map:
            return

        mapDumb = drm_mode_map_dumb()
        mapDumb.handle = self.handle

        rv = self._l.drmIoctl(self._fd, DRM_IOCTL_MODE_MAP_DUMB,
                              pointer(mapDumb))
        if rv:
            raise IOError(rv, os.strerror(rv))

        # mmap.mmap() has a totally different order of arguments in Python
        # compared to C; check the documentation before altering this
        # incantation.
        self._map = mmap.mmap(self._fd,
                              size,
                              flags=mmap.MAP_SHARED,
                              prot=mmap.PROT_READ,
                              offset=mapDumb.offset)

    def unmap(self):
        """
        Unmap the framebuffer.
        """

        if self._map:
            self._map.close()
            self._map = None

    def getFD(self):
        """
        Convert handle to a FD.
        """
        prime_fd = c_int(0)
        rv = self._l.drmPrimeHandleToFD(self._fd, self.handle,
                                        DRM_CLOEXEC, byref(prime_fd))
        if rv:
            raise RuntimeError("Failed to convert FB handle to FD. %d" % rv)
        return prime_fd

def loadDRM():
    """
    Load a handle to libdrm.

    In addition to loading, this function also configures the argument and
    return types of functions.
    """

    l = None

    try:
        l = cdll.LoadLibrary("libdrm.so")
    except OSError:
        l = cdll.LoadLibrary("libdrm.so.2") # ubuntu doesn't have libdrm.so

    l.drmGetVersion.argtypes = [c_int]
    l.drmGetVersion.restype = POINTER(DrmVersion)

    l.drmFreeVersion.argtypes = [POINTER(DrmVersion)]
    l.drmFreeVersion.restype = None

    l.drmModeGetResources.argtypes = [c_int]
    l.drmModeGetResources.restype = POINTER(DrmModeResources)

    l.drmModeFreeResources.argtypes = [POINTER(DrmModeResources)]
    l.drmModeFreeResources.restype = None

    l.drmModeGetCrtc.argtypes = [c_int, c_uint]
    l.drmModeGetCrtc.restype = POINTER(DrmModeCrtc)

    l.drmModeFreeCrtc.argtypes = [POINTER(DrmModeCrtc)]
    l.drmModeFreeCrtc.restype = None

    l.drmModeGetEncoder.argtypes = [c_int, c_uint]
    l.drmModeGetEncoder.restype = POINTER(DrmModeEncoder)

    l.drmModeFreeEncoder.argtypes = [POINTER(DrmModeEncoder)]
    l.drmModeFreeEncoder.restype = None

    l.drmModeGetConnector.argtypes = [c_int, c_uint]
    l.drmModeGetConnector.restype = POINTER(DrmModeConnector)

    l.drmModeFreeConnector.argtypes = [POINTER(DrmModeConnector)]
    l.drmModeFreeConnector.restype = None

    l.drmModeGetFB.argtypes = [c_int, c_uint]
    l.drmModeGetFB.restype = POINTER(DrmModeFB)

    l.drmModeFreeFB.argtypes = [POINTER(DrmModeFB)]
    l.drmModeFreeFB.restype = None

    l.drmIoctl.argtypes = [c_int, c_ulong, c_voidp]
    l.drmIoctl.restype = c_int

    l.drmPrimeHandleToFD.argtypes = [c_int, c_uint, c_uint, POINTER(c_int)]
    l.drmPrimeHandleToFD.restype = c_int

    return l


class DRM(object):
    """
    A DRM node.
    """

    def __init__(self, library, fd):
        self._l = library
        self._fd = fd

    def __repr__(self):
        return "<DRM (FD %d)>" % self._fd

    @classmethod
    def fromHandle(cls, handle):
        """
        Create a node from a file handle.

        @param handle: A file-like object backed by a file descriptor.
        """

        self = cls(loadDRM(), handle.fileno())
        # We must keep the handle alive, and we cannot trust the caller to
        # keep it alive for us.
        self._handle = handle
        return self

    def version(self):
        """
        Obtain the version.
        """

        v = self._l.drmGetVersion(self._fd).contents
        v._l = self._l
        return v

    def resources(self):
        """
        Obtain the modesetting resources.
        """

        resources_ptr = self._l.drmModeGetResources(self._fd)
        if resources_ptr:
            r = resources_ptr.contents
            r._fd = self._fd
            r._l = self._l
            return r

        return None

    def getCrtc(self, crtc_id):
        c_ptr = self._l.drmModeGetCrtc(self._fd, crtc_id)
        if c_ptr:
            c = c_ptr.contents
            c._fd = self._fd
            c._l = self._l
            return c

        return None

    def getEncoder(self, encoder_id):
        e_ptr = self._l.drmModeGetEncoder(self._fd, encoder_id)
        if e_ptr:
            e = e_ptr.contents
            e._fd = self._fd
            e._l = self._l
            return e

        return None

    def getConnector(self, connector_id):
        c_ptr = self._l.drmModeGetConnector(self._fd, connector_id)
        if c_ptr:
            c = c_ptr.contents
            c._fd = self._fd
            c._l = self._l
            return c

        return None



def drmFromPath(path):
    """
    Given a DRM node path, open the corresponding node.

    @param path: The path of the minor node to open.
    """
    # Always open the device as RW (r+) so that mmap works later.
    handle = open(path, "r+")
    return DRM.fromHandle(handle)


_drm = None


def getCrtc(crtc_id=None):
    """
    @param crtc_id: None for first found CRTC with mode set
                    or "internal" for crtc connected to internal LCD
                    or "external" for crtc connected to external display
                    or "usb" "evdi" or "udl" for crtc with valid mode on evdi or
                    udl display
                    or DRM integer crtc_id
    """
    global _drm

    if not _drm:
        paths = [
            "/dev/dri/" + n
            for n in filter(lambda x: x.startswith("card"),
                            os.listdir("/dev/dri"))
        ]

        if crtc_id == "usb" or crtc_id == "evdi" or crtc_id == "udl":
            for p in paths:
                d = drmFromPath(p)
                v = d.version()

                if crtc_id == v.name:
                    _drm = d
                    break

                if crtc_id == "usb" and (v.name == "evdi" or v.name == "udl"):
                    _drm = d
                    break

        elif crtc_id == "internal" or crtc_id == "external":
            internal = crtc_id == "internal"
            for p in paths:
                d = drmFromPath(p)
                if d.resources() is None:
                    continue
                if d.resources() and d.resources().count_connectors > 0:
                    for c in xrange(0, d.resources().count_connectors):
                        connector = d.getConnector(d.resources().connectors[c])
                        if (internal == connector.isInternal()
                            and connector.isConnected()
                            and connector.encoder_id != 0):
                            e = d.getEncoder(connector.encoder_id)
                            crtc = d.getCrtc(e.crtc_id)
                            if crtc.mode_valid:
                                crtc_id = crtc.crtc_id
                                _drm = d
                                break
                if _drm:
                    break

        elif crtc_id is None or crtc_id == 0:
            for p in paths:
                d = drmFromPath(p)
                if d.resources() is None:
                    continue
                for c in xrange(0, d.resources().count_crtcs):
                    crtc = d.getCrtc(d.resources().crtcs[c])
                    if crtc.mode_valid:
                        crtc_id = d.resources().crtcs[c]
                        _drm = d
                        break
                if _drm:
                    break

        else:
            for p in paths:
                d = drmFromPath(p)
                if d.resources() is None:
                    continue
                for c in xrange(0, d.resources().count_crtcs):
                    if crtc_id == d.resources().crtcs[c]:
                        _drm = d
                        break
                if _drm:
                    break

    if _drm:
        return _drm.resources().getCrtcRobust(crtc_id)

    return None
