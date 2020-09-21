#!/usr/bin/env python
#
# Copyright (C) 2015 The Android Open Source Project
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
"""Builds the Android Clang toolchain."""
import argparse
import glob
import logging
import multiprocessing
import os
import pprint
import subprocess
import sys

import version


# Disable all the "too many/few methods/parameters" warnings and the like.
# pylint: disable=design

# Disable lint warnings for todo comments and the like.
# pylint: disable=fixme

# TODO: Add docstrings?
# pylint: disable=missing-docstring


THIS_DIR = os.path.realpath(os.path.dirname(__file__))
ORIG_ENV = dict(os.environ)


class Config(object):
    """Container for global configuration options."""

    # Set True to skip all actions (log only). Controlled by --dry-run.
    dry_run = False


def logger():
    """Returns the default logger for the module."""
    return logging.getLogger(__name__)


def android_path(*args):
    return os.path.realpath(os.path.join(THIS_DIR, '../..', *args))


def build_path(*args):
    # Our multistage build directories will be placed under OUT_DIR if it is in
    # the environment. By default they will be placed under
    # $ANDROID_BUILD_TOP/out.
    top_out = ORIG_ENV.get('OUT_DIR', 'out')
    return os.path.join(top_out, *args)


def short_version():
    return '.'.join([version.major, version.minor])


def long_version():
    return '.'.join([version.major, version.minor, version.patch])


def check_call(cmd, *args, **kwargs):
    """Proxy for subprocess.check_call with logging and dry-run support."""
    import subprocess
    logger().info('check_call: %s', ' '.join(cmd))
    if 'env' in kwargs:
        # Rather than dump the whole environment to the terminal every time,
        # just print the difference between this call and our environment.
        # Note that this will not include environment that was *removed* from
        # os.environ.
        extra_env = dict(set(kwargs['env'].items()) - set(os.environ.items()))
        if len(extra_env) > 0:
            logger().info('check_call additional env:\n%s',
                          pprint.pformat(extra_env))
    if not Config.dry_run:
        subprocess.check_call(cmd, *args, **kwargs)


def install_file(src, dst):
    """Proxy for shutil.copy2 with logging and dry-run support."""
    import shutil
    logger().info('copy %s %s', src, dst)
    if not Config.dry_run:
        shutil.copy2(src, dst)


def install_directory(src, dst):
    """Proxy for shutil.copytree with logging and dry-run support."""
    import shutil
    logger().info('copytree %s %s', src, dst)
    if not Config.dry_run:
        shutil.copytree(src, dst)


def rmtree(path):
    """Proxy for shutil.rmtree with logging and dry-run support."""
    import shutil
    logger().info('rmtree %s', path)
    if not Config.dry_run:
        shutil.rmtree(path)


def rename(src, dst):
    """Proxy for os.rename with logging and dry-run support."""
    logger().info('rename %s %s', src, dst)
    if not Config.dry_run:
        os.rename(src, dst)


def makedirs(path):
    """Proxy for os.makedirs with logging and dry-run support."""
    logger().info('makedirs %s', path)
    if not Config.dry_run:
        os.makedirs(path)


def symlink(src, dst):
    """Proxy for os.symlink with logging and dry-run support."""
    logger().info('symlink %s %s', src, dst)
    if not Config.dry_run:
        os.symlink(src, dst)


def build(out_dir, prebuilts_path=None, prebuilts_version=None,
          build_all_clang_tools=None, build_all_llvm_tools=None,
          debug_clang=None, max_jobs=multiprocessing.cpu_count()):
    products = (
        'aosp_arm',
        'aosp_arm64',
        'aosp_mips',
        'aosp_mips64',
        'aosp_x86',
        'aosp_x86_64',
    )
    for product in products:
        build_product(out_dir, product, prebuilts_path, prebuilts_version,
                      build_all_clang_tools, build_all_llvm_tools, debug_clang,
                      max_jobs)


def build_product(out_dir, product, prebuilts_path, prebuilts_version,
                  build_all_clang_tools, build_all_llvm_tools, debug_clang,
                  max_jobs):
    env = dict(ORIG_ENV)
    env['DISABLE_LLVM_DEVICE_BUILDS'] = 'true'
    env['DISABLE_RELOCATION_PACKER'] = 'true'
    env['FORCE_BUILD_LLVM_COMPONENTS'] = 'true'
    env['FORCE_BUILD_SANITIZER_SHARED_OBJECTS'] = 'true'
    env['OUT_DIR'] = out_dir
    env['SKIP_LLVM_TESTS'] = 'true'
    env['SOONG_ALLOW_MISSING_DEPENDENCIES'] = 'true'
    env['TARGET_BUILD_VARIANT'] = 'userdebug'
    env['TARGET_PRODUCT'] = product

    if debug_clang:
        env['FORCE_BUILD_LLVM_DEBUG'] = 'true'
        env['FORCE_BUILD_LLVM_DISABLE_NDEBUG'] = 'true'

    overrides = []
    if prebuilts_path is not None:
        overrides.append('LLVM_PREBUILTS_BASE={}'.format(prebuilts_path))
    if prebuilts_version is not None:
        overrides.append('LLVM_PREBUILTS_VERSION={}'.format(prebuilts_version))

    # Use at least 1 and at most all available CPUs (sanitize the user input).
    jobs_arg = '-j{}'.format(
        max(1, min(max_jobs, multiprocessing.cpu_count())))

    targets = ['clang-toolchain-minimal']
    if build_all_clang_tools:
        targets += ['clang-toolchain-full']
    if build_all_llvm_tools:
        targets += ['llvm-tools']
    check_call(['make', jobs_arg] + overrides + targets,
               cwd=android_path(), env=env)


def package_toolchain(build_dir, build_name, host, dist_dir):
    package_name = 'clang-' + build_name
    install_host_dir = build_path('install', host)
    install_dir = os.path.join(install_host_dir, package_name)

    # Remove any previously installed toolchain so it doesn't pollute the
    # build.
    if os.path.exists(install_host_dir):
        rmtree(install_host_dir)

    install_toolchain(build_dir, install_dir, host, True)

    version_file_path = os.path.join(install_dir, 'AndroidVersion.txt')
    with open(version_file_path, 'w') as version_file:
        version_file.write('{}.{}.{}\n'.format(
            version.major, version.minor, version.patch))

    tarball_name = package_name + '-' + host
    package_path = os.path.join(dist_dir, tarball_name) + '.tar.bz2'
    logger().info('Packaging %s', package_path)
    args = [
        'tar', '-cjC', install_host_dir, '-f', package_path, package_name
    ]
    check_call(args)


def install_minimal_toolchain(build_dir, install_dir, host, strip):
    install_built_host_files(build_dir, install_dir, host, strip, minimal=True)
    install_headers(build_dir, install_dir, host)
    install_sanitizers(build_dir, install_dir, host)


def install_toolchain(build_dir, install_dir, host, strip):
    install_built_host_files(build_dir, install_dir, host, strip)
    install_compiler_wrapper(install_dir, host)
    install_sanitizer_scripts(install_dir)
    install_scan_scripts(install_dir)
    install_analyzer_scripts(install_dir)
    install_headers(build_dir, install_dir, host)
    install_profile_rt(build_dir, install_dir, host)
    install_sanitizers(build_dir, install_dir, host)
    install_sanitizer_tests(build_dir, install_dir, host)
    install_libomp(build_dir, install_dir, host)
    install_license_files(install_dir)
    install_repo_prop(install_dir)


def get_built_host_files(host, minimal):
    is_windows = host.startswith('windows')
    is_darwin = host.startswith('darwin-x86')
    bin_ext = '.exe' if is_windows else ''

    if is_windows:
        lib_ext = '.dll'
    elif is_darwin:
        lib_ext = '.dylib'
    else:
        lib_ext = '.so'

    built_files = [
        'bin/clang' + bin_ext,
        'bin/clang++' + bin_ext,
    ]
    if not is_windows:
        built_files.extend(['lib64/libc++' + lib_ext])

    if minimal:
        return built_files

    built_files.extend([
        'bin/clang-format' + bin_ext,
        'bin/clang-tidy' + bin_ext,
    ])

    if is_windows:
        built_files.extend([
            'bin/clang_32' + bin_ext,
        ])
    else:
        built_files.extend([
            'bin/FileCheck' + bin_ext,
            'bin/llvm-as' + bin_ext,
            'bin/llvm-dis' + bin_ext,
            'bin/llvm-link' + bin_ext,
            'bin/llvm-symbolizer' + bin_ext,
            'lib64/libLLVM' + lib_ext,
            'lib64/LLVMgold' + lib_ext,
        ])
    return built_files


def install_built_host_files(build_dir, install_dir, host, strip, minimal=None):
    built_files = get_built_host_files(host, minimal)
    for built_file in built_files:
        dirname = os.path.dirname(built_file)
        install_path = os.path.join(install_dir, dirname)
        if not os.path.exists(install_path):
            makedirs(install_path)

        built_path = os.path.join(build_dir, 'host', host, built_file)
        install_file(built_path, install_path)

        file_name = os.path.basename(built_file)

        # Only strip bin files (not libs) on darwin.
        is_darwin = host.startswith('darwin-x86')
        if strip and (not is_darwin or built_file.startswith('bin/')):
            check_call(['strip', os.path.join(install_path, file_name)])


def install_sanitizer_scripts(install_dir):
    script_path = android_path(
        'external/compiler-rt/lib/asan/scripts/asan_device_setup')
    install_file(script_path, os.path.join(install_dir, 'bin'))


def install_analyzer_scripts(install_dir):
    """Create and install bash scripts for invoking Clang for analysis."""
    analyzer_text = (
        '#!/bin/bash\n'
        'if [ "$1" != "-cc1" ]; then\n'
        '    `dirname $0`/../clang{clang_suffix} -target {target} "$@"\n'
        'else\n'
        '    # target/triple already spelled out.\n'
        '    `dirname $0`/../clang{clang_suffix} "$@"\n'
        'fi\n'
    )

    arch_target_pairs = (
        ('arm64-v8a', 'aarch64-none-linux-android'),
        ('armeabi', 'armv5te-none-linux-androideabi'),
        ('armeabi-v7a', 'armv7-none-linux-androideabi'),
        ('armeabi-v7a-hard', 'armv7-none-linux-androideabi'),
        ('mips', 'mipsel-none-linux-android'),
        ('mips64', 'mips64el-none-linux-android'),
        ('x86', 'i686-none-linux-android'),
        ('x86_64', 'x86_64-none-linux-android'),
    )

    for arch, target in arch_target_pairs:
        arch_path = os.path.join(install_dir, 'bin', arch)
        makedirs(arch_path)

        analyzer_file_path = os.path.join(arch_path, 'analyzer')
        logger().info('Creating %s', analyzer_file_path)
        with open(analyzer_file_path, 'w') as analyzer_file:
            analyzer_file.write(
                analyzer_text.format(clang_suffix='', target=target))
        subprocess.check_call(['chmod', 'a+x', analyzer_file_path])

        analyzerpp_file_path = os.path.join(arch_path, 'analyzer++')
        logger().info('Creating %s', analyzerpp_file_path)
        with open(analyzerpp_file_path, 'w') as analyzerpp_file:
            analyzerpp_file.write(
                analyzer_text.format(clang_suffix='++', target=target))
        subprocess.check_call(['chmod', 'a+x', analyzerpp_file_path])


def install_scan_scripts(install_dir):
    tools_install_dir = os.path.join(install_dir, 'tools')
    makedirs(tools_install_dir)
    tools = ('scan-build', 'scan-view')
    tools_dir = android_path('external/clang/tools')
    for tool in tools:
        tool_path = os.path.join(tools_dir, tool)
        install_path = os.path.join(install_dir, 'tools', tool)
        install_directory(tool_path, install_path)


def install_headers(build_dir, install_dir, host):
    def should_copy(path):
        if os.path.basename(path) in ('Makefile', 'CMakeLists.txt'):
            return False
        _, ext = os.path.splitext(path)
        if ext == '.mk':
            return False
        return True

    headers_src = android_path('external/clang/lib/Headers')
    headers_dst = os.path.join(
        install_dir, 'lib64/clang', short_version(), 'include')
    makedirs(headers_dst)
    for header in os.listdir(headers_src):
        if not should_copy(header):
            continue
        install_file(os.path.join(headers_src, header), headers_dst)

    install_file(android_path('bionic/libc/include/stdatomic.h'), headers_dst)

    # arm_neon.h gets produced as part of external/clang/Android.bp.
    # We must bundle the resulting file as part of the official Clang headers.
    arm_neon_h = os.path.join(
        build_dir, 'soong/.intermediates/external/clang/clang-gen-arm-neon/gen/clang/Basic/arm_neon.h')
    install_file(arm_neon_h, headers_dst)

    symlink(short_version(),
            os.path.join(install_dir, 'lib64/clang', long_version()))


def install_profile_rt(build_dir, install_dir, host):
    lib_dir = os.path.join(
        install_dir, 'lib64/clang', short_version(), 'lib/linux')
    makedirs(lib_dir)

    install_target_profile_rt(build_dir, lib_dir)

    # We only support profiling libs for Linux and Android.
    if host == 'linux-x86':
        install_host_profile_rt(build_dir, host, lib_dir)


def install_target_profile_rt(build_dir, lib_dir):
    product_to_arch = {
        'generic': 'arm',
        'generic_arm64': 'aarch64',
        'generic_mips': 'mipsel',
        'generic_mips64': 'mips64el',
        'generic_x86': 'i686',
        'generic_x86_64': 'x86_64',
    }

    for product, arch in product_to_arch.items():
        product_dir = os.path.join(build_dir, 'target/product', product)
        static_libs = os.path.join(product_dir, 'obj/STATIC_LIBRARIES')
        built_lib = os.path.join(
            static_libs, 'libprofile_rt_intermediates/libprofile_rt.a')
        lib_name = 'libclang_rt.profile-{}-android.a'.format(arch)
        install_file(built_lib, os.path.join(lib_dir, lib_name))


def install_host_profile_rt(build_dir, host, lib_dir):
    arch_to_obj_dir = {
        'i686': 'obj32',
        'x86_64': 'obj',
    }

    for arch, obj_dir in arch_to_obj_dir.items():
        static_libs = os.path.join(
            build_dir, 'host', host, obj_dir, 'STATIC_LIBRARIES')
        built_lib = os.path.join(
            static_libs, 'libprofile_rt_intermediates/libprofile_rt.a')
        lib_name = 'libclang_rt.profile-{}.a'.format(arch)
        install_file(built_lib, os.path.join(lib_dir, lib_name))


def install_libomp(build_dir, install_dir, host):
    # libomp is not built for Darwin
    if host == 'darwin-x86':
        return

    lib_dir = os.path.join(
        install_dir, 'lib64/clang', short_version(), 'lib/linux')
    if not os.path.isdir(lib_dir):
        makedirs(lib_dir)

    product_to_arch = {
        'generic': 'arm',
        'generic_arm64': 'arm64',
        'generic_x86': 'x86',
        'generic_x86_64': 'x86_64',
    }

    for product, arch in product_to_arch.items():
        module = 'libomp-' + arch
        product_dir = os.path.join(build_dir, 'target/product', product)
        shared_libs = os.path.join(product_dir, 'obj/SHARED_LIBRARIES')
        built_lib = os.path.join(
            shared_libs,
            '{}_intermediates/PACKED/{}.so'.format(module, module))
        install_file(built_lib, os.path.join(lib_dir, module + '.so'))


def install_sanitizers(build_dir, install_dir, host):
    headers_src = android_path('external/compiler-rt/include/sanitizer')
    clang_lib = os.path.join(install_dir, 'lib64/clang', short_version())
    headers_dst = os.path.join(clang_lib, 'include/sanitizer')
    lib_dst = os.path.join(clang_lib, 'lib/linux')
    install_directory(headers_src, headers_dst)

    if not os.path.exists(lib_dst):
        makedirs(lib_dst)

    if host == 'linux-x86':
        install_host_sanitizers(build_dir, host, lib_dst)

    # Tuples of (product, arch)
    product_to_arch = (
        ('generic', 'arm'),
        ('generic_arm64', 'aarch64'),
        ('generic_x86', 'i686'),
        ('generic_mips', 'mips'),
        ('generic_mips64', 'mips64'),
    )

    sanitizers = ('asan', 'ubsan_standalone')

    for product, arch in product_to_arch:
        for sanitizer in sanitizers:
            module = 'libclang_rt.{}-{}-android'.format(sanitizer, arch)
            product_dir = os.path.join(build_dir, 'target/product', product)
            lib_dir = os.path.join(product_dir, 'obj/SHARED_LIBRARIES',
                                   '{}_intermediates'.format(module))
            lib_name = '{}.so'.format(module)
            built_lib = os.path.join(lib_dir, 'PACKED', lib_name)
            install_file(built_lib, lib_dst)


# Also install the asan_test binaries. We need to do this because the
# platform sources for compiler-rt are potentially different from our
# toolchain sources. The only way to ensure that this test builds
# correctly is to make it a prebuilt based on our latest toolchain
# sources. Note that this is only created/compiled by the previous
# stage (usually stage1) compiler. We are not doing a subsequent
# compile with our stage2 binaries to construct any further
# device-targeted objects.
def install_sanitizer_tests(build_dir, install_dir, host):
    # Tuples of (product, arch)
    product_to_arch = (
        ('generic', 'arm'),
        ('generic_arm64', 'aarch64'),
        ('generic_x86', 'i686'),
        ('generic_mips', 'mips'),
        ('generic_mips64', 'mips64'),
    )

    for product, arch in product_to_arch:
        product_dir = os.path.join(build_dir, 'target/product', product)
        test_module = 'asan_test'
        test_dir = os.path.join(product_dir, 'obj/EXECUTABLES',
                                '{}_intermediates'.format(test_module))
        built_test = os.path.join(test_dir, 'PACKED', test_module)
        test_dst = os.path.join(install_dir, 'test', arch, 'bin')
        makedirs(test_dst)
        install_file(built_test, test_dst)


def install_host_sanitizers(build_dir, host, lib_dst):
    # Tuples of (name, multilib).
    libs = (
        ('asan', True),
        ('asan_cxx', True),
        ('ubsan_standalone', True),
        ('ubsan_standalone_cxx', True),
        ('tsan', False),
        ('tsan_cxx', False),
    )

    obj32 = os.path.join(build_dir, 'host', host, 'obj32/STATIC_LIBRARIES')
    obj64 = os.path.join(build_dir, 'host', host, 'obj/STATIC_LIBRARIES')
    for lib, is_multilib in libs:
        built_lib_name = 'lib{}.a'.format(lib)

        obj64_dir = os.path.join(obj64, 'lib{}_intermediates'.format(lib))
        lib64_name = 'libclang_rt.{}-x86_64.a'.format(lib)
        built_lib64 = os.path.join(obj64_dir, built_lib_name)
        install_file(built_lib64, os.path.join(lib_dst, lib64_name))
        if is_multilib:
            obj32_dir = os.path.join(obj32, 'lib{}_intermediates'.format(lib))
            lib32_name = 'libclang_rt.{}-i686.a'.format(lib)
            built_lib32 = os.path.join(obj32_dir, built_lib_name)
            install_file(built_lib32, os.path.join(lib_dst, lib32_name))


def install_license_files(install_dir):
    projects = (
        'clang',
        'clang-tools-extra',
        'compiler-rt',
        'libcxx',
        'libcxxabi',
        'libunwind_llvm',
        'llvm',
        'openmp_llvm'
    )

    notices = []
    for project in projects:
        project_path = android_path('external', project)
        license_pattern = os.path.join(project_path, 'MODULE_LICENSE_*')
        for license_file in glob.glob(license_pattern):
            install_file(license_file, install_dir)
        with open(os.path.join(project_path, 'NOTICE')) as notice_file:
            notices.append(notice_file.read())
    with open(os.path.join(install_dir, 'NOTICE'), 'w') as notice_file:
        notice_file.write('\n'.join(notices))


def install_repo_prop(install_dir):
    file_name = 'repo.prop'

    dist_dir = os.environ.get('DIST_DIR')
    if dist_dir is not None:
        dist_repo_prop = os.path.join(dist_dir, file_name)
        install_file(dist_repo_prop, install_dir)
    else:
        out_file = os.path.join(install_dir, file_name)
        with open(out_file, 'w') as prop_file:
            cmd = [
                'repo', 'forall', '-c',
                'echo $REPO_PROJECT $(git rev-parse HEAD)',
            ]
            check_call(cmd, stdout=prop_file)


def install_compiler_wrapper(install_dir, host):
    is_windows = host.startswith('windows')
    bin_ext = '.exe' if is_windows else ''

    built_files = [
        'bin/clang' + bin_ext,
        'bin/clang++' + bin_ext,
    ]

    if is_windows:
        built_files.extend([
            'bin/clang_32' + bin_ext,
        ])

    wrapper_dir = android_path('external/clang')
    wrapper = os.path.join(wrapper_dir, 'compiler_wrapper')

    for built_file in built_files:
        old_file = os.path.join(install_dir, built_file)
        new_file = os.path.join(install_dir, built_file + ".real")
        rename(old_file, new_file)
        install_file(wrapper, old_file)


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('-j', action='store', dest='jobs', type=int,
                        default=multiprocessing.cpu_count(),
                        help='Specify number of executed jobs')

    parser.add_argument(
        '--build-name', default='dev', help='Release name for the package.')
    parser.add_argument(
        '--dry-run', action='store_true', default=False,
        help='Skip running commands; just print.')
    parser.add_argument(
        '-v', '--verbose', action='store_true', default=False,
        help='Print debug output.')

    multi_stage_group = parser.add_mutually_exclusive_group()
    multi_stage_group.add_argument(
        '--multi-stage', action='store_true', default=True,
        help='Perform multi-stage build (enabled by default).')
    multi_stage_group.add_argument(
        '--no-multi-stage', action='store_false', dest='multi_stage',
        help='Do not perform multi-stage build.')

    build_all_llvm_tools_group = parser.add_mutually_exclusive_group()
    build_all_llvm_tools_group.add_argument(
        '--build-all-llvm-tools', action='store_true', default=True,
        help='Build all the LLVM tools/utilities.')
    build_all_llvm_tools_group.add_argument(
        '--no-build-all-llvm-tools', action='store_false',
        dest='build_all_llvm_tools',
        help='Do not build all the LLVM tools/utilities.')

    build_debug_clang_group = parser.add_mutually_exclusive_group()
    build_debug_clang_group.add_argument(
        '--debug-clang', action='store_true', default=True,
        help='Also generate a debug version of clang (enabled by default).')
    build_debug_clang_group.add_argument(
        '--no-debug-clang', action='store_false',
        dest='debug_clang',
        help='Skip generating a debug version of clang.')

    return parser.parse_args()


def main():
    args = parse_args()
    log_level = logging.INFO
    if args.verbose:
        log_level = logging.DEBUG
    logging.basicConfig(level=log_level)

    logger().info('chdir %s', android_path())
    os.chdir(android_path())

    Config.dry_run = args.dry_run

    if sys.platform.startswith('linux'):
        hosts = ['linux-x86', 'windows-x86']
    elif sys.platform == 'darwin':
        hosts = ['darwin-x86']
    else:
        raise RuntimeError('Unsupported host: {}'.format(sys.platform))

    stage_1_out_dir = build_path('stage1')

    # For a multi-stage build, build a minimum clang for the first stage that is
    # just enough to build the second stage.
    is_stage1_final = not args.multi_stage
    build(out_dir=stage_1_out_dir,
          build_all_clang_tools=is_stage1_final,
          build_all_llvm_tools=(is_stage1_final and args.build_all_llvm_tools),
          max_jobs=args.jobs)
    final_out_dir = stage_1_out_dir
    if args.multi_stage:
        stage_1_install_dir = build_path('stage1-install')
        for host in hosts:
            package_name = 'clang-' + args.build_name
            install_host_dir = os.path.join(stage_1_install_dir, host)
            install_dir = os.path.join(install_host_dir, package_name)

            # Remove any previously installed toolchain so it doesn't pollute
            # the build.
            if os.path.exists(install_host_dir):
                rmtree(install_host_dir)

            install_minimal_toolchain(stage_1_out_dir, install_dir, host, True)

        stage_2_out_dir = build_path('stage2')
        build(out_dir=stage_2_out_dir, prebuilts_path=stage_1_install_dir,
              prebuilts_version=package_name,
              build_all_clang_tools=True,
              build_all_llvm_tools=args.build_all_llvm_tools,
              max_jobs=args.jobs)
        final_out_dir = stage_2_out_dir

        if args.debug_clang:
            debug_clang_out_dir = build_path('debug')
            build(out_dir=debug_clang_out_dir,
                  prebuilts_path=stage_1_install_dir,
                  prebuilts_version=package_name,
                  build_all_clang_tools=True,
                  build_all_llvm_tools=args.build_all_llvm_tools,
                  debug_clang=args.debug_clang,
                  max_jobs=args.jobs)
            # Install the actual debug toolchain somewhere, so it is easier to
            # use.
            debug_package_name = 'clang-debug'
            base_debug_install_dir = build_path('debug-install')
            for host in hosts:
                debug_install_host_dir = os.path.join(
                    base_debug_install_dir, host)
                debug_install_dir = os.path.join(
                    debug_install_host_dir, debug_package_name)
                if os.path.exists(debug_install_host_dir):
                    rmtree(debug_install_host_dir)
                install_toolchain(
                    debug_clang_out_dir, debug_install_dir, host, False)

    dist_dir = ORIG_ENV.get('DIST_DIR', final_out_dir)
    for host in hosts:
        package_toolchain(final_out_dir, args.build_name, host, dist_dir)


if __name__ == '__main__':
    main()
