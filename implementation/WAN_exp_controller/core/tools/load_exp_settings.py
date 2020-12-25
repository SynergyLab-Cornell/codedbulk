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
import os, json

def load_network_name():
    os.chdir(os.getcwd())

    network_name = ''

    try:
        with open('exp_settings.json') as fsettings:
            settings = json.load(fsettings)

            network_name = settings['network_name']
    except:
        print('exp_settings.json does not exist')

    return network_name

def load_network_name_and_total_nodes():
    os.chdir(os.getcwd())

    network_name = ''
    total_nodes = 0

    try:
        with open('exp_settings.json') as fsettings:
            settings = json.load(fsettings)

            network_name = settings['network_name']
            total_nodes = settings['total_nodes']
    except:
        print('exp_settings.json does not exist')

    return network_name, total_nodes

def load_server_ips():
    os.chdir(os.getcwd())

    server_ips = {}
    server_ips["127.0.0.1"] = []

    try:
        with open('server_ips.json','r') as fin:
            server_ips = json.load(fin)
    except:
        print('server_ips.json does not exist')

    return server_ips

def load_exp_settings():
    os.chdir(os.getcwd())

    network_name = ''
    total_nodes = 0
    start_from_exp = ''

    try:
        with open('exp_settings.json') as fsettings:
            settings = json.load(fsettings)

            network_name = settings['network_name']
            total_nodes = settings['total_nodes']
            start_from_exp = settings['start_from_exp']
    except:
        print('exp_settings.json does not exist')

    server_ips = load_server_ips()
    
    return network_name, total_nodes, start_from_exp, server_ips