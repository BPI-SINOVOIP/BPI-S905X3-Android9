#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from host_controller import common

# The list of the kwargs key. can retrieve informations on the leased job.
_JOB_ATTR_LIST = [
    "build_id",
    "test_name",
    "shards",
    "serial",
    "build_target",
    "manifest_branch",
    "gsi_branch",
    "gsi_build_target",
    "test_branch",
    "test_build_target",
]


def EmitConsoleCommands(**kwargs):
    """Runs a common VTS-on-GSI or CTS-on-GSI test.

    This uses a given device branch information and automatically
    selects a GSI branch and a test branch.
    """
    result = []

    if not set(_JOB_ATTR_LIST).issubset(kwargs):
        missing_keys = [key for key in _JOB_ATTR_LIST if key not in kwargs]
        print("Leased job missing attribute(s): {}".format(
            ", ".join(missing_keys)))
        return None

    if isinstance(kwargs["build_target"], list):
        build_target = kwargs["build_target"][0]
    else:
        build_target = kwargs["build_target"]

    if "pab_account_id" in kwargs and kwargs["pab_account_id"]:
        pab_account_id = kwargs["pab_account_id"]
    else:
        pab_account_id = common._DEFAULT_ACCOUNT_ID_INTERNAL

    manifest_branch = kwargs["manifest_branch"]
    build_id = kwargs["build_id"]
    result.append(
        "fetch --type=pab --branch=%s --target=%s --artifact_name=%s-img-%s.zip "
        "--build_id=%s --account_id=%s" %
        (manifest_branch, build_target, build_target.split("-")[0], build_id
         if build_id != "latest" else "{build_id}", build_id, pab_account_id))

    result.append(
        "fetch --type=pab --branch=%s --target=%s --artifact_name=bootloader.img "
        "--build_id=%s --account_id=%s" % (manifest_branch, build_target,
                                           build_id, pab_account_id))

    result.append(
        "fetch --type=pab --branch=%s --target=%s --artifact_name=radio.img "
        "--build_id=%s --account_id=%s" % (manifest_branch, build_target,
                                           build_id, pab_account_id))

    if "gsi_branch" in kwargs and kwargs["gsi_branch"]:
        gsi = True
    else:
        gsi = False

    if gsi:
        if "gsi_build_id" in kwargs and kwargs["gsi_build_id"]:
            gsi_build_id = kwargs["gsi_build_id"]
        else:
            gsi_build_id = "latest"
        result.append(
            "fetch --type=pab --branch=%s --target=%s "
            "--artifact_name=aosp_arm64_ab-img-{build_id}.zip --build_id=%s" %
            (kwargs["gsi_branch"], kwargs["gsi_build_target"], gsi_build_id))
        if "gsi_pab_account_id" in kwargs and kwargs["gsi_pab_account_id"] != "":
            result[-1] += " --account_id=%s" % kwargs["gsi_pab_account_id"]

    if "test_build_id" in kwargs and kwargs["test_build_id"]:
        test_build_id = kwargs["test_build_id"]
    else:
        test_build_id = "latest"
    result.append("fetch --type=pab --branch=%s --target=%s "
                  "--artifact_name=android-vts.zip --build_id=%s" %
                  (kwargs["test_branch"], kwargs["test_build_target"],
                   test_build_id))
    if "test_pab_account_id" in kwargs and kwargs["test_pab_account_id"] != "":
        result[-1] += " --account_id=%s" % kwargs["test_pab_account_id"]

    result.append("info")
    if gsi:
        result.append("gsispl --version_from_path=boot.img")
        result.append("info")

    shards = int(kwargs["shards"])
    test_name = kwargs["test_name"].split("/")[-1]
    serials = kwargs["serial"]
    param = ""
    if "param" in kwargs and kwargs["param"]:
        param = " ".join(kwargs["param"])

    if shards > 1:
        sub_commands = []
        test_command = "test --keep-result -- %s --shards %d %s" % (
            test_name, shards, param)
        if shards <= len(serials):
            for shard_index in range(shards):
                new_cmd_list = []
                new_cmd_list.append(
                    "flash --current --serial %s" % serials[shard_index])
                test_command += " --serial %s" % serials[shard_index]
                sub_commands.append(new_cmd_list)
        result.append(sub_commands)
        result.append(test_command)
    else:
        result.append("flash --current --serial %s" % serials[0])
        if serials:
            result.append("test --keep-result -- %s --serial %s --shards %s %s" %
                          (test_name, ",".join(serials), shards, param))
        else:
            result.append("test --keep-result -- %s --shards %s %s" % (
                test_name, shards, param))

    if "retry_count" in kwargs:
        retry_count = int(kwargs["retry_count"])
        result.append("retry --count %d" % retry_count)

    result.append(
        "upload --src={result_full} --dest=gs://vts-report/{suite_plan}"
        "/{branch}/{target}/%s_{build_id}_{timestamp}/" % build_target)

    return result
