class CrasOutputNode(object):
    """Class representing an output node from ChromeOS Audio Server data.

    An output node is a node that can play out audio, e.g. a headphone jack.
    """

    def __init__(self, node_id, node_type, node_name, volume, device_id,
                 device_name):
        self.node_id = node_id
        self.node_type = node_type
        self.node_name = node_name
        self.volume = int(volume)
        self.device_id = device_id
        self.device_name = device_name

    def __str__(self):
        return ('Node id %s, Node name: %s, Device id: %s, Device name: %s '
                'Volume: %d' % (self.node_id, self.node_name, self.device_id,
                                self.device_name, self.volume))
