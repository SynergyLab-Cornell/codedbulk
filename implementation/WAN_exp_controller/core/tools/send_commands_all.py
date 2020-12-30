#!/usr/bin/env python
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Author:  Shih-Hao Tseng (shtseng@caltech.edu)
#
from subprocess import Popen, PIPE, STDOUT
from server_information import *
from load_exp_settings import load_network_name, load_server_ips 
import os, json

network_name = load_network_name()
server_ips = load_server_ips()

commands = [
# setup commands
#'sudo yum install git gcc-c++ tc gdb -y\n',
#'echo \"net.ipv4.tcp_fin_timeout=10\" | sudo tee -a /etc/sysctl.conf\n',
#'echo \"net.ipv4.tcp_tw_reuse=1\" | sudo tee -a /etc/sysctl.conf\n',
#'sudo sysctl -p\n',
#'ln -rs nc_%s/apps .\n' % network_name.lower(),
'rm -rf nc_%s/.git*\n' % network_name.lower(),
# recompile
'cd nc_%s/shared\n' % network_name.lower(),
'make clean\n',
'cd ../proxy\n',
'make clean\n',
'cd ../multicast_agent\n',
'make clean\n',
#'cd ../controller\n',
#'rm -rf workloads/workloads\n',
#'make clean\n',
#'make -j8\n',
#'./%s_auto_gen.sh\n' % network_name,
'exit\n'
]

def wait_and_print(process):
    return_code = None
    output = b''
    while (return_code is None) or (output != b''):
        return_code = process.poll()
        output = process.stdout.readline().strip()
        print(output.decode('utf-8'))

def send_commands_all():
    parallel = []
    for index, ip in enumerate(server_ips.keys()):
        ssh = Popen(['ssh', '-T', '-i', 
            'keys/aws_%d.pem' % index,
            '%s@%s' % (server_account, ip)
        ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        print('server %d connected' % index)
        for command in commands:
            ssh.stdin.write(command.encode('utf-8'))
            ssh.stdin.flush()
        #ssh.communicate(input=command.encode('utf-8'))
        parallel.append(ssh)
    for ssh in parallel:
        #ssh.wait()
        wait_and_print(ssh)

if __name__ == '__main__':
    send_commands_all()
