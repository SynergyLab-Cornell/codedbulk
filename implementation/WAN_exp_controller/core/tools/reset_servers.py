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
'sudo pkill screen\n',
'sudo pkill -f exec-\n',
'sudo pkill -f exec-\n',
'sudo pkill -f exec-\n',
'cd ~/codedbulk_%s/WAN_exp_controller\n' % network_name.lower(),
'screen -d -m sudo python remote_server.py\n',
'exit\n'
]

def wait_and_print(process):
    return_code = None
    output = b''
    while (return_code is None) or (output != b''):
        return_code = process.poll()
        output = process.stdout.readline().strip()
        print(output.decode('utf-8'))

def reset_servers():
    # read server_ips from file
    global server_ips
    try:
        with open('server_ips.json','r') as fin:
            server_ips = json.load(fin)
    except:
        print('server_ips.json does not exist')

    parallel = []
    for index, ip in enumerate(server_ips.keys()):
        ssh = Popen(['ssh', '-T', '-i', 
            'keys/node_%d.pem' % index,
            '%s@%s' % (server_account, ip)
        ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        print('server %d connected' % index)
        for command in commands:
            ssh.stdin.write(command.encode('utf-8'))
            ssh.stdin.flush()
        #ssh.communicate(input=command.encode('utf-8'))
        parallel.append(ssh)
    for ssh in parallel:
        ssh.wait()
        #wait_and_print(ssh)

if __name__ == '__main__':
    reset_servers()