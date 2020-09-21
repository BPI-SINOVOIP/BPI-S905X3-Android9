# -*- sh -*- (Bash only)
#
# Copyright 2015 The Bazel Authors. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#
# Bash completion of Bazel commands.
#
# The template is expanded at build time using tables of commands/options
# derived from the bazel executable built in the same client; the expansion is
# written to bazel-complete.bash.
#
# Provides command-completion for:
# - bazel prefix options (e.g. --host_jvm_args)
# - blaze command-set (e.g. build, test)
# - blaze command-specific options (e.g. --copts)
# - values for enum-valued options
# - package-names, exploring all package-path roots.
# - targets within packages.

# The package path used by the completion routines.  Unfortunately
# this isn't necessarily the same as the actual package path used by
# Bazel, but that's ok.  (It's impossible for us to reliably know what
# the relevant package-path, so this is just a good guess.  Users can
# override it if they want.)
#
# Don't use it directly. Generate the final script with
# bazel build //scripts:bash_completion instead.
#
: ${BAZEL_COMPLETION_PACKAGE_PATH:=%workspace%}


# If true, Bazel query is used for autocompletion.  This is more
# accurate than the heuristic grep, especially for strangely-formatted
# BUILD files.  But it can be slower, especially if the Bazel server
# is busy, and more brittle, if the BUILD file contains serious
# errors.   This is an experimental feature.
: ${BAZEL_COMPLETION_USE_QUERY:=false}


# If true, Bazel run allows autocompletion for test targets. This is convenient
# for users who run a lot of tests/benchmarks locally with blaze run.
: ${BAZEL_COMPLETION_ALLOW_TESTS_FOR_RUN:=false}

# Some commands might interfer with the important one, so don't complete them
: ${BAZEL_IGNORED_COMMAND_REGEX:="__none__"}

# Bazel command
: ${BAZEL:=bazel}

# Pattern to match for looking for a target
#  BAZEL_BUILD_MATCH_PATTERN__* give the pattern for label-*
#  when looking in the the build file.
#  BAZEL_QUERY_MATCH_PATTERN__* give the pattern for label-*
#  when using 'bazel query'.
# _RUNTEST are special case when BAZEL_COMPLETION_ALLOW_TESTS_FOR_RUN
# is on.
: ${BAZEL_BUILD_MATCH_PATTERN__test:='(.*_test|test_suite)'}
: ${BAZEL_QUERY_MATCH_PATTERN__test:='(test|test_suite)'}
: ${BAZEL_BUILD_MATCH_PATTERN__bin:='.*_binary'}
: ${BAZEL_QUERY_MATCH_PATTERN__bin:='(binary)'}
: ${BAZEL_BUILD_MATCH_PATTERN_RUNTEST__bin:='(.*_(binary|test)|test_suite)'}
: ${BAZEL_QUERY_MATCH_PATTERN_RUNTEST__bin:='(binary|test)'}
: ${BAZEL_BUILD_MATCH_PATTERN__:='.*'}
: ${BAZEL_QUERY_MATCH_PATTERN__:=''}

# Usage: _bazel__get_rule_match_pattern <command>
# Determine what kind of rules to match, based on command.
_bazel__get_rule_match_pattern() {
  local var_name pattern
  if _bazel__is_true "$BAZEL_COMPLETION_USE_QUERY"; then
    var_name="BAZEL_QUERY_MATCH_PATTERN"
  else
    var_name="BAZEL_BUILD_MATCH_PATTERN"
  fi
  if [[ "$1" =~ ^label-?([a-z]*)$ ]]; then
    pattern=${BASH_REMATCH[1]:-}
    if _bazel__is_true "$BAZEL_COMPLETION_ALLOW_TESTS_FOR_RUN"; then
      eval "echo \"\${${var_name}_RUNTEST__${pattern}:-\$${var_name}__${pattern}}\""
    else
      eval "echo \"\$${var_name}__${pattern}\""
    fi
  fi
}

# Compute workspace directory. Search for the innermost
# enclosing directory with a WORKSPACE file.
_bazel__get_workspace_path() {
  local workspace=$PWD
  while true; do
    if [ -f "${workspace}/WORKSPACE" ]; then
      break
    elif [ -z "$workspace" -o "$workspace" = "/" ]; then
      workspace=$PWD
      break;
    fi
    workspace=${workspace%/*}
  done
  echo $workspace
}


# Find the current piece of the line to complete, but only do word breaks at
# certain characters. In particular, ignore these: "':=
# This method also takes into account the current cursor position.
#
# Works with both bash 3 and 4! Bash 3 and 4 perform different word breaks when
# computing the COMP_WORDS array. We need this here because Bazel options are of
# the form --a=b, and labels of the form //some/label:target.
_bazel__get_cword() {
  local cur=${COMP_LINE:0:$COMP_POINT}
  # This expression finds the last word break character, as defined in the
  # COMP_WORDBREAKS variable, but without '=' or ':', which is not preceeded by
  # a slash. Quote characters are also excluded.
  local wordbreaks="$COMP_WORDBREAKS"
  wordbreaks="${wordbreaks//\'/}"
  wordbreaks="${wordbreaks//\"/}"
  wordbreaks="${wordbreaks//:/}"
  wordbreaks="${wordbreaks//=/}"
  local word_start=$(expr "$cur" : '.*[^\]['"${wordbreaks}"']')
  echo "${cur:$word_start}"
}


# Usage: _bazel__package_path <workspace> <displacement>
#
# Prints a list of package-path root directories, displaced using the
# current displacement from the workspace.  All elements have a
# trailing slash.
_bazel__package_path() {
  local workspace=$1 displacement=$2 root
  IFS=:
  for root in ${BAZEL_COMPLETION_PACKAGE_PATH//\%workspace\%/$workspace}; do
    unset IFS
    echo "$root/$displacement"
  done
}

# Usage: _bazel__options_for <command>
#
# Prints the set of options for a given Bazel command, e.g. "build".
_bazel__options_for() {
  local options
  if [[ "${BAZEL_COMMAND_LIST}" =~ ^(.* )?$1( .*)?$ ]]; then
      local option_name=$(echo $1 | perl -ne 'print uc' | tr "-" "_")
      eval "echo \${BAZEL_COMMAND_${option_name}_FLAGS}" | tr " " "\n"
  fi
}
# Usage: _bazel__expansion_for <command>
#
# Prints the completion pattern for a given Bazel command, e.g. "build".
_bazel__expansion_for() {
  local options
  if [[ "${BAZEL_COMMAND_LIST}" =~ ^(.* )?$1( .*)?$ ]]; then
      local option_name=$(echo $1 | perl -ne 'print uc' | tr "-" "_")
      eval "echo \${BAZEL_COMMAND_${option_name}_ARGUMENT}"
  fi
}

# Usage: _bazel__matching_targets <kind> <prefix>
#
# Prints target names of kind <kind> and starting with <prefix> in the BUILD
# file given as standard input.  <kind> is a basic regex (BRE) used to match the
# bazel rule kind and <prefix> is the prefix of the target name.
_bazel__matching_targets() {
  local kind_pattern="$1"
  local target_prefix="$2"
  # The following commands do respectively:
  #   Remove BUILD file comments
  #   Replace \n by spaces to have the BUILD file in a single line
  #   Extract all rule types and target names
  #   Grep the kind pattern and the target prefix
  #   Returns the target name
  sed 's/#.*$//' \
      | tr "\n" " " \
      | sed 's/\([a-zA-Z0-9_]*\) *(\([^)]* \)\{0,1\}name *= *['\''"]\([a-zA-Z0-9_/.+=,@~-]*\)['\''"][^)]*)/\
type:\1 name:\3\
/g' \
      | "grep" -E "^type:$kind_pattern name:$target_prefix" \
      | cut -d ':' -f 3
}


# Usage: _bazel__is_true <string>
#
# Returns true or false based on the input string. The following are
# valid true values (the rest are false): "1", "true".
_bazel__is_true() {
  local str="$1"
  [[ "$str" == "1" || "$str" == "true" ]]
}

# Usage: _bazel__expand_rules_in_package <workspace> <displacement>
#                                        <current> <label-type>
#
# Expands rules in specified packages, exploring all roots of
# $BAZEL_COMPLETION_PACKAGE_PATH, not just $(pwd).  Only rules
# appropriate to the command are printed.  Sets $COMPREPLY array to
# result.
#
# If $BAZEL_COMPLETION_USE_QUERY is true, 'bazel query' is used
# instead, with the actual Bazel package path;
# $BAZEL_COMPLETION_PACKAGE_PATH is ignored in this case, since the
# actual Bazel value is likely to be more accurate.
_bazel__expand_rules_in_package() {
  local workspace=$1 displacement=$2 current=$3 label_type=$4
  local package_name=$(echo "$current" | cut -f1 -d:)
  local rule_prefix=$(echo "$current" | cut -f2 -d:)
  local root buildfile rule_pattern r result

  result=
  pattern=$(_bazel__get_rule_match_pattern "$label_type")
  if _bazel__is_true "$BAZEL_COMPLETION_USE_QUERY"; then
    package_name=$(echo "$package_name" | tr -d "'\"") # remove quotes
    result=$(${BAZEL} --output_base=/tmp/${BAZEL}-completion-$USER query \
                   --keep_going --noshow_progress \
      "kind('$pattern rule', '$package_name:*')" 2>/dev/null |
      cut -f2 -d: | "grep" "^$rule_prefix")
  else
    for root in $(_bazel__package_path "$workspace" "$displacement"); do
      buildfile="$root/$package_name/BUILD"
      if [ -f "$buildfile" ]; then
        result=$(_bazel__matching_targets \
                   "$pattern" "$rule_prefix" <"$buildfile")
        break
      fi
    done
  fi

  index=$(echo $result | wc -w)
  if [ -n "$result" ]; then
      echo "$result" | tr " " "\n" | sed 's|$| |'
  fi
  # Include ":all" wildcard if there was no unique match.  (The zero
  # case is tricky: we need to include "all" in that case since
  # otherwise we won't expand "a" to "all" in the absence of rules
  # starting with "a".)
  if [ $index -ne 1 ] && expr all : "\\($rule_prefix\\)" >/dev/null; then
    echo "all "
  fi
}

# Usage: _bazel__expand_package_name <workspace> <displacement> <current-word>
#                                    <label-type>
#
# Expands directories, but explores all roots of
# BAZEL_COMPLETION_PACKAGE_PATH, not just $(pwd).  When a directory is
# a bazel package, the completion offers "pkg:" so you can expand
# inside the package.
# Sets $COMPREPLY array to result.
_bazel__expand_package_name() {
  local workspace=$1 displacement=$2 current=$3 type=${4:-} root dir index
  for root in $(_bazel__package_path "$workspace" "$displacement"); do
    found=0
    for dir in $(compgen -d $root$current); do
      [ -L "$dir" ] && continue  # skip symlinks (e.g. bazel-bin)
      [[ "$dir" =~ ^(.*/)?\.[^/]*$ ]] && continue  # skip dotted dir (e.g. .git)
      found=1
      echo "${dir#$root}/"
      if [ -f $dir/BUILD ]; then
        if [ "${type}" = "label-package" ]; then
          echo "${dir#$root} "
        else
          echo "${dir#$root}:"
        fi
      fi
    done
    [ $found -gt 0 ] && break  # Stop searching package path upon first match.
  done
}

# Usage: _bazel__expand_target_pattern <workspace> <displacement>
#                                      <word> <label-syntax>
#
# Expands "word" to match target patterns, using the current workspace
# and displacement from it.  "command" is used to filter rules.
# Sets $COMPREPLY array to result.
_bazel__expand_target_pattern() {
  local workspace=$1 displacement=$2 current=$3 label_syntax=$4
  case "$current" in
    //*:*) # Expand rule names within package, no displacement.
      if [ "${label_syntax}" = "label-package" ]; then
        compgen -S " " -W "BUILD" "$(echo current | cut -f ':' -d2)"
      else
        _bazel__expand_rules_in_package "$workspace" "" "$current" "$label_syntax"
      fi
      ;;
    *:*) # Expand rule names within package, displaced.
      if [ "${label_syntax}" = "label-package" ]; then
        compgen -S " " -W "BUILD" "$(echo current | cut -f ':' -d2)"
      else
        _bazel__expand_rules_in_package \
          "$workspace" "$displacement" "$current" "$label_syntax"
      fi
      ;;
    //*) # Expand filenames using package-path, no displacement
      _bazel__expand_package_name "$workspace" "" "$current" "$label_syntax"
      ;;
    *) # Expand filenames using package-path, displaced.
      if [ -n "$current" ]; then
        _bazel__expand_package_name "$workspace" "$displacement" "$current" "$label_syntax"
      fi
      ;;
  esac
}

_bazel__get_command() {
  for word in "${COMP_WORDS[@]:1:COMP_CWORD-1}"; do
    if echo "$BAZEL_COMMAND_LIST" | "grep" -wsq -e "$word"; then
      echo $word
      break
    fi
  done
}

# Returns the displacement to the workspace given in $1
_bazel__get_displacement() {
  if [[ "$PWD" =~ ^$1/.*$ ]]; then
    echo ${PWD##$1/}/
  fi
}


# Usage: _bazel__complete_pattern <workspace> <displacement> <current>
#                                 <type>
#
# Expand a word according to a type. The currently supported types are:
#  - {a,b,c}: an enum that can take value a, b or c
#  - label: a label of any kind
#  - label-bin: a label to a runnable rule (basically to a _binary rule)
#  - label-test: a label to a test rule
#  - info-key: an info key as listed by `bazel help info-keys`
#  - command: the name of a command
#  - path: a file path
#  - combinaison of previous type using | as separator
_bazel__complete_pattern() {
  local workspace=$1 displacement=$2 current=$3 types=$4
  for type in $(echo $types | tr "|" "\n"); do
    case "$type" in
      label*)
        _bazel__expand_target_pattern "$workspace" "$displacement" \
            "$current" "$type"
        ;;
      info-key)
    compgen -S " " -W "${BAZEL_INFO_KEYS}" -- "$current"
        ;;
      "command")
        local commands=$(echo "${BAZEL_COMMAND_LIST}" \
          | tr " " "\n" | "grep" -v "^${BAZEL_IGNORED_COMMAND_REGEX}$")
    compgen -S " " -W "${commands}" -- "$current"
        ;;
      path)
        compgen -f -- "$current"
        ;;
      *)
        compgen -S " " -W "$type" -- "$current"
        ;;
    esac
  done
}

# Usage: _bazel__expand_options <workspace> <displacement> <current-word>
#                               <options>
#
# Expands options, making sure that if current-word contains an equals sign,
# it is handled appropriately.
_bazel__expand_options() {
  local workspace="$1" displacement="$2" cur="$3" options="$4"
  if [[ $cur =~ = ]]; then
    # also expands special labels
    current=$(echo "$cur" | cut -f2 -d=)
    _bazel__complete_pattern "$workspace" "$displacement" "$current" \
    "$(compgen -W "$options" -- "$cur" | cut -f2 -d=)" \
        | sort -u
  else
    compgen -W "$(echo "$options" | sed 's|=.*$|=|')" -- "$cur" \
    | sed 's|\([^=]\)$|\1 |'
  fi
}


_bazel__complete_stdout() {
  local cur=$(_bazel__get_cword) word command displacement workspace

  # Determine command: "" (startup-options) or one of $BAZEL_COMMAND_LIST.
  command="$(_bazel__get_command)"

  workspace="$(_bazel__get_workspace_path)"
  displacement="$(_bazel__get_displacement ${workspace})"

  case "$command" in
    "") # Expand startup-options or commands
      local commands=$(echo "${BAZEL_COMMAND_LIST}" \
        | tr " " "\n" | "grep" -v "^${BAZEL_IGNORED_COMMAND_REGEX}$")
      _bazel__expand_options  "$workspace" "$displacement" "$cur" \
          "${commands}\
          ${BAZEL_STARTUP_OPTIONS}"
      ;;

    *)
      case "$cur" in
        -*) # Expand options:
          _bazel__expand_options  "$workspace" "$displacement" "$cur" \
              "$(_bazel__options_for $command)"
          ;;
        *)  # Expand target pattern
      expansion_pattern="$(_bazel__expansion_for $command)"
          NON_QUOTE_REGEX="^[\"']"
          if [[ $command = query && $cur =~ $NON_QUOTE_REGEX ]]; then
            : # Ideally we would expand query expressions---it's not
              # that hard, conceptually---but readline is just too
              # damn complex when it comes to quotation.  Instead,
              # for query, we just expand target patterns, unless
              # the first char is a quote.
          elif [ -n "$expansion_pattern" ]; then
            _bazel__complete_pattern \
        "$workspace" "$displacement" "$cur" "$expansion_pattern"
          fi
          ;;
      esac
      ;;
  esac
}

_bazel__to_compreply() {
  local replies="$1"
  COMPREPLY=()
  # Trick to preserve whitespaces
  while IFS="" read -r reply; do
    COMPREPLY+=("${reply}")
  done < <(echo "${replies}")
}

_bazel__complete() {
  _bazel__to_compreply "$(_bazel__complete_stdout)"
}

# Some users have aliases such as bt="bazel test" or bb="bazel build", this
# completion function allows them to have auto-completion for these aliases.
_bazel__complete_target_stdout() {
  local cur=$(_bazel__get_cword) word command displacement workspace

  # Determine command: "" (startup-options) or one of $BAZEL_COMMAND_LIST.
  command="$1"

  workspace="$(_bazel__get_workspace_path)"
  displacement="$(_bazel__get_displacement ${workspace})"

  _bazel__to_compreply "$(_bazel__expand_target_pattern "$workspace" "$displacement" \
      "$cur" "$(_bazel__expansion_for $command)")"
}

# default completion for bazel
complete -F _bazel__complete -o nospace "${BAZEL}"
BAZEL_COMMAND_LIST="analyze-profile build canonicalize-flags clean coverage dump fetch help info mobile-install query run shutdown test version"
BAZEL_INFO_KEYS="
workspace
install_base
output_base
execution_root
output_path
client-env
bazel-bin
bazel-genfiles
bazel-testlogs
command_log
message_log
release
server_pid
package_path
used-heap-size
used-heap-size-after-gc
committed-heap-size
max-heap-size
gc-time
gc-count
defaults-package
build-language
default-package-path
"
BAZEL_STARTUP_OPTIONS="
--batch
--nobatch
--batch_cpu_scheduling
--nobatch_cpu_scheduling
--blazerc=
--block_for_lock
--noblock_for_lock
--client_debug
--noclient_debug
--connect_timeout_secs=
--deep_execroot
--nodeep_execroot
--exoblaze
--noexoblaze
--experimental_oom_more_eagerly
--noexperimental_oom_more_eagerly
--experimental_oom_more_eagerly_threshold=
--host_jvm_args=
--host_jvm_debug
--host_jvm_profile=
--io_nice_level=
--master_blazerc
--nomaster_blazerc
--max_idle_secs=
--output_base=path
--output_user_root=path
--use_action_cache
--nouse_action_cache
--watchfs
--nowatchfs
"
BAZEL_COMMAND_ANALYZE_PROFILE_ARGUMENT="path"
BAZEL_COMMAND_ANALYZE_PROFILE_FLAGS="
--announce_rc
--noannounce_rc
--chart
--nochart
--color={yes,no,auto}
--combine=
--config=
--curses={yes,no,auto}
--dump=
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--html
--nohtml
--html_details
--nohtml_details
--html_histograms
--nohtml_histograms
--html_pixels_per_second=
--logging=
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--task_tree=
--task_tree_threshold=
--tool_tag=
--vfs_stats
--novfs_stats
--vfs_stats_limit=
--watchfs
--nowatchfs
"
BAZEL_COMMAND_BUILD_ARGUMENT="label"
BAZEL_COMMAND_BUILD_FLAGS="
--action_env=
--analysis_warnings_as_errors
--noanalysis_warnings_as_errors
--android_compiler=
--android_cpu=
--android_crosstool_top=label
--android_manifest_merger={legacy,android}
--android_resource_shrinking
--noandroid_resource_shrinking
--android_sdk=label
--announce
--noannounce
--announce_rc
--noannounce_rc
--apple_bitcode={none,embedded_markers,embedded}
--apple_crosstool_top=label
--apple_generate_dsym
--noapple_generate_dsym
--autofdo_lipo_data
--noautofdo_lipo_data
--build
--nobuild
--build_runfile_links
--nobuild_runfile_links
--build_tag_filters=
--build_test_dwp
--nobuild_test_dwp
--build_tests_only
--nobuild_tests_only
--cache_test_results={auto,yes,no}
--nocache_test_results
--check_constraint=
--check_fileset_dependencies_recursively
--nocheck_fileset_dependencies_recursively
--check_licenses
--nocheck_licenses
--check_tests_up_to_date
--nocheck_tests_up_to_date
--check_up_to_date
--nocheck_up_to_date
--check_visibility
--nocheck_visibility
--collect_code_coverage
--nocollect_code_coverage
--color={yes,no,auto}
--compilation_mode={fastbuild,dbg,opt}
--compile_one_dependency
--nocompile_one_dependency
--compiler=
--config=
--conlyopt=
--copt=
--coverage_report_generator=label
--coverage_support=label
--cpu=
--crosstool_top=label
--curses={yes,no,auto}
--custom_malloc=label
--cxxopt=
--dash_secret=
--dash_url=
--define=
--deleted_packages=
--deprecated_generate_xcode_project
--nodeprecated_generate_xcode_project
--device_debug_entitlements
--nodevice_debug_entitlements
--discard_analysis_cache
--nodiscard_analysis_cache
--distinct_host_configuration
--nodistinct_host_configuration
--dynamic_mode={off,default,fully}
--embed_label=
--enable_apple_binary_native_protos
--noenable_apple_binary_native_protos
--experimental_action_listener=
--experimental_android_jack_sanity_checks
--noexperimental_android_jack_sanity_checks
--experimental_android_use_jack_for_dexing
--noexperimental_android_use_jack_for_dexing
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_extra_action_filter=
--experimental_extra_action_top_level_only
--noexperimental_extra_action_top_level_only
--experimental_extra_action_top_level_only_with_aspects
--noexperimental_extra_action_top_level_only_with_aspects
--experimental_incremental_dexing_for_lite_protos
--noexperimental_incremental_dexing_for_lite_protos
--experimental_inmemory_dotd_files
--noexperimental_inmemory_dotd_files
--experimental_interleave_loading_and_analysis
--noexperimental_interleave_loading_and_analysis
--experimental_j2objc_srcjar_processing
--noexperimental_j2objc_srcjar_processing
--experimental_link_compile_output_separately
--noexperimental_link_compile_output_separately
--experimental_multi_cpu=
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_objc_header_thinning
--noexperimental_objc_header_thinning
--experimental_omitfp
--noexperimental_omitfp
--experimental_persistent_javac
--experimental_proto_extra_actions
--noexperimental_proto_extra_actions
--experimental_prune_more_modules
--noexperimental_prune_more_modules
--experimental_skip_static_outputs
--noexperimental_skip_static_outputs
--experimental_skip_unused_modules
--noexperimental_skip_unused_modules
--experimental_skyframe_native_filesets
--noexperimental_skyframe_native_filesets
--experimental_stl=label
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--experimental_use_llvm_covmap
--noexperimental_use_llvm_covmap
--explain=path
--explicit_jre_deps
--noexplicit_jre_deps
--extra_entitlements=label
--fat_apk_cpu=
--fdo_instrument=path
--fdo_optimize=
--features=
--fission=
--flaky_test_attempts=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--force_ignore_dash_static
--noforce_ignore_dash_static
--force_pic
--noforce_pic
--force_python={py2,py3,py2and3,py2only,py3only}
--genrule_strategy=
--glibc=
--grpc_max_batch_inputs=
--grpc_max_batch_size_bytes=
--grpc_max_chunk_size_bytes=
--grpc_timeout_seconds=
--grte_top=
--hazelcast_client_config=
--hazelcast_node=
--hazelcast_standalone_listen_port=
--host_copt=
--host_cpu=
--host_crosstool_top=label
--host_force_python={py2,py3,py2and3,py2only,py3only}
--host_grte_top=
--host_java_launcher=label
--host_java_toolchain=label
--host_javabase=
--ignore_unsupported_sandboxing
--noignore_unsupported_sandboxing
--incremental_dexing
--noincremental_dexing
--instrument_test_targets
--noinstrument_test_targets
--instrumentation_filter=
--interface_shared_objects
--nointerface_shared_objects
--ios_cpu=
--ios_memleaks
--noios_memleaks
--ios_minimum_os=
--ios_multi_cpus=
--ios_sdk_version=
--ios_signing_cert_name=
--ios_simulator_device=
--ios_simulator_version=
--j2objc_translation_flags=
--java_classpath={off,javabuilder,experimental_blaze}
--java_debug
--java_deps
--nojava_deps
--java_header_compilation
--nojava_header_compilation
--java_launcher=label
--java_toolchain=label
--javabase=
--javacopt=
--jobs=
--jvmopt=
--keep_going
--nokeep_going
--legacy_external_runfiles
--nolegacy_external_runfiles
--legacy_whole_archive
--nolegacy_whole_archive
--linkopt=
--lipo={off,binary}
--lipo_context=label
--loading_phase_threads=
--local_resources=
--local_test_jobs=
--logging=
--ltoindexopt=
--macos_cpus=
--macos_minimum_os=
--macos_sdk_version=
--message_translations=
--microcoverage
--nomicrocoverage
--non_incremental_per_target_dexopts=
--objc_enable_binary_stripping
--noobjc_enable_binary_stripping
--objc_generate_linkmap
--noobjc_generate_linkmap
--objc_includes_prioritize_static_libs
--noobjc_includes_prioritize_static_libs
--objc_use_dotd_pruning
--noobjc_use_dotd_pruning
--objccopt=
--output_descriptor_set
--nooutput_descriptor_set
--output_filter=
--output_symbol_counts
--nooutput_symbol_counts
--package_path=
--parse_headers_verifies_modules
--noparse_headers_verifies_modules
--per_file_copt=
--platform_suffix=
--plugin=
--plugin_copt=
--process_headers_in_dependencies
--noprocess_headers_in_dependencies
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--progress_report_interval=
--proguard_top=label
--proto_compiler=label
--proto_toolchain_for_cc=label
--proto_toolchain_for_java=label
--proto_toolchain_for_javalite=label
--protocopt=
--prune_cpp_modules
--noprune_cpp_modules
--python2_path=
--python3_path=
--ram_utilization_factor=
--remote_accept_cached
--noremote_accept_cached
--remote_allow_local_fallback
--noremote_allow_local_fallback
--remote_cache=
--remote_worker=
--resource_autosense
--noresource_autosense
--rest_cache_url=
--run_under=
--runs_per_test=
--runs_per_test_detects_flakes
--noruns_per_test_detects_flakes
--sandbox_add_mount_pair=
--sandbox_block_path=
--sandbox_debug
--nosandbox_debug
--sandbox_fake_hostname
--nosandbox_fake_hostname
--sandbox_tmpfs_path=
--save_temps
--nosave_temps
--send_transitive_header_module_srcs
--nosend_transitive_header_module_srcs
--share_native_deps
--noshare_native_deps
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_result=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--spawn_strategy=
--stamp
--nostamp
--start_end_lib
--nostart_end_lib
--strategy=
--strict_filesets
--nostrict_filesets
--strict_java_deps={off,warn,error,strict,default}
--strict_proto_deps={off,warn,error,strict,default}
--strict_system_includes
--nostrict_system_includes
--strip={always,sometimes,never}
--stripopt=
--subcommands
--nosubcommands
--swift_whole_module_optimization
--noswift_whole_module_optimization
--swiftcopt=
--symlink_prefix=
--target_environment=
--test_arg=
--test_env=
--test_filter=
--test_keep_going
--notest_keep_going
--test_lang_filters=
--test_output={summary,errors,all,streamed}
--test_result_expiration=
--test_sharding_strategy={explicit,experimental_heuristic,disabled}
--test_size_filters=
--test_strategy=
--test_summary={short,terse,detailed,none}
--test_tag_filters=
--test_timeout=
--test_timeout_filters=
--test_tmpdir=path
--tool_tag=
--translations={auto,yes,no}
--notranslations
--tvos_cpus=
--tvos_minimum_os=
--tvos_sdk_version=
--tvos_simulator_device=
--tvos_simulator_version=
--use_dash
--nouse_dash
--use_ijars
--nouse_ijars
--verbose_explanations
--noverbose_explanations
--verbose_failures
--noverbose_failures
--watchfs
--nowatchfs
--watchos_cpus=
--watchos_minimum_os=
--watchos_sdk_version=
--watchos_simulator_device=
--watchos_simulator_version=
--worker_extra_flag=
--worker_max_instances=
--worker_max_retries=
--worker_quit_after_build
--noworker_quit_after_build
--worker_sandboxing
--noworker_sandboxing
--worker_verbose
--noworker_verbose
--workspace_status_command=path
--xcode_override_workspace_root=
--xcode_toolchain=
--xcode_version=
"
BAZEL_COMMAND_CANONICALIZE_FLAGS_FLAGS="
--announce_rc
--noannounce_rc
--color={yes,no,auto}
--config=
--curses={yes,no,auto}
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--for_command=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--invocation_policy=
--logging=
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--tool_tag=
--watchfs
--nowatchfs
"
BAZEL_COMMAND_CLEAN_FLAGS="
--action_env=
--analysis_warnings_as_errors
--noanalysis_warnings_as_errors
--android_compiler=
--android_cpu=
--android_crosstool_top=label
--android_manifest_merger={legacy,android}
--android_resource_shrinking
--noandroid_resource_shrinking
--android_sdk=label
--announce
--noannounce
--announce_rc
--noannounce_rc
--apple_bitcode={none,embedded_markers,embedded}
--apple_crosstool_top=label
--apple_generate_dsym
--noapple_generate_dsym
--async
--noasync
--autofdo_lipo_data
--noautofdo_lipo_data
--build
--nobuild
--build_runfile_links
--nobuild_runfile_links
--build_tag_filters=
--build_test_dwp
--nobuild_test_dwp
--build_tests_only
--nobuild_tests_only
--cache_test_results={auto,yes,no}
--nocache_test_results
--check_constraint=
--check_fileset_dependencies_recursively
--nocheck_fileset_dependencies_recursively
--check_licenses
--nocheck_licenses
--check_tests_up_to_date
--nocheck_tests_up_to_date
--check_up_to_date
--nocheck_up_to_date
--check_visibility
--nocheck_visibility
--clean_style=
--collect_code_coverage
--nocollect_code_coverage
--color={yes,no,auto}
--compilation_mode={fastbuild,dbg,opt}
--compile_one_dependency
--nocompile_one_dependency
--compiler=
--config=
--conlyopt=
--copt=
--coverage_report_generator=label
--coverage_support=label
--cpu=
--crosstool_top=label
--curses={yes,no,auto}
--custom_malloc=label
--cxxopt=
--dash_secret=
--dash_url=
--define=
--deleted_packages=
--deprecated_generate_xcode_project
--nodeprecated_generate_xcode_project
--device_debug_entitlements
--nodevice_debug_entitlements
--discard_analysis_cache
--nodiscard_analysis_cache
--distinct_host_configuration
--nodistinct_host_configuration
--dynamic_mode={off,default,fully}
--embed_label=
--enable_apple_binary_native_protos
--noenable_apple_binary_native_protos
--experimental_action_listener=
--experimental_android_jack_sanity_checks
--noexperimental_android_jack_sanity_checks
--experimental_android_use_jack_for_dexing
--noexperimental_android_use_jack_for_dexing
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_extra_action_filter=
--experimental_extra_action_top_level_only
--noexperimental_extra_action_top_level_only
--experimental_extra_action_top_level_only_with_aspects
--noexperimental_extra_action_top_level_only_with_aspects
--experimental_incremental_dexing_for_lite_protos
--noexperimental_incremental_dexing_for_lite_protos
--experimental_inmemory_dotd_files
--noexperimental_inmemory_dotd_files
--experimental_interleave_loading_and_analysis
--noexperimental_interleave_loading_and_analysis
--experimental_j2objc_srcjar_processing
--noexperimental_j2objc_srcjar_processing
--experimental_link_compile_output_separately
--noexperimental_link_compile_output_separately
--experimental_multi_cpu=
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_objc_header_thinning
--noexperimental_objc_header_thinning
--experimental_omitfp
--noexperimental_omitfp
--experimental_persistent_javac
--experimental_proto_extra_actions
--noexperimental_proto_extra_actions
--experimental_prune_more_modules
--noexperimental_prune_more_modules
--experimental_skip_static_outputs
--noexperimental_skip_static_outputs
--experimental_skip_unused_modules
--noexperimental_skip_unused_modules
--experimental_skyframe_native_filesets
--noexperimental_skyframe_native_filesets
--experimental_stl=label
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--experimental_use_llvm_covmap
--noexperimental_use_llvm_covmap
--explain=path
--explicit_jre_deps
--noexplicit_jre_deps
--expunge
--noexpunge
--expunge_async
--noexpunge_async
--extra_entitlements=label
--fat_apk_cpu=
--fdo_instrument=path
--fdo_optimize=
--features=
--fission=
--flaky_test_attempts=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--force_ignore_dash_static
--noforce_ignore_dash_static
--force_pic
--noforce_pic
--force_python={py2,py3,py2and3,py2only,py3only}
--genrule_strategy=
--glibc=
--grpc_max_batch_inputs=
--grpc_max_batch_size_bytes=
--grpc_max_chunk_size_bytes=
--grpc_timeout_seconds=
--grte_top=
--hazelcast_client_config=
--hazelcast_node=
--hazelcast_standalone_listen_port=
--host_copt=
--host_cpu=
--host_crosstool_top=label
--host_force_python={py2,py3,py2and3,py2only,py3only}
--host_grte_top=
--host_java_launcher=label
--host_java_toolchain=label
--host_javabase=
--ignore_unsupported_sandboxing
--noignore_unsupported_sandboxing
--incremental_dexing
--noincremental_dexing
--instrument_test_targets
--noinstrument_test_targets
--instrumentation_filter=
--interface_shared_objects
--nointerface_shared_objects
--ios_cpu=
--ios_memleaks
--noios_memleaks
--ios_minimum_os=
--ios_multi_cpus=
--ios_sdk_version=
--ios_signing_cert_name=
--ios_simulator_device=
--ios_simulator_version=
--j2objc_translation_flags=
--java_classpath={off,javabuilder,experimental_blaze}
--java_debug
--java_deps
--nojava_deps
--java_header_compilation
--nojava_header_compilation
--java_launcher=label
--java_toolchain=label
--javabase=
--javacopt=
--jobs=
--jvmopt=
--keep_going
--nokeep_going
--legacy_external_runfiles
--nolegacy_external_runfiles
--legacy_whole_archive
--nolegacy_whole_archive
--linkopt=
--lipo={off,binary}
--lipo_context=label
--loading_phase_threads=
--local_resources=
--local_test_jobs=
--logging=
--ltoindexopt=
--macos_cpus=
--macos_minimum_os=
--macos_sdk_version=
--message_translations=
--microcoverage
--nomicrocoverage
--non_incremental_per_target_dexopts=
--objc_enable_binary_stripping
--noobjc_enable_binary_stripping
--objc_generate_linkmap
--noobjc_generate_linkmap
--objc_includes_prioritize_static_libs
--noobjc_includes_prioritize_static_libs
--objc_use_dotd_pruning
--noobjc_use_dotd_pruning
--objccopt=
--output_descriptor_set
--nooutput_descriptor_set
--output_filter=
--output_symbol_counts
--nooutput_symbol_counts
--package_path=
--parse_headers_verifies_modules
--noparse_headers_verifies_modules
--per_file_copt=
--platform_suffix=
--plugin=
--plugin_copt=
--process_headers_in_dependencies
--noprocess_headers_in_dependencies
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--progress_report_interval=
--proguard_top=label
--proto_compiler=label
--proto_toolchain_for_cc=label
--proto_toolchain_for_java=label
--proto_toolchain_for_javalite=label
--protocopt=
--prune_cpp_modules
--noprune_cpp_modules
--python2_path=
--python3_path=
--ram_utilization_factor=
--remote_accept_cached
--noremote_accept_cached
--remote_allow_local_fallback
--noremote_allow_local_fallback
--remote_cache=
--remote_worker=
--resource_autosense
--noresource_autosense
--rest_cache_url=
--run_under=
--runs_per_test=
--runs_per_test_detects_flakes
--noruns_per_test_detects_flakes
--sandbox_add_mount_pair=
--sandbox_block_path=
--sandbox_debug
--nosandbox_debug
--sandbox_fake_hostname
--nosandbox_fake_hostname
--sandbox_tmpfs_path=
--save_temps
--nosave_temps
--send_transitive_header_module_srcs
--nosend_transitive_header_module_srcs
--share_native_deps
--noshare_native_deps
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_result=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--spawn_strategy=
--stamp
--nostamp
--start_end_lib
--nostart_end_lib
--strategy=
--strict_filesets
--nostrict_filesets
--strict_java_deps={off,warn,error,strict,default}
--strict_proto_deps={off,warn,error,strict,default}
--strict_system_includes
--nostrict_system_includes
--strip={always,sometimes,never}
--stripopt=
--subcommands
--nosubcommands
--swift_whole_module_optimization
--noswift_whole_module_optimization
--swiftcopt=
--symlink_prefix=
--target_environment=
--test_arg=
--test_env=
--test_filter=
--test_keep_going
--notest_keep_going
--test_lang_filters=
--test_output={summary,errors,all,streamed}
--test_result_expiration=
--test_sharding_strategy={explicit,experimental_heuristic,disabled}
--test_size_filters=
--test_strategy=
--test_summary={short,terse,detailed,none}
--test_tag_filters=
--test_timeout=
--test_timeout_filters=
--test_tmpdir=path
--tool_tag=
--translations={auto,yes,no}
--notranslations
--tvos_cpus=
--tvos_minimum_os=
--tvos_sdk_version=
--tvos_simulator_device=
--tvos_simulator_version=
--use_dash
--nouse_dash
--use_ijars
--nouse_ijars
--verbose_explanations
--noverbose_explanations
--verbose_failures
--noverbose_failures
--watchfs
--nowatchfs
--watchos_cpus=
--watchos_minimum_os=
--watchos_sdk_version=
--watchos_simulator_device=
--watchos_simulator_version=
--worker_extra_flag=
--worker_max_instances=
--worker_max_retries=
--worker_quit_after_build
--noworker_quit_after_build
--worker_sandboxing
--noworker_sandboxing
--worker_verbose
--noworker_verbose
--workspace_status_command=path
--xcode_override_workspace_root=
--xcode_toolchain=
--xcode_version=
"
BAZEL_COMMAND_COVERAGE_ARGUMENT="label-test"
BAZEL_COMMAND_COVERAGE_FLAGS="
--action_env=
--analysis_warnings_as_errors
--noanalysis_warnings_as_errors
--android_compiler=
--android_cpu=
--android_crosstool_top=label
--android_manifest_merger={legacy,android}
--android_resource_shrinking
--noandroid_resource_shrinking
--android_sdk=label
--announce
--noannounce
--announce_rc
--noannounce_rc
--apple_bitcode={none,embedded_markers,embedded}
--apple_crosstool_top=label
--apple_generate_dsym
--noapple_generate_dsym
--autofdo_lipo_data
--noautofdo_lipo_data
--build
--nobuild
--build_runfile_links
--nobuild_runfile_links
--build_tag_filters=
--build_test_dwp
--nobuild_test_dwp
--build_tests_only
--nobuild_tests_only
--cache_test_results={auto,yes,no}
--nocache_test_results
--check_constraint=
--check_fileset_dependencies_recursively
--nocheck_fileset_dependencies_recursively
--check_licenses
--nocheck_licenses
--check_tests_up_to_date
--nocheck_tests_up_to_date
--check_up_to_date
--nocheck_up_to_date
--check_visibility
--nocheck_visibility
--collect_code_coverage
--nocollect_code_coverage
--color={yes,no,auto}
--compilation_mode={fastbuild,dbg,opt}
--compile_one_dependency
--nocompile_one_dependency
--compiler=
--config=
--conlyopt=
--copt=
--coverage_report_generator=label
--coverage_support=label
--cpu=
--crosstool_top=label
--curses={yes,no,auto}
--custom_malloc=label
--cxxopt=
--dash_secret=
--dash_url=
--define=
--deleted_packages=
--deprecated_generate_xcode_project
--nodeprecated_generate_xcode_project
--device_debug_entitlements
--nodevice_debug_entitlements
--discard_analysis_cache
--nodiscard_analysis_cache
--distinct_host_configuration
--nodistinct_host_configuration
--dynamic_mode={off,default,fully}
--embed_label=
--enable_apple_binary_native_protos
--noenable_apple_binary_native_protos
--experimental_action_listener=
--experimental_android_jack_sanity_checks
--noexperimental_android_jack_sanity_checks
--experimental_android_use_jack_for_dexing
--noexperimental_android_use_jack_for_dexing
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_extra_action_filter=
--experimental_extra_action_top_level_only
--noexperimental_extra_action_top_level_only
--experimental_extra_action_top_level_only_with_aspects
--noexperimental_extra_action_top_level_only_with_aspects
--experimental_incremental_dexing_for_lite_protos
--noexperimental_incremental_dexing_for_lite_protos
--experimental_inmemory_dotd_files
--noexperimental_inmemory_dotd_files
--experimental_interleave_loading_and_analysis
--noexperimental_interleave_loading_and_analysis
--experimental_j2objc_srcjar_processing
--noexperimental_j2objc_srcjar_processing
--experimental_link_compile_output_separately
--noexperimental_link_compile_output_separately
--experimental_multi_cpu=
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_objc_header_thinning
--noexperimental_objc_header_thinning
--experimental_omitfp
--noexperimental_omitfp
--experimental_persistent_javac
--experimental_proto_extra_actions
--noexperimental_proto_extra_actions
--experimental_prune_more_modules
--noexperimental_prune_more_modules
--experimental_skip_static_outputs
--noexperimental_skip_static_outputs
--experimental_skip_unused_modules
--noexperimental_skip_unused_modules
--experimental_skyframe_native_filesets
--noexperimental_skyframe_native_filesets
--experimental_stl=label
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--experimental_use_llvm_covmap
--noexperimental_use_llvm_covmap
--explain=path
--explicit_jre_deps
--noexplicit_jre_deps
--extra_entitlements=label
--fat_apk_cpu=
--fdo_instrument=path
--fdo_optimize=
--features=
--fission=
--flaky_test_attempts=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--force_ignore_dash_static
--noforce_ignore_dash_static
--force_pic
--noforce_pic
--force_python={py2,py3,py2and3,py2only,py3only}
--genrule_strategy=
--glibc=
--grpc_max_batch_inputs=
--grpc_max_batch_size_bytes=
--grpc_max_chunk_size_bytes=
--grpc_timeout_seconds=
--grte_top=
--hazelcast_client_config=
--hazelcast_node=
--hazelcast_standalone_listen_port=
--host_copt=
--host_cpu=
--host_crosstool_top=label
--host_force_python={py2,py3,py2and3,py2only,py3only}
--host_grte_top=
--host_java_launcher=label
--host_java_toolchain=label
--host_javabase=
--ignore_unsupported_sandboxing
--noignore_unsupported_sandboxing
--incremental_dexing
--noincremental_dexing
--instrument_test_targets
--noinstrument_test_targets
--instrumentation_filter=
--interface_shared_objects
--nointerface_shared_objects
--ios_cpu=
--ios_memleaks
--noios_memleaks
--ios_minimum_os=
--ios_multi_cpus=
--ios_sdk_version=
--ios_signing_cert_name=
--ios_simulator_device=
--ios_simulator_version=
--j2objc_translation_flags=
--java_classpath={off,javabuilder,experimental_blaze}
--java_debug
--java_deps
--nojava_deps
--java_header_compilation
--nojava_header_compilation
--java_launcher=label
--java_toolchain=label
--javabase=
--javacopt=
--jobs=
--jvmopt=
--keep_going
--nokeep_going
--legacy_external_runfiles
--nolegacy_external_runfiles
--legacy_whole_archive
--nolegacy_whole_archive
--linkopt=
--lipo={off,binary}
--lipo_context=label
--loading_phase_threads=
--local_resources=
--local_test_jobs=
--logging=
--ltoindexopt=
--macos_cpus=
--macos_minimum_os=
--macos_sdk_version=
--message_translations=
--microcoverage
--nomicrocoverage
--non_incremental_per_target_dexopts=
--objc_enable_binary_stripping
--noobjc_enable_binary_stripping
--objc_generate_linkmap
--noobjc_generate_linkmap
--objc_includes_prioritize_static_libs
--noobjc_includes_prioritize_static_libs
--objc_use_dotd_pruning
--noobjc_use_dotd_pruning
--objccopt=
--output_descriptor_set
--nooutput_descriptor_set
--output_filter=
--output_symbol_counts
--nooutput_symbol_counts
--package_path=
--parse_headers_verifies_modules
--noparse_headers_verifies_modules
--per_file_copt=
--platform_suffix=
--plugin=
--plugin_copt=
--process_headers_in_dependencies
--noprocess_headers_in_dependencies
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--progress_report_interval=
--proguard_top=label
--proto_compiler=label
--proto_toolchain_for_cc=label
--proto_toolchain_for_java=label
--proto_toolchain_for_javalite=label
--protocopt=
--prune_cpp_modules
--noprune_cpp_modules
--python2_path=
--python3_path=
--ram_utilization_factor=
--remote_accept_cached
--noremote_accept_cached
--remote_allow_local_fallback
--noremote_allow_local_fallback
--remote_cache=
--remote_worker=
--resource_autosense
--noresource_autosense
--rest_cache_url=
--run_under=
--runs_per_test=
--runs_per_test_detects_flakes
--noruns_per_test_detects_flakes
--sandbox_add_mount_pair=
--sandbox_block_path=
--sandbox_debug
--nosandbox_debug
--sandbox_fake_hostname
--nosandbox_fake_hostname
--sandbox_tmpfs_path=
--save_temps
--nosave_temps
--send_transitive_header_module_srcs
--nosend_transitive_header_module_srcs
--share_native_deps
--noshare_native_deps
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_result=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--spawn_strategy=
--stamp
--nostamp
--start_end_lib
--nostart_end_lib
--strategy=
--strict_filesets
--nostrict_filesets
--strict_java_deps={off,warn,error,strict,default}
--strict_proto_deps={off,warn,error,strict,default}
--strict_system_includes
--nostrict_system_includes
--strip={always,sometimes,never}
--stripopt=
--subcommands
--nosubcommands
--swift_whole_module_optimization
--noswift_whole_module_optimization
--swiftcopt=
--symlink_prefix=
--target_environment=
--test_arg=
--test_env=
--test_filter=
--test_keep_going
--notest_keep_going
--test_lang_filters=
--test_output={summary,errors,all,streamed}
--test_result_expiration=
--test_sharding_strategy={explicit,experimental_heuristic,disabled}
--test_size_filters=
--test_strategy=
--test_summary={short,terse,detailed,none}
--test_tag_filters=
--test_timeout=
--test_timeout_filters=
--test_tmpdir=path
--test_verbose_timeout_warnings
--notest_verbose_timeout_warnings
--tool_tag=
--translations={auto,yes,no}
--notranslations
--tvos_cpus=
--tvos_minimum_os=
--tvos_sdk_version=
--tvos_simulator_device=
--tvos_simulator_version=
--use_dash
--nouse_dash
--use_ijars
--nouse_ijars
--verbose_explanations
--noverbose_explanations
--verbose_failures
--noverbose_failures
--verbose_test_summary
--noverbose_test_summary
--watchfs
--nowatchfs
--watchos_cpus=
--watchos_minimum_os=
--watchos_sdk_version=
--watchos_simulator_device=
--watchos_simulator_version=
--worker_extra_flag=
--worker_max_instances=
--worker_max_retries=
--worker_quit_after_build
--noworker_quit_after_build
--worker_sandboxing
--noworker_sandboxing
--worker_verbose
--noworker_verbose
--workspace_status_command=path
--xcode_override_workspace_root=
--xcode_toolchain=
--xcode_version=
"
BAZEL_COMMAND_DUMP_FLAGS="
--action_cache
--noaction_cache
--announce_rc
--noannounce_rc
--color={yes,no,auto}
--config=
--curses={yes,no,auto}
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--logging=
--packages
--nopackages
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--rule_classes
--norule_classes
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--skyframe={off,summary,detailed}
--tool_tag=
--vfs
--novfs
--watchfs
--nowatchfs
"
BAZEL_COMMAND_FETCH_ARGUMENT="label"
BAZEL_COMMAND_FETCH_FLAGS="
--announce_rc
--noannounce_rc
--color={yes,no,auto}
--config=
--curses={yes,no,auto}
--deleted_packages=
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--keep_going
--nokeep_going
--logging=
--package_path=
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--tool_tag=
--watchfs
--nowatchfs
"
BAZEL_COMMAND_HELP_ARGUMENT="command|{startup_options,target-syntax,info-keys}"
BAZEL_COMMAND_HELP_FLAGS="
--announce_rc
--noannounce_rc
--color={yes,no,auto}
--config=
--curses={yes,no,auto}
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--help_verbosity={long,medium,short}
--logging=
--long
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--short
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--tool_tag=
--watchfs
--nowatchfs
"
BAZEL_COMMAND_INFO_ARGUMENT="info-key"
BAZEL_COMMAND_INFO_FLAGS="
--action_env=
--analysis_warnings_as_errors
--noanalysis_warnings_as_errors
--android_compiler=
--android_cpu=
--android_crosstool_top=label
--android_manifest_merger={legacy,android}
--android_resource_shrinking
--noandroid_resource_shrinking
--android_sdk=label
--announce
--noannounce
--announce_rc
--noannounce_rc
--apple_bitcode={none,embedded_markers,embedded}
--apple_crosstool_top=label
--apple_generate_dsym
--noapple_generate_dsym
--autofdo_lipo_data
--noautofdo_lipo_data
--build
--nobuild
--build_runfile_links
--nobuild_runfile_links
--build_tag_filters=
--build_test_dwp
--nobuild_test_dwp
--build_tests_only
--nobuild_tests_only
--cache_test_results={auto,yes,no}
--nocache_test_results
--check_constraint=
--check_fileset_dependencies_recursively
--nocheck_fileset_dependencies_recursively
--check_licenses
--nocheck_licenses
--check_tests_up_to_date
--nocheck_tests_up_to_date
--check_up_to_date
--nocheck_up_to_date
--check_visibility
--nocheck_visibility
--collect_code_coverage
--nocollect_code_coverage
--color={yes,no,auto}
--compilation_mode={fastbuild,dbg,opt}
--compile_one_dependency
--nocompile_one_dependency
--compiler=
--config=
--conlyopt=
--copt=
--coverage_report_generator=label
--coverage_support=label
--cpu=
--crosstool_top=label
--curses={yes,no,auto}
--custom_malloc=label
--cxxopt=
--dash_secret=
--dash_url=
--define=
--deleted_packages=
--deprecated_generate_xcode_project
--nodeprecated_generate_xcode_project
--device_debug_entitlements
--nodevice_debug_entitlements
--discard_analysis_cache
--nodiscard_analysis_cache
--distinct_host_configuration
--nodistinct_host_configuration
--dynamic_mode={off,default,fully}
--embed_label=
--enable_apple_binary_native_protos
--noenable_apple_binary_native_protos
--experimental_action_listener=
--experimental_android_jack_sanity_checks
--noexperimental_android_jack_sanity_checks
--experimental_android_use_jack_for_dexing
--noexperimental_android_use_jack_for_dexing
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_extra_action_filter=
--experimental_extra_action_top_level_only
--noexperimental_extra_action_top_level_only
--experimental_extra_action_top_level_only_with_aspects
--noexperimental_extra_action_top_level_only_with_aspects
--experimental_incremental_dexing_for_lite_protos
--noexperimental_incremental_dexing_for_lite_protos
--experimental_inmemory_dotd_files
--noexperimental_inmemory_dotd_files
--experimental_interleave_loading_and_analysis
--noexperimental_interleave_loading_and_analysis
--experimental_j2objc_srcjar_processing
--noexperimental_j2objc_srcjar_processing
--experimental_link_compile_output_separately
--noexperimental_link_compile_output_separately
--experimental_multi_cpu=
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_objc_header_thinning
--noexperimental_objc_header_thinning
--experimental_omitfp
--noexperimental_omitfp
--experimental_persistent_javac
--experimental_proto_extra_actions
--noexperimental_proto_extra_actions
--experimental_prune_more_modules
--noexperimental_prune_more_modules
--experimental_skip_static_outputs
--noexperimental_skip_static_outputs
--experimental_skip_unused_modules
--noexperimental_skip_unused_modules
--experimental_skyframe_native_filesets
--noexperimental_skyframe_native_filesets
--experimental_stl=label
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--experimental_use_llvm_covmap
--noexperimental_use_llvm_covmap
--explain=path
--explicit_jre_deps
--noexplicit_jre_deps
--extra_entitlements=label
--fat_apk_cpu=
--fdo_instrument=path
--fdo_optimize=
--features=
--fission=
--flaky_test_attempts=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--force_ignore_dash_static
--noforce_ignore_dash_static
--force_pic
--noforce_pic
--force_python={py2,py3,py2and3,py2only,py3only}
--genrule_strategy=
--glibc=
--grpc_max_batch_inputs=
--grpc_max_batch_size_bytes=
--grpc_max_chunk_size_bytes=
--grpc_timeout_seconds=
--grte_top=
--hazelcast_client_config=
--hazelcast_node=
--hazelcast_standalone_listen_port=
--host_copt=
--host_cpu=
--host_crosstool_top=label
--host_force_python={py2,py3,py2and3,py2only,py3only}
--host_grte_top=
--host_java_launcher=label
--host_java_toolchain=label
--host_javabase=
--ignore_unsupported_sandboxing
--noignore_unsupported_sandboxing
--incremental_dexing
--noincremental_dexing
--instrument_test_targets
--noinstrument_test_targets
--instrumentation_filter=
--interface_shared_objects
--nointerface_shared_objects
--ios_cpu=
--ios_memleaks
--noios_memleaks
--ios_minimum_os=
--ios_multi_cpus=
--ios_sdk_version=
--ios_signing_cert_name=
--ios_simulator_device=
--ios_simulator_version=
--j2objc_translation_flags=
--java_classpath={off,javabuilder,experimental_blaze}
--java_debug
--java_deps
--nojava_deps
--java_header_compilation
--nojava_header_compilation
--java_launcher=label
--java_toolchain=label
--javabase=
--javacopt=
--jobs=
--jvmopt=
--keep_going
--nokeep_going
--legacy_external_runfiles
--nolegacy_external_runfiles
--legacy_whole_archive
--nolegacy_whole_archive
--linkopt=
--lipo={off,binary}
--lipo_context=label
--loading_phase_threads=
--local_resources=
--local_test_jobs=
--logging=
--ltoindexopt=
--macos_cpus=
--macos_minimum_os=
--macos_sdk_version=
--message_translations=
--microcoverage
--nomicrocoverage
--non_incremental_per_target_dexopts=
--objc_enable_binary_stripping
--noobjc_enable_binary_stripping
--objc_generate_linkmap
--noobjc_generate_linkmap
--objc_includes_prioritize_static_libs
--noobjc_includes_prioritize_static_libs
--objc_use_dotd_pruning
--noobjc_use_dotd_pruning
--objccopt=
--output_descriptor_set
--nooutput_descriptor_set
--output_filter=
--output_symbol_counts
--nooutput_symbol_counts
--package_path=
--parse_headers_verifies_modules
--noparse_headers_verifies_modules
--per_file_copt=
--platform_suffix=
--plugin=
--plugin_copt=
--process_headers_in_dependencies
--noprocess_headers_in_dependencies
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--progress_report_interval=
--proguard_top=label
--proto_compiler=label
--proto_toolchain_for_cc=label
--proto_toolchain_for_java=label
--proto_toolchain_for_javalite=label
--protocopt=
--prune_cpp_modules
--noprune_cpp_modules
--python2_path=
--python3_path=
--ram_utilization_factor=
--remote_accept_cached
--noremote_accept_cached
--remote_allow_local_fallback
--noremote_allow_local_fallback
--remote_cache=
--remote_worker=
--resource_autosense
--noresource_autosense
--rest_cache_url=
--run_under=
--runs_per_test=
--runs_per_test_detects_flakes
--noruns_per_test_detects_flakes
--sandbox_add_mount_pair=
--sandbox_block_path=
--sandbox_debug
--nosandbox_debug
--sandbox_fake_hostname
--nosandbox_fake_hostname
--sandbox_tmpfs_path=
--save_temps
--nosave_temps
--send_transitive_header_module_srcs
--nosend_transitive_header_module_srcs
--share_native_deps
--noshare_native_deps
--show_loading_progress
--noshow_loading_progress
--show_make_env
--noshow_make_env
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_result=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--spawn_strategy=
--stamp
--nostamp
--start_end_lib
--nostart_end_lib
--strategy=
--strict_filesets
--nostrict_filesets
--strict_java_deps={off,warn,error,strict,default}
--strict_proto_deps={off,warn,error,strict,default}
--strict_system_includes
--nostrict_system_includes
--strip={always,sometimes,never}
--stripopt=
--subcommands
--nosubcommands
--swift_whole_module_optimization
--noswift_whole_module_optimization
--swiftcopt=
--symlink_prefix=
--target_environment=
--test_arg=
--test_env=
--test_filter=
--test_keep_going
--notest_keep_going
--test_lang_filters=
--test_output={summary,errors,all,streamed}
--test_result_expiration=
--test_sharding_strategy={explicit,experimental_heuristic,disabled}
--test_size_filters=
--test_strategy=
--test_summary={short,terse,detailed,none}
--test_tag_filters=
--test_timeout=
--test_timeout_filters=
--test_tmpdir=path
--tool_tag=
--translations={auto,yes,no}
--notranslations
--tvos_cpus=
--tvos_minimum_os=
--tvos_sdk_version=
--tvos_simulator_device=
--tvos_simulator_version=
--use_dash
--nouse_dash
--use_ijars
--nouse_ijars
--verbose_explanations
--noverbose_explanations
--verbose_failures
--noverbose_failures
--watchfs
--nowatchfs
--watchos_cpus=
--watchos_minimum_os=
--watchos_sdk_version=
--watchos_simulator_device=
--watchos_simulator_version=
--worker_extra_flag=
--worker_max_instances=
--worker_max_retries=
--worker_quit_after_build
--noworker_quit_after_build
--worker_sandboxing
--noworker_sandboxing
--worker_verbose
--noworker_verbose
--workspace_status_command=path
--xcode_override_workspace_root=
--xcode_toolchain=
--xcode_version=
"
BAZEL_COMMAND_MOBILE_INSTALL_ARGUMENT="label"
BAZEL_COMMAND_MOBILE_INSTALL_FLAGS="
--action_env=
--adb=
--adb_arg=
--adb_jobs=
--analysis_warnings_as_errors
--noanalysis_warnings_as_errors
--android_compiler=
--android_cpu=
--android_crosstool_top=label
--android_manifest_merger={legacy,android}
--android_resource_shrinking
--noandroid_resource_shrinking
--android_sdk=label
--announce
--noannounce
--announce_rc
--noannounce_rc
--apple_bitcode={none,embedded_markers,embedded}
--apple_crosstool_top=label
--apple_generate_dsym
--noapple_generate_dsym
--autofdo_lipo_data
--noautofdo_lipo_data
--build
--nobuild
--build_runfile_links
--nobuild_runfile_links
--build_tag_filters=
--build_test_dwp
--nobuild_test_dwp
--build_tests_only
--nobuild_tests_only
--cache_test_results={auto,yes,no}
--nocache_test_results
--check_constraint=
--check_fileset_dependencies_recursively
--nocheck_fileset_dependencies_recursively
--check_licenses
--nocheck_licenses
--check_tests_up_to_date
--nocheck_tests_up_to_date
--check_up_to_date
--nocheck_up_to_date
--check_visibility
--nocheck_visibility
--collect_code_coverage
--nocollect_code_coverage
--color={yes,no,auto}
--compilation_mode={fastbuild,dbg,opt}
--compile_one_dependency
--nocompile_one_dependency
--compiler=
--config=
--conlyopt=
--copt=
--coverage_report_generator=label
--coverage_support=label
--cpu=
--crosstool_top=label
--curses={yes,no,auto}
--custom_malloc=label
--cxxopt=
--dash_secret=
--dash_url=
--define=
--deleted_packages=
--deprecated_generate_xcode_project
--nodeprecated_generate_xcode_project
--device_debug_entitlements
--nodevice_debug_entitlements
--discard_analysis_cache
--nodiscard_analysis_cache
--distinct_host_configuration
--nodistinct_host_configuration
--dynamic_mode={off,default,fully}
--embed_label=
--enable_apple_binary_native_protos
--noenable_apple_binary_native_protos
--experimental_action_listener=
--experimental_android_jack_sanity_checks
--noexperimental_android_jack_sanity_checks
--experimental_android_use_jack_for_dexing
--noexperimental_android_use_jack_for_dexing
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_extra_action_filter=
--experimental_extra_action_top_level_only
--noexperimental_extra_action_top_level_only
--experimental_extra_action_top_level_only_with_aspects
--noexperimental_extra_action_top_level_only_with_aspects
--experimental_incremental_dexing_for_lite_protos
--noexperimental_incremental_dexing_for_lite_protos
--experimental_inmemory_dotd_files
--noexperimental_inmemory_dotd_files
--experimental_interleave_loading_and_analysis
--noexperimental_interleave_loading_and_analysis
--experimental_j2objc_srcjar_processing
--noexperimental_j2objc_srcjar_processing
--experimental_link_compile_output_separately
--noexperimental_link_compile_output_separately
--experimental_multi_cpu=
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_objc_header_thinning
--noexperimental_objc_header_thinning
--experimental_omitfp
--noexperimental_omitfp
--experimental_persistent_javac
--experimental_proto_extra_actions
--noexperimental_proto_extra_actions
--experimental_prune_more_modules
--noexperimental_prune_more_modules
--experimental_skip_static_outputs
--noexperimental_skip_static_outputs
--experimental_skip_unused_modules
--noexperimental_skip_unused_modules
--experimental_skyframe_native_filesets
--noexperimental_skyframe_native_filesets
--experimental_stl=label
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--experimental_use_llvm_covmap
--noexperimental_use_llvm_covmap
--explain=path
--explicit_jre_deps
--noexplicit_jre_deps
--extra_entitlements=label
--fat_apk_cpu=
--fdo_instrument=path
--fdo_optimize=
--features=
--fission=
--flaky_test_attempts=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--force_ignore_dash_static
--noforce_ignore_dash_static
--force_pic
--noforce_pic
--force_python={py2,py3,py2and3,py2only,py3only}
--genrule_strategy=
--glibc=
--grpc_max_batch_inputs=
--grpc_max_batch_size_bytes=
--grpc_max_chunk_size_bytes=
--grpc_timeout_seconds=
--grte_top=
--hazelcast_client_config=
--hazelcast_node=
--hazelcast_standalone_listen_port=
--host_copt=
--host_cpu=
--host_crosstool_top=label
--host_force_python={py2,py3,py2and3,py2only,py3only}
--host_grte_top=
--host_java_launcher=label
--host_java_toolchain=label
--host_javabase=
--ignore_unsupported_sandboxing
--noignore_unsupported_sandboxing
--incremental
--noincremental
--incremental_dexing
--noincremental_dexing
--incremental_install_verbosity=
--instrument_test_targets
--noinstrument_test_targets
--instrumentation_filter=
--interface_shared_objects
--nointerface_shared_objects
--ios_cpu=
--ios_memleaks
--noios_memleaks
--ios_minimum_os=
--ios_multi_cpus=
--ios_sdk_version=
--ios_signing_cert_name=
--ios_simulator_device=
--ios_simulator_version=
--j2objc_translation_flags=
--java_classpath={off,javabuilder,experimental_blaze}
--java_debug
--java_deps
--nojava_deps
--java_header_compilation
--nojava_header_compilation
--java_launcher=label
--java_toolchain=label
--javabase=
--javacopt=
--jobs=
--jvmopt=
--keep_going
--nokeep_going
--legacy_external_runfiles
--nolegacy_external_runfiles
--legacy_whole_archive
--nolegacy_whole_archive
--linkopt=
--lipo={off,binary}
--lipo_context=label
--loading_phase_threads=
--local_resources=
--local_test_jobs=
--logging=
--ltoindexopt=
--macos_cpus=
--macos_minimum_os=
--macos_sdk_version=
--message_translations=
--microcoverage
--nomicrocoverage
--non_incremental_per_target_dexopts=
--objc_enable_binary_stripping
--noobjc_enable_binary_stripping
--objc_generate_linkmap
--noobjc_generate_linkmap
--objc_includes_prioritize_static_libs
--noobjc_includes_prioritize_static_libs
--objc_use_dotd_pruning
--noobjc_use_dotd_pruning
--objccopt=
--output_descriptor_set
--nooutput_descriptor_set
--output_filter=
--output_symbol_counts
--nooutput_symbol_counts
--package_path=
--parse_headers_verifies_modules
--noparse_headers_verifies_modules
--per_file_copt=
--platform_suffix=
--plugin=
--plugin_copt=
--process_headers_in_dependencies
--noprocess_headers_in_dependencies
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--progress_report_interval=
--proguard_top=label
--proto_compiler=label
--proto_toolchain_for_cc=label
--proto_toolchain_for_java=label
--proto_toolchain_for_javalite=label
--protocopt=
--prune_cpp_modules
--noprune_cpp_modules
--python2_path=
--python3_path=
--ram_utilization_factor=
--remote_accept_cached
--noremote_accept_cached
--remote_allow_local_fallback
--noremote_allow_local_fallback
--remote_cache=
--remote_worker=
--resource_autosense
--noresource_autosense
--rest_cache_url=
--run_under=
--runs_per_test=
--runs_per_test_detects_flakes
--noruns_per_test_detects_flakes
--sandbox_add_mount_pair=
--sandbox_block_path=
--sandbox_debug
--nosandbox_debug
--sandbox_fake_hostname
--nosandbox_fake_hostname
--sandbox_tmpfs_path=
--save_temps
--nosave_temps
--send_transitive_header_module_srcs
--nosend_transitive_header_module_srcs
--share_native_deps
--noshare_native_deps
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_result=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--spawn_strategy=
--split_apks
--nosplit_apks
--stamp
--nostamp
--start={no,cold,warm}
--start_app
--start_end_lib
--nostart_end_lib
--strategy=
--strict_filesets
--nostrict_filesets
--strict_java_deps={off,warn,error,strict,default}
--strict_proto_deps={off,warn,error,strict,default}
--strict_system_includes
--nostrict_system_includes
--strip={always,sometimes,never}
--stripopt=
--subcommands
--nosubcommands
--swift_whole_module_optimization
--noswift_whole_module_optimization
--swiftcopt=
--symlink_prefix=
--target_environment=
--test_arg=
--test_env=
--test_filter=
--test_keep_going
--notest_keep_going
--test_lang_filters=
--test_output={summary,errors,all,streamed}
--test_result_expiration=
--test_sharding_strategy={explicit,experimental_heuristic,disabled}
--test_size_filters=
--test_strategy=
--test_summary={short,terse,detailed,none}
--test_tag_filters=
--test_timeout=
--test_timeout_filters=
--test_tmpdir=path
--tool_tag=
--translations={auto,yes,no}
--notranslations
--tvos_cpus=
--tvos_minimum_os=
--tvos_sdk_version=
--tvos_simulator_device=
--tvos_simulator_version=
--use_dash
--nouse_dash
--use_ijars
--nouse_ijars
--verbose_explanations
--noverbose_explanations
--verbose_failures
--noverbose_failures
--watchfs
--nowatchfs
--watchos_cpus=
--watchos_minimum_os=
--watchos_sdk_version=
--watchos_simulator_device=
--watchos_simulator_version=
--worker_extra_flag=
--worker_max_instances=
--worker_max_retries=
--worker_quit_after_build
--noworker_quit_after_build
--worker_sandboxing
--noworker_sandboxing
--worker_verbose
--noworker_verbose
--workspace_status_command=path
--xcode_override_workspace_root=
--xcode_toolchain=
--xcode_version=
"
BAZEL_COMMAND_QUERY_ARGUMENT="label"
BAZEL_COMMAND_QUERY_FLAGS="
--announce_rc
--noannounce_rc
--aspect_deps={off,conservative,precise}
--color={yes,no,auto}
--config=
--curses={yes,no,auto}
--deleted_packages=
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--graph:factored
--nograph:factored
--graph:node_limit=
--host_deps
--nohost_deps
--implicit_deps
--noimplicit_deps
--keep_going
--nokeep_going
--line_terminator_null
--noline_terminator_null
--logging=
--noorder_results
--null
--order_output={no,deps,auto,full}
--order_results
--output=
--package_path=
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--proto:default_values
--noproto:default_values
--query_file=
--relative_locations
--norelative_locations
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--strict_test_suite
--nostrict_test_suite
--tool_tag=
--universe_scope=
--watchfs
--nowatchfs
--xml:default_values
--noxml:default_values
--xml:line_numbers
--noxml:line_numbers
"
BAZEL_COMMAND_RUN_ARGUMENT="label-bin"
BAZEL_COMMAND_RUN_FLAGS="
--action_env=
--analysis_warnings_as_errors
--noanalysis_warnings_as_errors
--android_compiler=
--android_cpu=
--android_crosstool_top=label
--android_manifest_merger={legacy,android}
--android_resource_shrinking
--noandroid_resource_shrinking
--android_sdk=label
--announce
--noannounce
--announce_rc
--noannounce_rc
--apple_bitcode={none,embedded_markers,embedded}
--apple_crosstool_top=label
--apple_generate_dsym
--noapple_generate_dsym
--autofdo_lipo_data
--noautofdo_lipo_data
--build
--nobuild
--build_runfile_links
--nobuild_runfile_links
--build_tag_filters=
--build_test_dwp
--nobuild_test_dwp
--build_tests_only
--nobuild_tests_only
--cache_test_results={auto,yes,no}
--nocache_test_results
--check_constraint=
--check_fileset_dependencies_recursively
--nocheck_fileset_dependencies_recursively
--check_licenses
--nocheck_licenses
--check_tests_up_to_date
--nocheck_tests_up_to_date
--check_up_to_date
--nocheck_up_to_date
--check_visibility
--nocheck_visibility
--collect_code_coverage
--nocollect_code_coverage
--color={yes,no,auto}
--compilation_mode={fastbuild,dbg,opt}
--compile_one_dependency
--nocompile_one_dependency
--compiler=
--config=
--conlyopt=
--copt=
--coverage_report_generator=label
--coverage_support=label
--cpu=
--crosstool_top=label
--curses={yes,no,auto}
--custom_malloc=label
--cxxopt=
--dash_secret=
--dash_url=
--define=
--deleted_packages=
--deprecated_generate_xcode_project
--nodeprecated_generate_xcode_project
--device_debug_entitlements
--nodevice_debug_entitlements
--discard_analysis_cache
--nodiscard_analysis_cache
--distinct_host_configuration
--nodistinct_host_configuration
--dynamic_mode={off,default,fully}
--embed_label=
--enable_apple_binary_native_protos
--noenable_apple_binary_native_protos
--experimental_action_listener=
--experimental_android_jack_sanity_checks
--noexperimental_android_jack_sanity_checks
--experimental_android_use_jack_for_dexing
--noexperimental_android_use_jack_for_dexing
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_extra_action_filter=
--experimental_extra_action_top_level_only
--noexperimental_extra_action_top_level_only
--experimental_extra_action_top_level_only_with_aspects
--noexperimental_extra_action_top_level_only_with_aspects
--experimental_incremental_dexing_for_lite_protos
--noexperimental_incremental_dexing_for_lite_protos
--experimental_inmemory_dotd_files
--noexperimental_inmemory_dotd_files
--experimental_interleave_loading_and_analysis
--noexperimental_interleave_loading_and_analysis
--experimental_j2objc_srcjar_processing
--noexperimental_j2objc_srcjar_processing
--experimental_link_compile_output_separately
--noexperimental_link_compile_output_separately
--experimental_multi_cpu=
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_objc_header_thinning
--noexperimental_objc_header_thinning
--experimental_omitfp
--noexperimental_omitfp
--experimental_persistent_javac
--experimental_proto_extra_actions
--noexperimental_proto_extra_actions
--experimental_prune_more_modules
--noexperimental_prune_more_modules
--experimental_skip_static_outputs
--noexperimental_skip_static_outputs
--experimental_skip_unused_modules
--noexperimental_skip_unused_modules
--experimental_skyframe_native_filesets
--noexperimental_skyframe_native_filesets
--experimental_stl=label
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--experimental_use_llvm_covmap
--noexperimental_use_llvm_covmap
--explain=path
--explicit_jre_deps
--noexplicit_jre_deps
--extra_entitlements=label
--fat_apk_cpu=
--fdo_instrument=path
--fdo_optimize=
--features=
--fission=
--flaky_test_attempts=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--force_ignore_dash_static
--noforce_ignore_dash_static
--force_pic
--noforce_pic
--force_python={py2,py3,py2and3,py2only,py3only}
--genrule_strategy=
--glibc=
--grpc_max_batch_inputs=
--grpc_max_batch_size_bytes=
--grpc_max_chunk_size_bytes=
--grpc_timeout_seconds=
--grte_top=
--hazelcast_client_config=
--hazelcast_node=
--hazelcast_standalone_listen_port=
--host_copt=
--host_cpu=
--host_crosstool_top=label
--host_force_python={py2,py3,py2and3,py2only,py3only}
--host_grte_top=
--host_java_launcher=label
--host_java_toolchain=label
--host_javabase=
--ignore_unsupported_sandboxing
--noignore_unsupported_sandboxing
--incremental_dexing
--noincremental_dexing
--instrument_test_targets
--noinstrument_test_targets
--instrumentation_filter=
--interface_shared_objects
--nointerface_shared_objects
--ios_cpu=
--ios_memleaks
--noios_memleaks
--ios_minimum_os=
--ios_multi_cpus=
--ios_sdk_version=
--ios_signing_cert_name=
--ios_simulator_device=
--ios_simulator_version=
--j2objc_translation_flags=
--java_classpath={off,javabuilder,experimental_blaze}
--java_debug
--java_deps
--nojava_deps
--java_header_compilation
--nojava_header_compilation
--java_launcher=label
--java_toolchain=label
--javabase=
--javacopt=
--jobs=
--jvmopt=
--keep_going
--nokeep_going
--legacy_external_runfiles
--nolegacy_external_runfiles
--legacy_whole_archive
--nolegacy_whole_archive
--linkopt=
--lipo={off,binary}
--lipo_context=label
--loading_phase_threads=
--local_resources=
--local_test_jobs=
--logging=
--ltoindexopt=
--macos_cpus=
--macos_minimum_os=
--macos_sdk_version=
--message_translations=
--microcoverage
--nomicrocoverage
--non_incremental_per_target_dexopts=
--objc_enable_binary_stripping
--noobjc_enable_binary_stripping
--objc_generate_linkmap
--noobjc_generate_linkmap
--objc_includes_prioritize_static_libs
--noobjc_includes_prioritize_static_libs
--objc_use_dotd_pruning
--noobjc_use_dotd_pruning
--objccopt=
--output_descriptor_set
--nooutput_descriptor_set
--output_filter=
--output_symbol_counts
--nooutput_symbol_counts
--package_path=
--parse_headers_verifies_modules
--noparse_headers_verifies_modules
--per_file_copt=
--platform_suffix=
--plugin=
--plugin_copt=
--process_headers_in_dependencies
--noprocess_headers_in_dependencies
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--progress_report_interval=
--proguard_top=label
--proto_compiler=label
--proto_toolchain_for_cc=label
--proto_toolchain_for_java=label
--proto_toolchain_for_javalite=label
--protocopt=
--prune_cpp_modules
--noprune_cpp_modules
--python2_path=
--python3_path=
--ram_utilization_factor=
--remote_accept_cached
--noremote_accept_cached
--remote_allow_local_fallback
--noremote_allow_local_fallback
--remote_cache=
--remote_worker=
--resource_autosense
--noresource_autosense
--rest_cache_url=
--run_under=
--runs_per_test=
--runs_per_test_detects_flakes
--noruns_per_test_detects_flakes
--sandbox_add_mount_pair=
--sandbox_block_path=
--sandbox_debug
--nosandbox_debug
--sandbox_fake_hostname
--nosandbox_fake_hostname
--sandbox_tmpfs_path=
--save_temps
--nosave_temps
--script_path=path
--send_transitive_header_module_srcs
--nosend_transitive_header_module_srcs
--share_native_deps
--noshare_native_deps
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_result=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--spawn_strategy=
--stamp
--nostamp
--start_end_lib
--nostart_end_lib
--strategy=
--strict_filesets
--nostrict_filesets
--strict_java_deps={off,warn,error,strict,default}
--strict_proto_deps={off,warn,error,strict,default}
--strict_system_includes
--nostrict_system_includes
--strip={always,sometimes,never}
--stripopt=
--subcommands
--nosubcommands
--swift_whole_module_optimization
--noswift_whole_module_optimization
--swiftcopt=
--symlink_prefix=
--target_environment=
--test_arg=
--test_env=
--test_filter=
--test_keep_going
--notest_keep_going
--test_lang_filters=
--test_output={summary,errors,all,streamed}
--test_result_expiration=
--test_sharding_strategy={explicit,experimental_heuristic,disabled}
--test_size_filters=
--test_strategy=
--test_summary={short,terse,detailed,none}
--test_tag_filters=
--test_timeout=
--test_timeout_filters=
--test_tmpdir=path
--tool_tag=
--translations={auto,yes,no}
--notranslations
--tvos_cpus=
--tvos_minimum_os=
--tvos_sdk_version=
--tvos_simulator_device=
--tvos_simulator_version=
--use_dash
--nouse_dash
--use_ijars
--nouse_ijars
--verbose_explanations
--noverbose_explanations
--verbose_failures
--noverbose_failures
--watchfs
--nowatchfs
--watchos_cpus=
--watchos_minimum_os=
--watchos_sdk_version=
--watchos_simulator_device=
--watchos_simulator_version=
--worker_extra_flag=
--worker_max_instances=
--worker_max_retries=
--worker_quit_after_build
--noworker_quit_after_build
--worker_sandboxing
--noworker_sandboxing
--worker_verbose
--noworker_verbose
--workspace_status_command=path
--xcode_override_workspace_root=
--xcode_toolchain=
--xcode_version=
"
BAZEL_COMMAND_SHUTDOWN_FLAGS="
--announce_rc
--noannounce_rc
--color={yes,no,auto}
--config=
--curses={yes,no,auto}
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--iff_heap_size_greater_than=
--logging=
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--tool_tag=
--watchfs
--nowatchfs
"
BAZEL_COMMAND_TEST_ARGUMENT="label-test"
BAZEL_COMMAND_TEST_FLAGS="
--action_env=
--analysis_warnings_as_errors
--noanalysis_warnings_as_errors
--android_compiler=
--android_cpu=
--android_crosstool_top=label
--android_manifest_merger={legacy,android}
--android_resource_shrinking
--noandroid_resource_shrinking
--android_sdk=label
--announce
--noannounce
--announce_rc
--noannounce_rc
--apple_bitcode={none,embedded_markers,embedded}
--apple_crosstool_top=label
--apple_generate_dsym
--noapple_generate_dsym
--autofdo_lipo_data
--noautofdo_lipo_data
--build
--nobuild
--build_runfile_links
--nobuild_runfile_links
--build_tag_filters=
--build_test_dwp
--nobuild_test_dwp
--build_tests_only
--nobuild_tests_only
--cache_test_results={auto,yes,no}
--nocache_test_results
--check_constraint=
--check_fileset_dependencies_recursively
--nocheck_fileset_dependencies_recursively
--check_licenses
--nocheck_licenses
--check_tests_up_to_date
--nocheck_tests_up_to_date
--check_up_to_date
--nocheck_up_to_date
--check_visibility
--nocheck_visibility
--collect_code_coverage
--nocollect_code_coverage
--color={yes,no,auto}
--compilation_mode={fastbuild,dbg,opt}
--compile_one_dependency
--nocompile_one_dependency
--compiler=
--config=
--conlyopt=
--copt=
--coverage_report_generator=label
--coverage_support=label
--cpu=
--crosstool_top=label
--curses={yes,no,auto}
--custom_malloc=label
--cxxopt=
--dash_secret=
--dash_url=
--define=
--deleted_packages=
--deprecated_generate_xcode_project
--nodeprecated_generate_xcode_project
--device_debug_entitlements
--nodevice_debug_entitlements
--discard_analysis_cache
--nodiscard_analysis_cache
--distinct_host_configuration
--nodistinct_host_configuration
--dynamic_mode={off,default,fully}
--embed_label=
--enable_apple_binary_native_protos
--noenable_apple_binary_native_protos
--experimental_action_listener=
--experimental_android_jack_sanity_checks
--noexperimental_android_jack_sanity_checks
--experimental_android_use_jack_for_dexing
--noexperimental_android_use_jack_for_dexing
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_extra_action_filter=
--experimental_extra_action_top_level_only
--noexperimental_extra_action_top_level_only
--experimental_extra_action_top_level_only_with_aspects
--noexperimental_extra_action_top_level_only_with_aspects
--experimental_incremental_dexing_for_lite_protos
--noexperimental_incremental_dexing_for_lite_protos
--experimental_inmemory_dotd_files
--noexperimental_inmemory_dotd_files
--experimental_interleave_loading_and_analysis
--noexperimental_interleave_loading_and_analysis
--experimental_j2objc_srcjar_processing
--noexperimental_j2objc_srcjar_processing
--experimental_link_compile_output_separately
--noexperimental_link_compile_output_separately
--experimental_multi_cpu=
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_objc_header_thinning
--noexperimental_objc_header_thinning
--experimental_omitfp
--noexperimental_omitfp
--experimental_persistent_javac
--experimental_proto_extra_actions
--noexperimental_proto_extra_actions
--experimental_prune_more_modules
--noexperimental_prune_more_modules
--experimental_skip_static_outputs
--noexperimental_skip_static_outputs
--experimental_skip_unused_modules
--noexperimental_skip_unused_modules
--experimental_skyframe_native_filesets
--noexperimental_skyframe_native_filesets
--experimental_stl=label
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--experimental_use_llvm_covmap
--noexperimental_use_llvm_covmap
--explain=path
--explicit_jre_deps
--noexplicit_jre_deps
--extra_entitlements=label
--fat_apk_cpu=
--fdo_instrument=path
--fdo_optimize=
--features=
--fission=
--flaky_test_attempts=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--force_ignore_dash_static
--noforce_ignore_dash_static
--force_pic
--noforce_pic
--force_python={py2,py3,py2and3,py2only,py3only}
--genrule_strategy=
--glibc=
--grpc_max_batch_inputs=
--grpc_max_batch_size_bytes=
--grpc_max_chunk_size_bytes=
--grpc_timeout_seconds=
--grte_top=
--hazelcast_client_config=
--hazelcast_node=
--hazelcast_standalone_listen_port=
--host_copt=
--host_cpu=
--host_crosstool_top=label
--host_force_python={py2,py3,py2and3,py2only,py3only}
--host_grte_top=
--host_java_launcher=label
--host_java_toolchain=label
--host_javabase=
--ignore_unsupported_sandboxing
--noignore_unsupported_sandboxing
--incremental_dexing
--noincremental_dexing
--instrument_test_targets
--noinstrument_test_targets
--instrumentation_filter=
--interface_shared_objects
--nointerface_shared_objects
--ios_cpu=
--ios_memleaks
--noios_memleaks
--ios_minimum_os=
--ios_multi_cpus=
--ios_sdk_version=
--ios_signing_cert_name=
--ios_simulator_device=
--ios_simulator_version=
--j2objc_translation_flags=
--java_classpath={off,javabuilder,experimental_blaze}
--java_debug
--java_deps
--nojava_deps
--java_header_compilation
--nojava_header_compilation
--java_launcher=label
--java_toolchain=label
--javabase=
--javacopt=
--jobs=
--jvmopt=
--keep_going
--nokeep_going
--legacy_external_runfiles
--nolegacy_external_runfiles
--legacy_whole_archive
--nolegacy_whole_archive
--linkopt=
--lipo={off,binary}
--lipo_context=label
--loading_phase_threads=
--local_resources=
--local_test_jobs=
--logging=
--ltoindexopt=
--macos_cpus=
--macos_minimum_os=
--macos_sdk_version=
--message_translations=
--microcoverage
--nomicrocoverage
--non_incremental_per_target_dexopts=
--objc_enable_binary_stripping
--noobjc_enable_binary_stripping
--objc_generate_linkmap
--noobjc_generate_linkmap
--objc_includes_prioritize_static_libs
--noobjc_includes_prioritize_static_libs
--objc_use_dotd_pruning
--noobjc_use_dotd_pruning
--objccopt=
--output_descriptor_set
--nooutput_descriptor_set
--output_filter=
--output_symbol_counts
--nooutput_symbol_counts
--package_path=
--parse_headers_verifies_modules
--noparse_headers_verifies_modules
--per_file_copt=
--platform_suffix=
--plugin=
--plugin_copt=
--process_headers_in_dependencies
--noprocess_headers_in_dependencies
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--progress_report_interval=
--proguard_top=label
--proto_compiler=label
--proto_toolchain_for_cc=label
--proto_toolchain_for_java=label
--proto_toolchain_for_javalite=label
--protocopt=
--prune_cpp_modules
--noprune_cpp_modules
--python2_path=
--python3_path=
--ram_utilization_factor=
--remote_accept_cached
--noremote_accept_cached
--remote_allow_local_fallback
--noremote_allow_local_fallback
--remote_cache=
--remote_worker=
--resource_autosense
--noresource_autosense
--rest_cache_url=
--run_under=
--runs_per_test=
--runs_per_test_detects_flakes
--noruns_per_test_detects_flakes
--sandbox_add_mount_pair=
--sandbox_block_path=
--sandbox_debug
--nosandbox_debug
--sandbox_fake_hostname
--nosandbox_fake_hostname
--sandbox_tmpfs_path=
--save_temps
--nosave_temps
--send_transitive_header_module_srcs
--nosend_transitive_header_module_srcs
--share_native_deps
--noshare_native_deps
--show_loading_progress
--noshow_loading_progress
--show_package_location
--noshow_package_location
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_result=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--spawn_strategy=
--stamp
--nostamp
--start_end_lib
--nostart_end_lib
--strategy=
--strict_filesets
--nostrict_filesets
--strict_java_deps={off,warn,error,strict,default}
--strict_proto_deps={off,warn,error,strict,default}
--strict_system_includes
--nostrict_system_includes
--strip={always,sometimes,never}
--stripopt=
--subcommands
--nosubcommands
--swift_whole_module_optimization
--noswift_whole_module_optimization
--swiftcopt=
--symlink_prefix=
--target_environment=
--test_arg=
--test_env=
--test_filter=
--test_keep_going
--notest_keep_going
--test_lang_filters=
--test_output={summary,errors,all,streamed}
--test_result_expiration=
--test_sharding_strategy={explicit,experimental_heuristic,disabled}
--test_size_filters=
--test_strategy=
--test_summary={short,terse,detailed,none}
--test_tag_filters=
--test_timeout=
--test_timeout_filters=
--test_tmpdir=path
--test_verbose_timeout_warnings
--notest_verbose_timeout_warnings
--tool_tag=
--translations={auto,yes,no}
--notranslations
--tvos_cpus=
--tvos_minimum_os=
--tvos_sdk_version=
--tvos_simulator_device=
--tvos_simulator_version=
--use_dash
--nouse_dash
--use_ijars
--nouse_ijars
--verbose_explanations
--noverbose_explanations
--verbose_failures
--noverbose_failures
--verbose_test_summary
--noverbose_test_summary
--watchfs
--nowatchfs
--watchos_cpus=
--watchos_minimum_os=
--watchos_sdk_version=
--watchos_simulator_device=
--watchos_simulator_version=
--worker_extra_flag=
--worker_max_instances=
--worker_max_retries=
--worker_quit_after_build
--noworker_quit_after_build
--worker_sandboxing
--noworker_sandboxing
--worker_verbose
--noworker_verbose
--workspace_status_command=path
--xcode_override_workspace_root=
--xcode_toolchain=
--xcode_version=
"
BAZEL_COMMAND_VERSION_FLAGS="
--announce_rc
--noannounce_rc
--color={yes,no,auto}
--config=
--curses={yes,no,auto}
--experimental_external_repositories
--noexperimental_external_repositories
--experimental_multi_threaded_digest
--noexperimental_multi_threaded_digest
--experimental_ui
--noexperimental_ui
--experimental_ui_actions_shown=
--force_experimental_external_repositories
--noforce_experimental_external_repositories
--logging=
--profile=path
--progress_in_terminal_title
--noprogress_in_terminal_title
--show_progress
--noshow_progress
--show_progress_rate_limit=
--show_task_finish
--noshow_task_finish
--show_timestamps
--noshow_timestamps
--tool_tag=
--watchfs
--nowatchfs
"
