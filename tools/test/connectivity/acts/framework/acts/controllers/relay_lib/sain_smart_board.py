#!/usr/bin/env python
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from future.moves.urllib.request import urlopen
import re

from acts.controllers.relay_lib.errors import RelayDeviceConnectionError
from acts.controllers.relay_lib.helpers import validate_key
from acts.controllers.relay_lib.relay import RelayState
from acts.controllers.relay_lib.relay_board import RelayBoard

BASE_URL = 'http://192.168.1.4/30000/'


class SainSmartBoard(RelayBoard):
    """Controls and queries SainSmart Web Relay Board.

    Controls and queries SainSmart Web Relay Board, found here:
    http://www.sainsmart.com/sainsmart-rj45-tcp-ip-remote-controller-board-with-8-channels-relay-integrated.html
    this uses a web interface to toggle relays.

    There is an unmentioned hidden status page that can be found at <root>/99/.
    """

    # No longer used. Here for debugging purposes.
    #
    # Old status pages. Used before base_url/99 was found.
    # STATUS_1 = '40'
    # STATUS_2 = '43'
    #
    # This is the regex used to parse the old status pages:
    # r'y-\d(?P<relay>\d).+?> (?:&nbsp)?(?P<status>.*?)&'
    #
    # Pages that will turn all switches on or off, even the ghost switches.
    # ALL_RELAY_OFF = '44'
    # ALL_RELAY_ON = '45'

    HIDDEN_STATUS_PAGE = '99'

    VALID_RELAY_POSITIONS = [0, 1, 2, 3, 4, 5, 6, 7]
    NUM_RELAYS = 8

    def __init__(self, config):
        # This will be lazy loaded
        self.status_dict = None
        self.base_url = validate_key('base_url', config, str, 'config')
        if not self.base_url.endswith('/'):
            self.base_url += '/'
        super(SainSmartBoard, self).__init__(config)

    def get_relay_position_list(self):
        return self.VALID_RELAY_POSITIONS

    def _load_page(self, relative_url):
        """Loads a web page at self.base_url + relative_url.

        Properly opens and closes the web page.

        Args:
            relative_url: The string appended to the base_url

        Returns:
            the contents of the web page.

        Raises:
            A RelayDeviceConnectionError is raised if the page cannot be loaded.

        """
        try:
            page = urlopen(self.base_url + relative_url)
            result = page.read().decode("utf-8")
            page.close()
        except:
            raise RelayDeviceConnectionError(
                'Unable to connect to board "{}" through {}'.format(
                    self.name, self.base_url + relative_url))
        return result

    def _sync_status_dict(self):
        """Returns a dictionary of relays and there current state."""
        result = self._load_page(self.HIDDEN_STATUS_PAGE)
        if 'TUX' not in result:
            raise RelayDeviceConnectionError(
                'Sainsmart board with URL %s has not completed initialization '
                'after its IP was set, and must be power-cycled to prevent '
                'random disconnections. After power-cycling, make sure %s/%s '
                'has TUX appear in its output.' %
                (self.base_url, self.base_url, self.HIDDEN_STATUS_PAGE))
        status_string = re.search(r'">([01]*)TUX', result).group(1)

        self.status_dict = dict()
        for index, char in enumerate(status_string):
            self.status_dict[index] = \
                RelayState.NC if char == '1' else RelayState.NO

    def _print_status(self):
        """Prints out the list of relays and their current state."""
        for i in range(0, 8):
            print('Relay {}: {}'.format(i, self.status_dict[i]))

    def get_relay_status(self, relay_position):
        """Returns the current status of the passed in relay."""
        if self.status_dict is None:
            self._sync_status_dict()
        return self.status_dict[relay_position]

    def set(self, relay_position, value):
        """Sets the given relay to be either ON or OFF, indicated by value."""
        if self.status_dict is None:
            self._sync_status_dict()
        self._load_page(self._get_relay_url_code(relay_position, value))
        self.status_dict[relay_position] = value

    @staticmethod
    def _get_relay_url_code(relay_position, no_or_nc):
        """Returns the two digit code corresponding to setting the relay."""
        if no_or_nc == RelayState.NC:
            on_modifier = 1
        else:
            on_modifier = 0
        return '{:02d}'.format(relay_position * 2 + on_modifier)
