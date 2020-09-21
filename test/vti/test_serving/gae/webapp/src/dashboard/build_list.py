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

from webapp.src.handlers.base import BaseHandler
from webapp.src.proto import model


def ReadBuildInfo(target_branch=""):
    """Reads build information.

    Args:
        target_branch: string, to select a specific branch.

    Returns:
        a dict containing test build information,
        a dict containing device build information,
        a dict containing gsi build information.
    """
    build_query = model.BuildModel.query()
    builds = build_query.fetch()

    test_builds = {}
    device_builds = {}
    gsi_builds = {}

    if builds:
        for build in builds:
            if build.manifest_branch.startswith("git_oc-mr1"):
                m_branch = "O-MR1"
            elif build.manifest_branch.startswith("git_oc-"):
                m_branch = "O"
            elif build.manifest_branch.startswith("gcs"):
                m_branch = "GCS"
            else:
                m_branch = "P"

            if target_branch and target_branch != m_branch:
                continue

            if build.manifest_branch.startswith("git_"):
                build.manifest_branch = build.manifest_branch.replace("git_", "")

            if build.artifact_type == "test":
                if m_branch in test_builds:
                    test_builds[m_branch].append(build)
                else:
                    test_builds[m_branch] = [build]
            elif build.artifact_type == "device":
                if m_branch in device_builds:
                   device_builds[m_branch].append(build)
                else:
                   device_builds[m_branch] = [build]
            elif build.artifact_type == "gsi":
                if m_branch in gsi_builds:
                   gsi_builds[m_branch].append(build)
                else:
                   gsi_builds[m_branch] = [build]
            else:
                print("unknown artifact_type %s" % build.artifact_type)

    if test_builds:
        for m_branch in test_builds:
            test_builds[m_branch] = sorted(
                test_builds[m_branch], key=lambda x: x.build_id, reverse=True)
    if device_builds:
        for m_branch in device_builds:
            device_builds[m_branch] = sorted(
                device_builds[m_branch], key=lambda x: x.build_id, reverse=True)
    if gsi_builds:
        for m_branch in gsi_builds:
            gsi_builds[m_branch] = sorted(
                gsi_builds[m_branch], key=lambda x: x.build_id, reverse=True)
    return test_builds, device_builds, gsi_builds


class BuildPage(BaseHandler):
    """Main class for /build web page."""

    def get(self):
        """Generates an HTML page based on the build info kept in DB."""
        self.template = "build.html"

        target_branch = self.request.get("branch", default_value="")

        test_builds, device_builds, gsi_builds = ReadBuildInfo(target_branch)

        manifest_branch_keys =  list(set().union(
            test_builds.keys(), device_builds.keys(),
            gsi_builds.keys()))
        all_builds = {}
        for manifest_branch_key in manifest_branch_keys:
            all_builds[manifest_branch_key] = {}
            if manifest_branch_key in test_builds:
                all_builds[manifest_branch_key]["test"] = test_builds[
                    manifest_branch_key]
            else:
                all_builds[manifest_branch_key]["test"] = []
            if manifest_branch_key in device_builds:
                all_builds[manifest_branch_key]["device"] = device_builds[
                    manifest_branch_key]
            else:
                all_builds[manifest_branch_key]["device"] = []
            if manifest_branch_key in gsi_builds:
                all_builds[manifest_branch_key]["gsi"] = gsi_builds[
                    manifest_branch_key]
            else:
                all_builds[manifest_branch_key]["gsi"] = []

        template_values = {
            "all_builds": all_builds,
        }

        self.render(template_values)
