# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2017, ARM Limited, Google, and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Parser for dumpsys SurfaceFlinger output

import ast, re

def get_value(token):
    try:
        v = ast.literal_eval(token)
    except:
        return token
    return v

def parse_value(value):
    """
    Parses and evaluates the different possible value types of the properties from SurfaceFlinger.
    Supported types: literals, tuples, arrays, multidimensional arrays (in the form [...][...])
    """
    value = value.replace('(', '[')
    value = value.replace(')', ']')

    # Parse arrays and tuples. Ex: (1024, 1980) Ex: [1, 89, 301]
    if value[0] == '[':
        parsed_values = []
        dim = -1
        start_index = 0
        for i, c in enumerate(value):
            # Handle multidimensional arrays. Ex: [0, 1][2, 3] -> [[0, 1], [2, 3]]
            if c == '[':
                dim += 1
                parsed_values.append([])
                start_index = i + 1
                continue
            # Finish parsing a value, evaluate it, and add it to the current array
            elif c == ']' or c == ',':
                parsed_values[dim].append(get_value(value[start_index:i].strip()))
                start_index = i + 1
                continue
        # If the array is one dimensional, return a one dimensional array
        if dim == 0:
            return parsed_values[0]
        else:
            return parsed_values
    else:
        # Attempt to parse a literal
        return get_value(value)


class _Layer(object):
    """
    Represents a layer of a view drawn in an Android app. This class holds the properties of the
    layer that have been parsed from the output of SurfaceFlinger.
    """

    def __init__(self, name):
        """
        Initializes the layer.

        :param name: the parsed name of the the layer
        """
        self._properties = { 'name' : name }

    def __dir__(self):
        """
        List all the available properties parsed from the output of SurfaceFlinger.
        """
        return self._properties.keys()

    def __getattr__(self, attr):
        """
        Retrieves a property of the layer through using the period operator.
        Ex: obj.frame_counter, obj.name, obj.size
        """
        return self._properties[attr]


class SurfaceFlinger(object):
    """
    Used to parse the output of dumpsys SurfaceFlinger.
    """

    def __init__(self, path):
        """
        Initializes the SurfaceFlinger parser object.

        :param path: Path to file containing output of SurfaceFlinger
        """
        self.path = path
        self.layers = { }
        self.__parse_surfaceflinger()

    def __parse_surfaceflinger(self):
        """
        Parses the SurfaceFlinger output and stores a map of layer names to objects that hold the
        parsed information of the layer.
        """
        with open(self.path) as f:
            current_layer = None
            lines_to_skip = 0

            content = f.readlines()
            for line in content:
                if lines_to_skip > 0:
                    lines_to_skip -= 1
                    continue

                # Begin parsing the properties of a layer
                if "+ Layer" in line:
                    # Extract the name from the layer
                    # Ex: + Layer 0x745c86c000 (NavigationBar#0) -> NavigationBar#0
                    layer_name = line[line.index('(') + 1 : line.index(')')]
                    current_layer = _Layer(layer_name)
                    self.layers[layer_name] = current_layer
                    lines_to_skip = 6
                    continue

                # Parse the properties of the current layer
                if not current_layer is None and '=' in line:

                    # Format the line so that the first and last properties parse correctly
                    line = "_ " + line + " _"

                    # Ex: _ layerStack=  0, z=  231000, pos=(0,1794), size=(1080, 126) _ ->
                    # ['_ layerStack', '  0, z', '  231000, pos', '(0,1794), size', '(1080, 126) _']
                    tokens = [x.strip() for x in line.split('=')]

                    for i in xrange(0, len(tokens) - 1):

                        # Parse layer property names and replace hyphens with underscores
                        # Ex: key = 'layerStack' value = '0,'
                        # Ex: key = 'z' value = '231000'
                        key = tokens[i][tokens[i].rfind(' ') + 1:].replace('-', '_')
                        value = tokens[i + 1][:tokens[i + 1].rfind(' ')].strip()

                        # Some properties are not separated by commas, so get rid of trailing commas
                        # if the value is separated from the next key by one
                        if value[len(value) - 1] == ',':
                            value = value[:-1]

                        # Parse the value and add the property to the layer
                        value = parse_value(value)
                        current_layer._properties.update({key : value})

                # Do not parse layer information after the slots field
                if not current_layer is None and "Slots:" in line:
                    current_layer = None

