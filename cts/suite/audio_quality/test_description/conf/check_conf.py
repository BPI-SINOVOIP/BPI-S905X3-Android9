#!/usr/bin/python

# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import numpy as np
import scipy as sp
from numpy import *
if __name__=="__main__":
    if(sys.version_info < (2,6,0)):
        print "Wrong Python version ", sys.version_info
        sys.exit(1)
    a = np.array([1,2,3])
    print "___CTS_AUDIO_PASS___"
