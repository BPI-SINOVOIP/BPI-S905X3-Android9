class CrasInputNode(object):
    """Class representing an input node from ChromeOS Audio Server data.

    An input node is a node that can pick up audio, e.g. a microphone jack.
    """

    def __init__(self, node_id, node_name, gain, node_type, device_id,
                 device_name):
        self.node_id = node_id
        self.node_name = node_name
        self.gain = int(gain)
        self.node_type = node_type
        self.device_id = device_id
        self.device_name = device_name

    def __str__(self):
        return ('Node id: %s, Node name: %s, Device id: %s, Device name: %s '
                'Gain: %d' % (self.node_id, self.node_name, self.device_id,
                              self.device_name, self.gain))
