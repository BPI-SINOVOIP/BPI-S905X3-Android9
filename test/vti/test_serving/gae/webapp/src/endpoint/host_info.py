# Copyright 2017 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Host Info APIs implemented using Google Cloud Endpoints."""

import datetime
import endpoints

from protorpc import remote

from google.appengine.api import users

from webapp.src import vtslab_status as Status
from webapp.src.proto import model

HOST_INFO_RESOURCE = endpoints.ResourceContainer(model.HostInfoMessage)

# Product type name for null device.
_NULL_DEVICE_PRODUCT_TYPE = "null"


def AddNullDevices(hostname, null_device_count):
    """Adds null devices to DeviceModel data store.

    Args:
        hostname: string, the host name.
        null_device_count: integer, the number of null devices.
    """
    device_query = model.DeviceModel.query(
        model.DeviceModel.hostname == hostname,
        model.DeviceModel.product == _NULL_DEVICE_PRODUCT_TYPE
    )
    null_devices = device_query.fetch()
    existing_null_device_count = len(null_devices)

    if existing_null_device_count < null_device_count:
        for _ in range(null_device_count - existing_null_device_count):
            device = model.DeviceModel()
            device.hostname = hostname
            device.serial = "n/a"
            device.product = _NULL_DEVICE_PRODUCT_TYPE
            device.status = Status.DEVICE_STATUS_DICT["ready"]
            device.scheduling_status = Status.DEVICE_SCHEDULING_STATUS_DICT[
                "free"]
            device.timestamp = datetime.datetime.now()
            device.put()


@endpoints.api(name='host_info', version='v1')
class HostInfoApi(remote.Service):
    """Endpoint API for host_info."""

    @endpoints.method(
        HOST_INFO_RESOURCE,
        model.DefaultResponse,
        path='set',
        http_method='POST',
        name='set')
    def set(self, request):
        """Sets the host info based on the `request`."""
        if users.get_current_user():
            username = users.get_current_user().email()
        else:
            username = "anonymous"

        for request_device in request.devices:
            device_query = model.DeviceModel.query(
                model.DeviceModel.serial == request_device.serial
            )
            existing_device = device_query.fetch()
            if existing_device:
                device = existing_device[0]
            else:
                device = model.DeviceModel()
                device.serial = request_device.serial
                device.scheduling_status = Status.DEVICE_SCHEDULING_STATUS_DICT[
                    "free"]
            device.username = username
            device.hostname = request.hostname
            device.product = request_device.product
            device.status = request_device.status
            device.timestamp = datetime.datetime.now()
            device.put()

        return model.DefaultResponse(
            return_code=model.ReturnCodeMessage.SUCCESS)
