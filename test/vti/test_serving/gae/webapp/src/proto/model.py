#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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
#

from google.appengine.ext import ndb

from protorpc import messages


class BuildModel(ndb.Model):
    """A model for representing an individual build entry."""
    manifest_branch = ndb.StringProperty()
    build_id = ndb.StringProperty()
    build_target = ndb.StringProperty()
    build_type = ndb.StringProperty()
    artifact_type = ndb.StringProperty()
    artifacts = ndb.StringProperty(repeated=True)
    timestamp = ndb.DateTimeProperty(auto_now=False)
    signed = ndb.BooleanProperty()


class BuildInfoMessage(messages.Message):
    """A message for representing an individual build entry."""
    manifest_branch = messages.StringField(1)
    build_id = messages.StringField(2)
    build_target = messages.StringField(3)
    build_type = messages.StringField(4)
    artifact_type = messages.StringField(5)
    artifacts = messages.StringField(6, repeated=True)
    signed = messages.BooleanField(7)


class ScheduleModel(ndb.Model):
    """A model for representing an individual schedule entry."""
    # schedule name for green build schedule, optional.
    name = ndb.StringProperty()
    schedule_type = ndb.StringProperty()

    # device image information
    build_storage_type = ndb.IntegerProperty()
    manifest_branch = ndb.StringProperty()
    build_target = ndb.StringProperty()  # type:name
    device_pab_account_id = ndb.StringProperty()
    require_signed_device_build = ndb.BooleanProperty()

    # GSI information
    gsi_storage_type = ndb.IntegerProperty()
    gsi_branch = ndb.StringProperty()
    gsi_build_target = ndb.StringProperty()
    gsi_pab_account_id = ndb.StringProperty()

    # test suite information
    test_storage_type = ndb.IntegerProperty()
    test_branch = ndb.StringProperty()
    test_build_target = ndb.StringProperty()
    test_pab_account_id = ndb.StringProperty()

    test_name = ndb.StringProperty()
    period = ndb.IntegerProperty()
    schedule = ndb.StringProperty()
    priority = ndb.StringProperty()
    device = ndb.StringProperty(repeated=True)
    shards = ndb.IntegerProperty()
    param = ndb.StringProperty(repeated=True)
    timestamp = ndb.DateTimeProperty(auto_now=False)
    retry_count = ndb.IntegerProperty()


class ScheduleInfoMessage(messages.Message):
    """A message for representing an individual schedule entry."""
    # schedule name for green build schedule, optional.
    name = messages.StringField(16)
    schedule_type = messages.StringField(19)

    # device image information
    build_storage_type = messages.IntegerField(21)
    manifest_branch = messages.StringField(1)
    build_target = messages.StringField(2)
    device_pab_account_id = messages.StringField(17)
    require_signed_device_build = messages.BooleanField(20)

    # GSI information
    gsi_storage_type = messages.IntegerField(22)
    gsi_branch = messages.StringField(9)
    gsi_build_target = messages.StringField(10)
    gsi_pab_account_id = messages.StringField(11)

    # test suite information
    test_storage_type = messages.IntegerField(23)
    test_branch = messages.StringField(12)
    test_build_target = messages.StringField(13)
    test_pab_account_id = messages.StringField(14)

    test_name = messages.StringField(3)
    period = messages.IntegerField(4)
    schedule = messages.StringField(18)
    priority = messages.StringField(5)
    device = messages.StringField(6, repeated=True)
    shards = messages.IntegerField(7)
    param = messages.StringField(8, repeated=True)
    retry_count = messages.IntegerField(15)


class LabModel(ndb.Model):
    """A model for representing an individual lab entry."""
    name = ndb.StringProperty()
    owner = ndb.StringProperty()
    hostname = ndb.StringProperty()
    ip = ndb.StringProperty()
    # devices is a comma-separated list of serial=product pairs
    devices = ndb.StringProperty()
    timestamp = ndb.DateTimeProperty(auto_now=False)


class LabDeviceInfoMessage(messages.Message):
    """A message for representing an individual lab host's device entry."""
    serial = messages.StringField(1, repeated=False)
    product = messages.StringField(2, repeated=False)


class LabHostInfoMessage(messages.Message):
    """A message for representing an individual lab's host entry."""
    hostname = messages.StringField(1, repeated=False)
    ip = messages.StringField(2, repeated=False)
    script = messages.StringField(3)
    device = messages.MessageField(
        LabDeviceInfoMessage, 4, repeated=True)


class LabInfoMessage(messages.Message):
    """A message for representing an individual lab entry."""
    name = messages.StringField(1)
    owner = messages.StringField(2)
    host = messages.MessageField(
        LabHostInfoMessage, 3, repeated=True)


class DeviceModel(ndb.Model):
    """A model for representing an individual device entry."""
    hostname = ndb.StringProperty()
    product = ndb.StringProperty()
    serial = ndb.StringProperty()
    status = ndb.IntegerProperty()
    scheduling_status = ndb.IntegerProperty()
    timestamp = ndb.DateTimeProperty(auto_now=False)


class DeviceInfoMessage(messages.Message):
    """A message for representing an individual host's device entry."""
    serial = messages.StringField(1)
    product = messages.StringField(2)
    status = messages.IntegerField(3)
    scheduling_status = messages.IntegerField(4)


class HostInfoMessage(messages.Message):
    """A message for representing an individual host entry."""
    hostname = messages.StringField(1)
    devices = messages.MessageField(
        DeviceInfoMessage, 2, repeated=True)


class JobModel(ndb.Model):
    """A model for representing an individual job entry."""
    hostname = ndb.StringProperty()
    priority = ndb.StringProperty()
    test_name = ndb.StringProperty()
    require_signed_device_build = ndb.BooleanProperty()
    device = ndb.StringProperty()
    serial = ndb.StringProperty(repeated=True)

    # device image information
    build_storage_type = ndb.IntegerProperty()
    manifest_branch = ndb.StringProperty()
    build_target = ndb.StringProperty()
    build_id = ndb.StringProperty()
    pab_account_id = ndb.StringProperty()

    shards = ndb.IntegerProperty()
    param = ndb.StringProperty(repeated=True)
    status = ndb.IntegerProperty()
    period = ndb.IntegerProperty()

    # GSI information
    gsi_storage_type = ndb.IntegerProperty()
    gsi_branch = ndb.StringProperty()
    gsi_build_target = ndb.StringProperty()
    gsi_build_id = ndb.StringProperty()
    gsi_pab_account_id = ndb.StringProperty()

    # test suite information
    test_storage_type = ndb.IntegerProperty()
    test_branch = ndb.StringProperty()
    test_build_target = ndb.StringProperty()
    test_build_id = ndb.StringProperty()
    test_pab_account_id = ndb.StringProperty()

    timestamp = ndb.DateTimeProperty(auto_now=False)
    heartbeat_stamp = ndb.DateTimeProperty(auto_now=False)
    retry_count = ndb.IntegerProperty()

    infra_log_url = ndb.StringProperty()


class JobMessage(messages.Message):
    """A message for representing an individual job entry."""
    hostname = messages.StringField(1)
    priority = messages.StringField(2)
    test_name = messages.StringField(3)
    require_signed_device_build = messages.BooleanField(23)
    device = messages.StringField(4)
    serial = messages.StringField(5, repeated=True)

    # device image information
    build_storage_type = messages.IntegerField(25)
    manifest_branch = messages.StringField(6)
    build_target = messages.StringField(7)
    build_id = messages.StringField(10)
    pab_account_id = messages.StringField(20)

    shards = messages.IntegerField(8)
    param = messages.StringField(9, repeated=True)
    status = messages.IntegerField(11)
    period = messages.IntegerField(12)

    # GSI information
    gsi_storage_type = messages.IntegerField(26)
    gsi_branch = messages.StringField(13)
    gsi_build_target = messages.StringField(14)
    gsi_build_id = messages.StringField(21)
    gsi_pab_account_id = messages.StringField(15)

    # test suite information
    test_storage_type = messages.IntegerField(27)
    test_branch = messages.StringField(16)
    test_build_target = messages.StringField(17)
    test_build_id = messages.StringField(22)
    test_pab_account_id = messages.StringField(18)

    retry_count = messages.IntegerField(19)

    infra_log_url = messages.StringField(24)


class ReturnCodeMessage(messages.Enum):
    """Enum for default return code."""
    SUCCESS = 0
    FAIL = 1


class DefaultResponse(messages.Message):
    """A default response proto message."""
    return_code = messages.EnumField(ReturnCodeMessage, 1)


class JobLeaseResponse(messages.Message):
    """A job lease response proto message."""
    return_code = messages.EnumField(ReturnCodeMessage, 1)
    jobs = messages.MessageField(JobMessage, 2, repeated=True)
