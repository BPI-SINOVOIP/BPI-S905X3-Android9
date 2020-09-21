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

"""Build Info APIs implemented using Google Cloud Endpoints."""

import datetime
import endpoints
import logging

from protorpc import remote

from webapp.src.proto import model


BUILD_INFO_RESOURCE = endpoints.ResourceContainer(
    model.BuildInfoMessage)


@endpoints.api(name="build_info", version="v1")
class BuildInfoApi(remote.Service):
    """Endpoint API for build_info."""

    @endpoints.method(
        BUILD_INFO_RESOURCE,
        model.DefaultResponse,
        path="set",
        http_method="POST",
        name="set")
    def set(self, request):
        """Sets the build info based on the `request`."""
        build_query = model.BuildModel.query(
            model.BuildModel.build_id == request.build_id,
            model.BuildModel.build_target == request.build_target,
            model.BuildModel.build_type == request.build_type
        )
        existing_builds = build_query.fetch()

        if existing_builds and len(existing_builds) > 1:
            logging.warning("Duplicated builds found for [build_id]{} "
                            "[build_target]{} [build_type]{}".format(
                request.build_id, request.build_target, request.build_type))

        if request.signed and existing_builds:
            # only signed builds need to overwrite the exist entities.
            build = existing_builds[0]
        elif not existing_builds:
            build = model.BuildModel()
        else:
            # the same build existed and request is not signed build.
            build = None

        if build:
            build.manifest_branch = request.manifest_branch
            build.build_id = request.build_id
            build.build_target = request.build_target
            build.build_type = request.build_type
            build.artifact_type = request.artifact_type
            build.artifacts = request.artifacts
            build.timestamp = datetime.datetime.now()
            build.signed = request.signed
            build.put()

        return model.DefaultResponse(
            return_code=model.ReturnCodeMessage.SUCCESS)
