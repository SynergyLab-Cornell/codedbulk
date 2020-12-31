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
import os,json,csv,shutil

def ip_list_generator(network_name):
    server_public_ips = {}
    server_private_ips = None

    server_ips = {}

    try:
        with open('settings/%s/public_ips.csv' % network_name,'r') as fpub:
            csv_reader = csv.reader(fpub, delimiter=',')
            line_count = 0
            for row in csv_reader:
                if line_count == 0:
                    line_count += 1
                    continue

                server_public_ips[row[0]] = []
                for ip in row[1:]:
                    if ip != '':
                        server_public_ips[row[0]].append(ip)
                
                line_count += 1
    except:
        print('error: settings/%s/public_ips.csv does not exist' % network_name)
        return

    try:
        server_private_ips = {}
        with open('settings/%s/private_ips.csv' % network_name,'r') as fpriv:
            csv_reader = csv.reader(fpriv, delimiter=',')
            line_count = 0
            for row in csv_reader:
                if line_count == 0:
                    line_count += 1
                    continue
                
                server_private_ips[row[0]] = []
                for ip in row[1:]:
                    if ip != '':
                        server_private_ips[row[0]].append(ip)
                
                line_count += 1
        
        # check if private ips match the public ips structure
        if len(server_public_ips) != len(server_private_ips):
            print('error: public_ips does not match private_ips')
            return

            for key in server_public_ips.keys():
                if len(server_public_ips[key]) != len(server_private_ips[key]):
                    print('error: public_ips[%d] does not match private_ips[%d]' % (key,key))
                    return
    except:
        server_private_ips = None
    
    # generate the topology.cc_part
    with open('tmp/%s.cc_part' % network_name,'w') as fcc:
        fcc.write('/* Author:  Shih-Hao Tseng (shtseng@caltech.edu) */\n')
        fcc.write('#ifdef __TOPOLOGY_MACROS__\n\n')
        
        total_nodes = len(server_public_ips.keys())
        fcc.write('#define TOTAL_SWITCHES %d\n' % total_nodes)
        fcc.write('#define TOTAL_HOSTS %d\n\n' % total_nodes)

        try:
            # include memo if there is any
            with open('settings/%s/memo' % network_name,'r') as fmemo:
                for line in fmemo:
                    fcc.write('// %s' % line)
            fcc.write('\n')
        except:
            pass

        fcc.write('#define GENERATE_TOPOLOGY()\\\n')
        fcc.write('graph->addNodes(TOTAL_SWITCHES);\\\n')
        fcc.write('\\\n')
        for key in server_public_ips.keys():
            fcc.write('LIST_OF_PUBLIC_ADDRESSES(%s,' % key)
            first = True
            for ip in server_public_ips[key]:
                if first:
                    first = False
                else:
                    fcc.write(' COMMA')
                fcc.write(' \"%s\"' % ip)
            fcc.write(' )\\\n')
        fcc.write('\\\n')
        if server_private_ips is None:
            for key in server_public_ips.keys():
                fcc.write('PRIVATE_ADDRESSES_IS_THE_SAME_AS_PUBLIC(%s)\\\n' % key)
            server_private_ips = server_public_ips
        else:
            for key in server_private_ips.keys():
                fcc.write('LIST_OF_PRIVATE_ADDRESSES(%s,' % key)
                first = True
                for ip in server_private_ips[key]:
                    if first:
                        first = False
                    else:
                        fcc.write(' COMMA')
                    fcc.write(' \"%s\"' % ip)
                fcc.write(' )\\\n')
        fcc.write('\\\n')
        for key in server_public_ips.keys():
            fcc.write('AUTO_REG_SW_ADDR(%s)\\\n' % key)

        public_ip_counter = {}
        # each node -> remote public ips
        remote_ips = {}
        for key in server_public_ips.keys():
            public_ip_counter[key] = 0
            remote_ips[key] = {}

        try:
            # links
            fcc.write('\\\n')
            fcc.write('/* link the switches */\\\n')
            fcc.write('/* private to public */\\\n')

            with open('settings/%s/links.csv' % network_name,'r') as flink:
                csv_reader = csv.reader(flink, delimiter=',')
                line_count = 0
                head = []
                for row in csv_reader:
                    if line_count == 0:
                        # first line
                        head = row[1:]
                    else:
                        line = row[1:]
                        len_line = len(line)
                        src_node = row[0]
                        for dst_node_index in range(len_line):
                            rate = line[dst_node_index]
                            if rate != '':
                                dst_node = head[dst_node_index]
                                fcc.write('AUTO_SW_TO_SW_IP(%s,%s,%s)\\\n' % (src_node,dst_node,rate))

                                remote_ips[src_node][dst_node] = server_public_ips[dst_node][public_ip_counter[dst_node]]
                                public_ip_counter[dst_node] += 1
                                remote_ips[dst_node][src_node] = server_public_ips[src_node][public_ip_counter[src_node]]
                                public_ip_counter[src_node] += 1

                    line_count += 1
        except:
            print('warning: no link data')
            pass

        fcc.write('\n#endif // __TOPOLOGY_MACROS__')

    # generate the server_ips
    for key in server_public_ips.keys():
        pub_ip = server_public_ips[key][0]
        server_ips[pub_ip] = server_private_ips[key]

    with open('tmp/server_ips.json','w') as fips:
        json.dump(server_ips,fips)

    # save the remote_ips
    with open('tmp/%s_remote_ips.json' % network_name,'w') as fips:
        json.dump(remote_ips,fips)

    return len(server_ips)

def generate_exp(network_name):
    network_name = network_name.lower()
    total_nodes = 0
    load = '0.1'

    print('Generate codes for experiment \'%s\'' % network_name)
    # generate topology files
    total_nodes = ip_list_generator(network_name)

    # generate codes
    with open('tmp/%s_auto_gen.sh' % network_name,'w') as fout:
        fout.write('#!/bin/bash\n./auto_gen.sh %s %d' % (network_name,total_nodes))
    with open('tmp/traffic-gen-%s.cc' % network_name,'w') as fout:
        fout.write("""/***********
* Generate %s
***********/
#define TOPOLOGY_NAME %s
#define WORKLOAD_PREFIX %s
#include "traffic-gen-mixed.cc_part" """ % (network_name, network_name, network_name))

    print('Generating WAN controller for experiment \'%s\' under WAN_exp_controller/exp_%s' % (network_name, network_name))
    # create root folder
    # delete the existing one
    shutil.rmtree('WAN_exp_controller/exp_%s' % network_name, ignore_errors=True)
    os.mkdir('WAN_exp_controller/exp_%s' % network_name)
    os.chdir('WAN_exp_controller/exp_%s' % network_name)

    # create subfolders
    os.mkdir('error_logs')
    os.mkdir('keys')
    os.mkdir('results')

    os.mkdir('codes')
    shutil.move('../../tmp/%s.cc_part' % network_name, 'codes/%s.cc_part' % network_name)
    shutil.move('../../tmp/%s_auto_gen.sh' % network_name, 'codes/%s_auto_gen.sh' % network_name)
    shutil.move('../../tmp/traffic-gen-%s.cc' % network_name, 'codes/traffic-gen-%s.cc' % network_name)

    # create symbolic links
    os.symlink('../core/common_settings.py','common_settings.py')
    os.symlink('../core/deployment.py','deployment.py')
    os.symlink('../core/load_exp_settings.py','load_exp_settings.py')
    os.symlink('../core/local_controller.py','local_controller.py')
    os.symlink('../core/server_information.py','server_information.py')
    os.symlink('../core/tools','tools')

    # create files
    with open('exp_settings.json','w') as fout:
        fout.write('{ "network_name": "%s", "total_nodes": %d, "start_from_exp": "%s-1-3-interactive-%s-0" }' 
        % (network_name, total_nodes, network_name, load))
    
    # move settings
    shutil.move('../../tmp/%s_remote_ips.json' % network_name, '%s_remote_ips.json' % network_name)
    shutil.move('../../tmp/server_ips.json', 'server_ips.json')

if __name__ == '__main__':
    network_name = input('Enter the network name of the experiment: ')
    if network_name == '':
        network_name = 'dummy'
    generate_exp(network_name)
