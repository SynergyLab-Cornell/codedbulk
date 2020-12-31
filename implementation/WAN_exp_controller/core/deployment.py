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
from server_information import *
from load_exp_settings import load_network_name, load_server_ips 
from subprocess import Popen, PIPE, STDOUT

network_name = load_network_name()
server_ips = load_server_ips()

send_files = [
    'apps/shared/local_socket/',
    'controller/auto_gen.sh',
    'controller/build/',
    'controller/executable/',
    'controller/Makefile',
    'controller/settings/controller_setup.cc_part',
    'controller/settings/topology/topology_macros.h',
    'controller/settings/traffic/*',
    'controller/sources/*',
    'multicast_agent/build/',
    'multicast_agent/Makefile',
    'multicast_agent/sources/*',
    'proxy/build/',
    'proxy/Makefile',
    'proxy/sources/*',
    'shared/build/',
    'shared/Makefile',
    'shared/sources/*',
    'controller/main/traffic-gen-%s.cc' % network_name,
    'controller/settings/topology/%s.cc_part' % network_name,
    'controller/workloads/%s-*' % network_name,
    'controller/%s_auto_gen.sh' % network_name,
]

def deploy():
    for line in send_files:
        print(line)

    '''
    parallel = []
    for index, ip in enumerate(server_ips.keys()):
        scp = Popen(['ssh', '-T', '-i', 
            'keys/node_%d.pem' % index,
            '%s@%s' % (server_account, ip)
        ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        print('server %d connected' % index)
        parallel.append(scp)
    for scp in parallel:
        scp.wait()
    '''

if __name__ == '__main__':
    deploy()
