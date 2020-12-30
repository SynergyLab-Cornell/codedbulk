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
from subprocess import Popen, PIPE, STDOUT
from load_exp_settings import load_server_ips
import os, json

server_ips = load_server_ips()

def download_results(exp_name):
    # server_ips
    os.system('mkdir -p results/raw/%s' % exp_name) 
    parallel = []   
    lack_of_traffic_summary = True
    for index, ip in enumerate(server_ips.keys()):
        if lack_of_traffic_summary:
            scp = Popen(['scp', '-i', 
                'keys/node_%d.pem' % index,
                ( '%s@%s:' % (server_account, ip) )+
                ( '~/apps/%s/traffic_summary.txt' % exp_name ),
                'results/raw/%s/' % exp_name
            ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
            parallel.append(scp)
            lack_of_traffic_summary = False
        os.system('mkdir -p results/raw/%s/v%d' % (exp_name,index))
        scp = Popen(['scp', '-i', 
            'keys/node_%d.pem' % index,
            ( '%s@%s:' % (server_account, ip) )+
            ( '~/apps/%s/results/*' % exp_name ),
            'results/raw/%s/v%d/' % (exp_name, index)
        ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        parallel.append(scp)
    for scp in parallel:
        #scp.wait()
        # wait and print
        return_code = None
        output = b''
        while (return_code is None) or (output != b''):
            return_code = scp.poll()
            output = scp.stdout.readline().strip()
            print(output.decode('utf-8'))
    print('finish download %s results' % exp_name)

if __name__ == '__main__':
    exp_name = input('exp_name: ')
    download_results(exp_name)
