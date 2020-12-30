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

scripts = [
'WAN_exp_controller/remote_server.py'
#'controller/settings/topology/%s.cc_part' % network_name,
#'controller/workloads/%s-200Mbps-0.05.txt' % network_name,
#'controller/workloads/%s-200Mbps-0.1.txt' % network_name,
#'controller/workloads/%s-200Mbps-0.15.txt' % network_name,
#'controller/workloads/%s-200Mbps-0.2.txt % network_name'
#'shared/sources/system_parameters.h'
#'proxy/sources/NC-codec.h'
]

def wait_and_print(process):
    return_code = None
    output = b''
    while (return_code is None) or (output != b''):
        return_code = process.poll()
        output = process.stdout.readline().strip()
        print(output.decode('utf-8'))

def update_scripts():
    # read server_ips from file
    global server_ips
    try:
        with open('server_ips.json','r') as fin:
            server_ips = json.load(fin)
    except:
        print('server_ips.json does not exist')

    parallel = []
    for index, ip in enumerate(server_ips.keys()):
        for script in scripts:
            local_folder = 'codedbulk_%s' % network_name
            if os.path.exists('deployment/core/%s' % script):
                local_folder = 'core'
            scp = Popen(['scp', '-i', 
                'keys/node_%d.pem' % index, '-r',
                'deployment/%s/%s' % (local_folder.lower(), script),
                ( '%s@%s:' % (server_account, ip) )+
                ( '~/codedbulk_%s/%s' % (network_name.lower(), script) )
            ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
            parallel.append(scp)
    for scp in parallel:
        scp.wait()
        wait_and_print(scp)

if __name__ == '__main__':
    os.chdir(os.getcwd())
    os.system('cd deployment/codedbulk_%s' % network_name.lower())
    os.system('git pull')
    os.system('cd -')
    update_scripts()