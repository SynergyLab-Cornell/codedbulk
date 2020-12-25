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
from load_exp_settings import load_server_ips
import os, json

server_ips = load_server_ips()

def connect_server(server_number):
    for index, ip in enumerate(server_ips):
        if index == server_number:
            command = 'ssh -i keys/aws_%d.pem %s@%s' % (server_number, server_account, ip)
            print(command)
            os.system(command)
            break

if __name__ == '__main__':
    server_number = int(input('server number: '))
    connect_server(server_number)
