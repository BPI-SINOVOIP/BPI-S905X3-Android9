class ActionContext(object):
    """
    Provides the dependencies actions might need to execute.
    """
    def __init__(self,
                 cfm_facade=None,
                 file_contents_collector=None,
                 host=None,
                 usb_device_collector=None,
                 usb_port_manager=None,
                 crash_detector=None):
        """
        Initializes.

        Parameters are set to None by default to make it easier to provide
        a subset of dependencies in tests.

        @param cfm_facade CFM facade to use, an instance of
                CFMFacadeRemoteAdapter.
        @param file_contents_collector object with a
                collect_file_contents(file_name) method to get file contents
                from the specified file on the DUT.
        @param host an Host instance.
        @param usb_device_collecor a UsbDeviceCollector instance.
        @param usb_port_manager a UsbPortManager instance.
        @param crash_detector a CrashDetector instance.
        """
        self.cfm_facade = cfm_facade
        self.file_contents_collector = file_contents_collector
        # TODO(kerl) consider using a facade to the Host to only provide an
        # interface with what we need.
        self.host = host
        self.usb_device_collector = usb_device_collector
        self.usb_port_manager = usb_port_manager
        self.crash_detector = crash_detector

