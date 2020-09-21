#
# Copyright 2018 - The Android Open Source Project
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
#

import imp

from fabric.api import env
from fabric.api import sudo


def SetPassword(password):
    """Sets password for hosts to access through ssh and to run sudo commands

    usage: $ fab SetPassword:<password for hosts>

    Args:
        password: string, password for hosts.
    """
    env.password = password

def GetHosts(hosts_file_path):
    """Configures env.hosts to a given list of hosts.

    usage: $ fab GetHosts:<path to a source file contains hosts info>

    Args:
        hosts_file_path: string, path to a python file passed from command file
                         input.
    """
    hosts_module = imp.load_source('hosts_module', hosts_file_path)
    env.hosts = hosts_module.EmitHostList()

def SetupIptables(ip_address_file_path):
    """Configures iptables setting for all hosts listed.

    usage: $ fab SetupIptables:<path to a source file contains ip addresses of
             certified machines>

    Args:
        ip_address_file_path: string, path to a python file passed from command
                              file input.
    """
    ip_addresses_module = imp.load_source('ip_addresses_module',
                                          ip_address_file_path)
    ip_address_list = ip_addresses_module.EmitIPAddressList()

    sudo("apt-get install -y iptables-persistent")
    sudo("iptables -P INPUT ACCEPT")
    sudo("iptables -P FORWARD ACCEPT")
    sudo("iptables -F")

    for ip_address in ip_address_list:
        sudo(
            "iptables -A INPUT -p tcp -s %s --dport 22 -j ACCEPT" % ip_address)

    sudo("iptables -P INPUT DROP")
    sudo("iptables -P FORWARD DROP")
    sudo("netfilter-persistent save")
    sudo("netfilter-persistent reload")
