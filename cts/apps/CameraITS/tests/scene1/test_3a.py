# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import its.caps
import its.device

import numpy as np


def main():
    """Basic test for bring-up of 3A.

    To pass, 3A must converge. Check that the returned 3A values are legal.
    """

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.read_3a(props))
        mono_camera = its.caps.mono_camera(props)

        sens, exp, gains, xform, focus = cam.do_3a(get_results=True,
                                                   mono_camera=mono_camera)
        print 'AE: sensitivity %d, exposure %dms' % (sens, exp/1000000)
        print 'AWB: gains', gains, 'transform', xform
        print 'AF: distance', focus
        assert sens > 0
        assert exp > 0
        assert len(gains) == 4
        for g in gains:
            assert not np.isnan(g)
        assert len(xform) == 9
        for x in xform:
            assert not np.isnan(x)
        assert focus >= 0

if __name__ == '__main__':
    main()

