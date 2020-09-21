"""A wrapper around the Generic Buffer Manager (GBM) library.

Currently implements exactly the functions required to screenshot a frame
buffer using DRM crtc info.
"""
from ctypes import *
import drm
from PIL import Image

GBM_BO_IMPORT_FD = 0x5503
GBM_BO_USE_SCANOUT = c_uint(1)
GBM_BO_TRANSFER_READ = c_uint(1)
GBM_MAX_PLANES = 4

def __gbm_fourcc_code(a, b, c, d):
    return ord(a) | (ord(b) << 8) | (ord(c) << 16) | (ord(d) << 24)

GBM_FORMAT_ARGB8888 = __gbm_fourcc_code("A", "R", "2", "4")
GBM_LIBRARIES = ["libgbm.so", "libgbm.so.1"]


class gbm_import_fd_data(Structure):
    _fields_ = [
        ("fd", c_int),
        ("width", c_uint),
        ("height", c_uint),
        ("stride", c_uint),
        ("bo_format", c_uint),
    ]


class gbm_import_fd_planar_data(Structure):
    _fields_ = [
        ("fds", c_int * GBM_MAX_PLANES),
        ("width", c_uint),
        ("height", c_uint),
        ("bo_format", c_uint),
        ("strides", c_uint * GBM_MAX_PLANES),
        ("offsets", c_uint * GBM_MAX_PLANES),
        ("format_modifiers", c_ulonglong * GBM_MAX_PLANES),
    ]


class gbm_device(Structure):
    """Opaque struct for GBM device.
    """
    pass


class gbm_bo(Structure):
    """Opaque struct for GBM buffer.
    """
    pass


def loadGBM():
    """Load and return a handle to libgbm.so.
    """
    l = None

    for lib in GBM_LIBRARIES:
        try:
            l = cdll.LoadLibrary(lib)
        except OSError:
            l = None
        if l is not None:
            break

    if l is None:
        raise RuntimeError("Could not load GBM library.")
        return None

    l.gbm_create_device.argtypes = [c_int]
    l.gbm_create_device.restype = POINTER(gbm_device)

    l.gbm_device_destroy.argtypes = [POINTER(gbm_device)]
    l.gbm_device_destroy.restype = None

    l.gbm_bo_import.argtypes = [POINTER(gbm_device), c_uint, c_void_p, c_uint]
    l.gbm_bo_import.restype = POINTER(gbm_bo)

    l.gbm_bo_map.argtypes = [
        POINTER(gbm_bo), c_uint, c_uint, c_uint, c_uint, c_uint,
        POINTER(c_uint),
        POINTER(c_void_p), c_size_t
    ]
    l.gbm_bo_map.restype = c_void_p

    l.gbm_bo_unmap.argtypes = [POINTER(gbm_bo), c_void_p]
    l.gbm_bo_unmap.restype = None

    return l


class GBMBuffer(object):
    """A GBM buffer.
    """

    def __init__(self, library, buffer):
        self._l = library
        self._buffer = buffer

    def __del__(self):
        if self._l:
            self._l.gbm_bo_destroy(self._buffer)

    @classmethod
    def fromFD(cls, device, fd, width, height, stride, bo_format, usage):
        """Create/import a GBM Buffer Object from a file descriptor.

        @param device: GBM device object.
        @param fd: a file descriptor for the buffer to be imported.
        @param width: buffer width in pixels.
        @param height: buffer height in pixels.
        @param stride: buffer pitch; number of pixels between sequential rows.
        @param bo_format: pixel format fourcc code, e.g. GBM_FORMAT_ARGB8888.
        @param usage: buffer usage flags, e.g. GBM_BO_USE_SCANOUT.
        """
        bo_data = gbm_import_fd_data()
        bo_data.fd = fd
        bo_data.width = width
        bo_data.height = height
        bo_data.stride = stride
        bo_data.bo_format = bo_format
        buffer = device._l.gbm_bo_import(device._device, GBM_BO_IMPORT_FD,
                                         byref(bo_data), usage)
        if buffer is None:
            raise RuntimeError("gbm_bo_import() returned NULL")

        self = cls(device._l, buffer)
        return self

    def map(self, x, y, width, height, flags, plane):
        """Map buffer data into this user-space.
        Returns (address, stride_bytes): void* start address for pixel array,
        number of BYTES between sequental rows of pixels.
        @param flags: The union of the GBM_BO_TRANSFER_* flags for this buffer.
        """
        self._map_p = c_void_p(0)
        stride_out = c_uint(0)
        if width == 0 or height == 0:
            raise RuntimeError("Map width and/or height is 0")
        map_p = self._l.gbm_bo_map(self._buffer, x, y, width, height, flags,
                                   byref(stride_out), byref(self._map_p), plane)
        if stride_out is 0:
            raise RuntimeError("gbm_bo_map() stride is 0")
        if map_p is 0:
            raise RuntimeError("gbm_bo_map() returned NULL")
        return map_p, stride_out

    def unmap(self, map_data_p):
        self._l.gbm_bo_unmap(self._buffer, map_data_p)


class GBMDevice(object):
    """A GBM device.
    """

    def __init__(self, library, handle):
        self._l = library
        self._handle = handle
        self._device = library.gbm_create_device(self._handle)

    def __del__(self):
        if self._l:
            self._l.gbm_device_destroy(self._device)

    @classmethod
    def fromHandle(cls, handle):
        """Create a device object from an open file descriptor.
        """
        self = cls(loadGBM(), handle)
        return self


def _bgrx24(i):
    b = i & 255
    g = (i >> 8) & 255
    r = (i >> 16) & 255
    return r, g, b


def crtcScreenshot(crtc_id=None):
    """Take a screenshot, returning an image object.

    @param crtc_id: The CRTC to screenshot.
                    None for first found CRTC with mode set
                    or "internal" for crtc connected to internal LCD
                    or "external" for crtc connected to external display
                    or "usb" "evdi" or "udl" for crtc with valid mode on evdi
                    or udl display
                    or DRM integer crtc_id
    """
    crtc = drm.getCrtc(crtc_id)
    if crtc is not None:
        device = GBMDevice.fromHandle(drm._drm._fd)
        framebuffer = crtc.fb()
        # TODO(djmk): The buffer format is hardcoded to ARGB8888, we should fix
        # this to query for the frambuffer's format instead.
        format_hardcode = GBM_FORMAT_ARGB8888

        bo = GBMBuffer.fromFD(device,
                              framebuffer.getFD(), framebuffer.width,
                              framebuffer.height, framebuffer.pitch,
                              format_hardcode, GBM_BO_USE_SCANOUT)
        map_void_p, stride_bytes = bo.map(0, 0, framebuffer.width,
                                          framebuffer.height,
                                          GBM_BO_TRANSFER_READ, 0)
        map_bytes = stride_bytes.value * framebuffer.height

        # Create a Python Buffer object which references (but does not own) the
        # memory.
        buffer_from_memory = pythonapi.PyBuffer_FromMemory
        buffer_from_memory.restype = py_object
        buffer_from_memory.argtypes = [c_void_p, c_ssize_t]
        map_buffer = buffer_from_memory(map_void_p, map_bytes)

        # Make a copy of the bytes. Doing this is faster than the conversion,
        # and is more likely to capture a consistent snapshot of the framebuffer
        # contents, as a process may be writing to it.
        buffer_bytes = bytes(map_buffer)

        # Load the image, converting from the BGRX format to a PIL Image in RGB
        # form. As the conversion is implemented by PIL as C code, this
        # conversion is much faster than calling _bgrx24().
        image = Image.fromstring(
                'RGB', (framebuffer.width, framebuffer.height), buffer_bytes,
                'raw', 'BGRX', stride_bytes.value, 1)
        bo.unmap(bo._map_p)
	del bo
        return image

    raise RuntimeError(
        "Unable to take screenshot. There may not be anything on the screen.")


def crtcGetPixel(x, y, crtc_id=None):
    """Get a pixel from the specified screen, returning a rgb tuple.

    @param x: The x-coordinate of the desired pixel.
    @param y: The y-coordinate of the desired pixel.
    @param crtc_id: The CRTC to get the pixel from.
                    None for first found CRTC with mode set
                    or "internal" for crtc connected to internal LCD
                    or "external" for crtc connected to external display
                    or "usb" "evdi" or "udl" for crtc with valid mode on evdi
                    or udl display
                    or DRM integer crtc_id
    """
    crtc = drm.getCrtc(crtc_id)
    if crtc is None:
        raise RuntimeError(
            "Unable to get pixel. There may not be anything on the screen.")
    device = GBMDevice.fromHandle(drm._drm._fd)
    framebuffer = crtc.fb()
    bo = GBMBuffer.fromFD(device,
                          framebuffer.getFD(), framebuffer.width,
                          framebuffer.height, framebuffer.pitch,
                          GBM_FORMAT_ARGB8888, GBM_BO_USE_SCANOUT)
    map_void_p, _ = bo.map(int(x), int(y), 1, 1, GBM_BO_TRANSFER_READ, 0)
    map_int_p = cast(map_void_p, POINTER(c_int))
    pixel = _bgrx24(map_int_p[0])
    bo.unmap(bo._map_p)
    return pixel
