from autotest_lib.client.common_lib.cros.cfm import cras_input_node
from autotest_lib.client.common_lib.cros.cfm import cras_output_node

class CrasNodeCollector(object):
    """Utility class for obtaining node data from cras_test_client."""

    DEVICE_COLUMN_COUNT = 2
    NODE_COLUMN_COUNT = 8
    DEVICE_HEADERS = ['ID', 'Name']
    OUTPUT_NODE_HEADERS = ['Stable Id', 'ID', 'Vol', 'Plugged', 'L/R swapped',
                           'Time Hotword', 'Type', 'Name']
    INPUT_NODE_HEADERS = ['Stable Id', 'ID', 'Gain', 'Plugged', 'L/Rswapped',
                          'Time Hotword', 'Type', 'Name']

    def __init__(self, host):
        """
        Constructor
        @param host the device under test (CrOS).
        """
        self._host = host

    def _replace_multiple_whitespace_with_one(self, string):
        """
        Replace multiple sequential whitespaces with a single whitespace.
        @returns a string
        """
        return ' '.join(string.split())

    def _construct_columns(self, columns_str, columns_count):
        """
        Constructs a list of strings from a single string.

        @param columns_str A whitespace separated list of values.
        @param columns_count number of columns to create
        @returns a list with strings.
        """
        # 1) Replace multiple whitespaces with one
        # 2) Split on whitespace, create nof_columns columns
        columns_str = self._replace_multiple_whitespace_with_one(columns_str)
        return columns_str.split(None, columns_count-1)

    def _collect_input_device_data(self):
        cmd = ("cras_test_client --dump_server_info "
              " | awk '/Input Devices:/,/Input Nodes:/'")
        lines = self._host.run_output(cmd).split('\n')
        # Ignore the first two lines ("Input Devices" and headers) and the
        # last line ("Input Nodes")
        lines = lines[2:-1]
        rows = [self._construct_columns(line, self.DEVICE_COLUMN_COUNT)
                for line in lines]
        return [dict(zip(self.DEVICE_HEADERS, row)) for row in rows]

    def _collect_output_device_data(self):
        cmd = ("cras_test_client --dump_server_info "
              " | awk '/Output Devices:/,/Output Nodes:/'")
        lines = self._host.run_output(cmd).split('\n')
        # Ignore the first two lines ("Output Devices" and headers) and the
        # last line ("Output Nodes")
        lines = lines[2:-1]
        rows = [self._construct_columns(line, self.DEVICE_COLUMN_COUNT)
                for line in lines]
        return [dict(zip(self.DEVICE_HEADERS, row)) for row in rows]

    def _collect_output_node_cras_data(self):
        """
        Collects output nodes data using cras_test_client.

        @returns a list of dictionaries where keys are in OUTPUT_NODE_HEADERS
        """
        # It's a bit hacky to use awk; we should probably do the parsing
        # in Python instead using textfsm or some other lib.
        cmd = ("cras_test_client --dump_server_info"
               "| awk '/Output Nodes:/,/Input Devices:/'")
        lines = self._host.run_output(cmd).split('\n')
        # Ignore the first two lines ("Output Nodes:" and headers) and the
        # last line ("Input Devices:")
        lines = lines[2:-1]
        rows = [self._construct_columns(line, self.NODE_COLUMN_COUNT)
                for line in lines]
        return [dict(zip(self.OUTPUT_NODE_HEADERS, row)) for row in rows]

    def _collect_input_node_cras_data(self):
        """
        Collects input nodes data using cras_test_client.

        @returns a list of dictionaries where keys are in INPUT_NODE_HEADERS
        """
        cmd = ("cras_test_client --dump_server_info "
              " | awk '/Input Nodes:/,/Attached clients:/'")
        lines = self._host.run_output(cmd).split('\n')
        # Ignore the first two lines ("Input Nodes:" and headers) and the
        # last line ("Attached clients:")
        lines = lines[2:-1]
        rows = [self._construct_columns(line, self.NODE_COLUMN_COUNT)
                for line in lines]
        return [dict(zip(self.INPUT_NODE_HEADERS, row)) for row in rows]

    def _get_device_name(self, device_data, device_id):
        for device in device_data:
            if device['ID'] == device_id:
                return device['Name']
        return None

    def _create_input_node(self, node_data, device_data):
        """
        Create a CrasInputNode.
        @param node_data Input Node data
        @param device_data Input Device data
        @return CrasInputNode
        """
        device_id = node_data['ID'].split(':')[0]
        device_name = self._get_device_name(device_data, device_id)
        return cras_input_node.CrasInputNode(
            node_id=node_data['ID'],
            node_name=node_data['Name'],
            gain=node_data['Gain'],
            node_type=node_data['Type'],
            device_id=device_id,
            device_name=device_name)

    def _create_output_node(self, node_data, device_data):
        """
        Create a CrasOutputNode.
        @param node_data Output Node data
        @param device_data Output Device data
        @return CrasOutputNode
        """
        device_id = node_data['ID'].split(':')[0]
        device_name = self._get_device_name(device_data, device_id)
        return cras_output_node.CrasOutputNode(
            node_id=node_data['ID'],
            node_type=node_data['Type'],
            node_name=node_data['Name'],
            volume=node_data['Vol'],
            device_id=device_id,
            device_name=device_name)

    def get_input_nodes(self):
        device_data = self._collect_input_device_data()
        node_data = self._collect_input_node_cras_data()
        return [self._create_input_node(d, device_data) for d in node_data]

    def get_output_nodes(self):
        device_data = self._collect_output_device_data()
        node_data = self._collect_output_node_cras_data()
        return [self._create_output_node(d, device_data) for d in node_data]
