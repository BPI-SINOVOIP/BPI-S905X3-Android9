# Copyright 2016 - The Android Open Source Project
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


class SshFormatter(object):
    """Handles formatting ssh commands.

    Handler for formatting chunks of the ssh command to run.
    """

    def format_ssh_executable(self, settings):
        """Format the executable name.

        Formats the executable name as a string.

        Args:
            settings: The ssh settings being used.

        Returns:
            A string for the ssh executable name.
        """
        return settings.executable

    def format_host_name(self, settings):
        """Format hostname.

        Formats the hostname to connect to.

        Args:
            settings: The ssh settings being used.

        Returns:
            A string of the connection host name to connect to.
        """
        return '%s@%s' % (settings.username, settings.hostname)

    def format_value(self, value):
        """Formats a command line value.

        Takes in a value and formats it so it can be safely used in the
        command line.

        Args:
            value: The value to format.

        Returns:
            A string representation of the formatted value.
        """
        if isinstance(value, bool):
            return 'yes' if value else 'no'

        return str(value)

    def format_options_list(self, options):
        """Format the option list.

        Formats a dictionary of options into a list of strings to be used
        on the command line.

        Args:
            options: A dictionary of options.

        Returns:
            An iterator of strings that should go on the command line.
        """
        for option_name in options:
            option = options[option_name]

            yield '-o'
            yield '%s=%s' % (option_name, self.format_value(option))

    def format_flag_list(self, flags):
        """Format the flags list.

        Formats a dictionary of flags into a list of strings to be used
        on the command line.

        Args:
            flags: A dictonary of options.

        Returns:
            An iterator of strings that should be used on the command line.
        """
        for flag_name in flags:
            flag = flags[flag_name]

            yield flag_name
            if flag is not None:
                yield self.format_value(flag)

    def format_ssh_local_command(self,
                                 settings,
                                 extra_flags={},
                                 extra_options={}):
        """Formats the local part of the ssh command.

        Formats the local section of the ssh command. This is the part of the
        command that will actual launch ssh on our local machine with the
        specified settings.

        Args:
            settings: The ssh settings.
            extra_flags: Extra flags to inlcude.
            extra_options: Extra options to include.

        Returns:
            An array of strings that make up the command and its local
            arguments.
        """
        options = settings.construct_ssh_options()
        for extra_option_name in extra_options:
            options[extra_option_name] = extra_options[extra_option_name]
        options_list = list(self.format_options_list(options))

        flags = settings.construct_ssh_flags()
        for extra_flag_name in extra_flags:
            flags[extra_flag_name] = extra_flags[extra_flag_name]
        flags_list = list(self.format_flag_list(flags))

        all_options = options_list + flags_list
        host_name = self.format_host_name(settings)
        executable = self.format_ssh_executable(settings)

        base_command = [executable] + all_options + [host_name]

        return base_command

    def format_ssh_command(self,
                           remote_command,
                           settings,
                           extra_flags={},
                           extra_options={}):
        """Formats the full ssh command.

        Creates the full format for an ssh command.

        Args:
            remote_command: A string that represents the remote command to
                            execute.
            settings: The ssh settings to use.
            extra_flags: Extra flags to include in the settings.
            extra_options: Extra options to include in the settings.

        Returns:
            A list of strings that make up the total ssh command.
        """
        local_command = self.format_ssh_local_command(settings, extra_flags,
                                                      extra_options)

        local_command.append(remote_command)
        return local_command

    def format_remote_command(self, command, env):
        """Formats the remote part of the ssh command.

        Formatts the command that will run on the remote machine.

        Args:
            command: string, The command to be executed.
            env: Enviroment variables to add to the remote envirment.

        Returns:
            A string that represents the command line to execute on the remote
            machine.
        """
        if not env:
            env_str = ''
        else:
            env_str = 'export '
            for name in env:
                value = env[name]
                env_str += '%s=%s ' % (name, str(value))
            env_str += ';'

        execution_line = '%s %s;' % (env_str, command)
        return execution_line

    def format_command(self,
                       command,
                       env,
                       settings,
                       extra_flags={},
                       extra_options={}):
        """Formats a full command.

        Formats the full command to run in order to run a command on a remote
        machine.

        Args:
            command: The command to run on the remote machine. Can either be
                     a string or a list.
            env: The enviroment variables to include on the remote machine.
            settings: The ssh settings to use.
            extra_flags: Extra flags to include with the settings.
            extra_options: Extra options to include with the settings.
        """
        remote_command = self.format_remote_command(command, env)
        return self.format_ssh_command(remote_command, settings, extra_flags,
                                       extra_options)
