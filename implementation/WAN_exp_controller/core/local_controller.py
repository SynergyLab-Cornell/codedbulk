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
from socket import *
from subprocess import Popen, PIPE, STDOUT
from server_information import *
from tools.reset_servers import reset_servers
from tools.load_exp_settings import *
from common_settings import *
from time import sleep
from datetime import datetime
import os, json #pickle

network_name, total_nodes, start_from_exp, server_ips = load_exp_settings()

error_reasons={}
current_experiment=""
current_scenario=""

test_proxy=-1

def wait_for_ready(client_sockets, no_wait=False):
    for index, client_socket in enumerate(client_sockets):
        if no_wait and (index == test_proxy):
            continue
        reply = client_socket.recv(2)
        while reply == b'e\n':
            # handle error message
            if current_experiment not in error_reasons.keys():
                error_reasons[current_experiment] = []
            reason = client_socket.recv(200).decode('utf-8')
            print('  [error] %s' % reason)
            error_reasons[current_experiment].append("["+str(current_scenario)+"] "+reason)
            client_socket.send(b'ok')
            reply = client_socket.recv(2)
    #print('command OK\n')

def measure_latency(client_sockets):
    remote_ips = {}
    with open('%s_remote_ips.json' % network_name, 'r') as fips:
        remote_ips = json.load(fips)

    # specify the ips to measure
    latencies = {}
    for src_node in remote_ips.keys():
        latencies[src_node] = {}
        index = int(src_node)
        client_socket = client_sockets[index]
        for dst_node in remote_ips[src_node].keys():
            latencies[src_node][dst_node] = 0
            remote_ip = remote_ips[src_node][dst_node]
            client_socket.send(b'nw')
            client_socket.send(remote_ip.encode('utf-8'))
            client_socket.recv(2)
        client_socket.send(b'ed')

    # receive the measurement results
    for src_node in remote_ips.keys():
        index = int(src_node)
        print('receive measurements from node %d:' % index)
        client_socket = client_sockets[index]
        for dst_node in remote_ips[src_node].keys():
            msg = client_socket.recv(8).decode('utf-8').strip()
            print(msg)
            latency = float(msg)
            client_socket.send(b'ok')
            latencies[src_node][dst_node] = latency

    with open('results/%s-latency.csv' % network_name, 'w') as flatency:
        flatency.write('node')
        for key in remote_ips.keys():
            flatency.write(',%s' % key)
        for src_node in remote_ips.keys():
            flatency.write('\n%s' % src_node)
            for dst_node in remote_ips.keys():
                flatency.write(',')
                if dst_node in latencies[src_node].keys():
                    flatency.write('%.3f' % latencies[src_node][dst_node])

def user_interaction(client_sockets):
    running = True
    while running:
        command = input('input the command, q to quit the user interaction mode\n')
        if command == 'q':
            running = False
            break

        if len(command) == 1:
            command += '\n'
        

        print(command.encode('utf-8'))
        for client_socket in client_sockets:
            client_socket.send(command.encode('utf-8'))
            #client_socket.flush()

        if command == 'bw':
            # set the bandwidth
            global bandwidth
            bandwidth_str = input('bandwidth = ?')
            bandwidth = int(bandwidth_str)
            for client_socket in client_sockets:
                client_socket.send(bandwidth_str.encode('utf-8'))
        if command == 'tc':
            # do tc settings
            pass
        elif command == 'c\n':
            # clear sockets and results
            pass
        elif command == 'e\n':
            # need to wait until make finishes
            command = input('exp name\n')
            for client_socket in client_sockets:
                client_socket.send(command.encode('utf-8'))
        elif command == 'sc':
            # run proxy
            command = input('scenario\n')
            for client_socket in client_sockets:
                client_socket.send(command.encode('utf-8'))
        elif command == 'p\n':
            # run proxy
            pass
        elif command == 'tp':
            # terminate proxy
            pass
        elif command == 'r\n':
            # run receivers
            pass
        elif command == 'tr':
            # terminate receivers
            pass
        elif command == 's\n':
            # run senders
            pass
        elif command == 'ts':
            # terminate senders
            pass
        elif command == 'ml':
            # measure latency
            measure_latency(client_sockets)

        wait_for_ready(client_sockets)

def get_all_WAN_experiments():
    ret = []
    '''
    name_base = network_name + "_WAN-"
    ret.append(name_base+"6-4-0")
    loads = [0.05,0.1,0.15,0.2]
    for load in loads:
        ret.append(name_base+"interactive-"+str(load))
    '''
    for case in range(total_cases):
        for load in loads:
            ret.append(network_name+"-"+str(target_src)+"-"+str(target_dst)+"-interactive-"+str(load)+"-"+str(case))
        for dst in range(2,total_nodes):
            if dst == target_dst:
                continue
            ret.append(network_name+"-"+str(target_src)+"-"+str(dst)+"-interactive-0.1-"+str(case))
        for src in range(1,total_nodes+1):
            if src == target_src:
                continue
            ret.append(network_name+"-"+str(src)+"-"+str(target_dst)+"-interactive-0.1-"+str(case))
    return ret
    
def get_all_bulk_experiments():
    ret = []
    total_cases=10
    for dst in range(2,total_nodes):
        for case in range(total_cases):
            ret.append(network_name+"-"+str(target_src)+"-"+str(dst)+"-"+str(case))
    for src in range(1,total_nodes+1):
        if src == target_src:
            continue
        for case in range(total_cases):
            ret.append(network_name+"-"+str(src)+"-"+str(target_dst)+"-"+str(case))
    return ret

def download_results(exp_name):
    # server_ips
    os.system('mkdir -p results/raw/%s' % exp_name) 
    parallel = []   
    lack_of_traffic_summary = True
    for index, ip in enumerate(server_ips.keys()):
        if lack_of_traffic_summary:
            scp = Popen(['scp', '-i', 
                'keys/aws_%d.pem' % index,
                ( '%s@%s:' % (server_account, ip) )+
                ( '~/apps/%s/traffic_summary.txt' % exp_name ),
                'results/raw/%s/' % exp_name
            ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
            parallel.append(scp)
            lack_of_traffic_summary = False
        os.system('mkdir -p results/raw/%s/v%d' % (exp_name,index))
        scp = Popen(['scp', '-i', 
            'keys/aws_%d.pem' % index,
            ( '%s@%s:' % (server_account, ip) )+
            ( '~/apps/%s/results/*' % exp_name ),
            'results/raw/%s/v%d/' % (exp_name, index)
        ], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        parallel.append(scp)
    for scp in parallel:
        scp.wait()
        # wait and print
        #return_code = None
        #output = b''
        #while (return_code is None) or (output != b''):
        #    return_code = scp.poll()
        #    output = scp.stdout.readline().strip()
        #    print(output.decode('utf-8'))
    print('finish download %s results' % exp_name)

def experiment(client_sockets, exp_name, specified_scenarios=None):
    print('run experiment %s' % exp_name)
    global current_experiment
    current_experiment=exp_name
    for client_socket in client_sockets:
        client_socket.send(b'e\n')
        client_socket.send(exp_name.encode('utf-8'))
    wait_for_ready(client_sockets)

    for client_socket in client_sockets:
        client_socket.send(b'cr')
    wait_for_ready(client_sockets)

    if specified_scenarios is None:
        specified_scenarios = scenarios.keys()

    global current_scenario
    for scenario in specified_scenarios:
        # reset the tc
        for client_socket in client_sockets:
            client_socket.send(b'tc')
        wait_for_ready(client_sockets)

        # reset local sockets
        print('clear local sockets')
        for client_socket in client_sockets:
            client_socket.send(b'cs')
        wait_for_ready(client_sockets)

        current_scenario = scenario
        print('test scenario %d' % scenario)
        for client_socket in client_sockets:
            client_socket.send(b'sc')
            client_socket.send(('%02d' % scenario).encode('utf-8'))
        wait_for_ready(client_sockets)

        print('start receivers')
        for client_socket in client_sockets:
            client_socket.send(b'r\n')
        wait_for_ready(client_sockets)

        sleep(3)

        print('start proxies')
        for index, client_socket in enumerate(client_sockets):
            if index == test_proxy:
                continue
            client_socket.send(b'p\n')
        wait_for_ready(client_sockets,True)

        sleep(3)

        print('start senders')
        for client_socket in client_sockets:
            client_socket.send(b's\n')
        wait_for_ready(client_sockets)

        # do experiments
        sleep(exp_duration_seconds)

        print('stop senders')
        for client_socket in client_sockets:
            client_socket.send(b'ts')
        wait_for_ready(client_sockets)

        sleep(3)

        print('stop proxies')
        for client_socket in client_sockets:
            client_socket.send(b'tp')
        wait_for_ready(client_sockets,True)

        sleep(3)

        print('stop receivers')
        for client_socket in client_sockets:
            client_socket.send(b'tr')
        wait_for_ready(client_sockets)

        sleep(tcp_wait_timeout)

    download_results(exp_name)

def print_help_message():
    print('specify the function:')
    print('rb: run bulk experiment')
    print('rw: run WAN experiment')
    print('rt: run test (B4-6-3-interactive-0.1-0)')
    print('rs: specific test')
    print('rf: rerun failed experiments')
    print('rr: rerun file specified experiments')
    print('lf: load failed experiments')
    print('pf: print failed experiments')
    print('ce: change experiment duration')
    print('h: print help message')
    print('u: user interaction mode')
    print('q: quit')
    print('target bandwidth = %d' % bandwidth)

def list_error_messages():
    if bool(error_reasons):
        for exp_name in error_reasons.keys():
            print('%s:' % exp_name)
            for reason in error_reasons[exp_name]:
                print('  - %s' % reason)

def log_error_messages():
    datatime_now = datetime.now()
    error_log_name = 'error_logs/log_error_reasons_%02d%02d%02d.json' % (datatime_now.month, datatime_now.day, datatime_now.hour)
    
    try:
        with open(error_log_name,'r') as fin:
            # include the existing records
            error_reasons.update(json.load(fin))
    except:
        # no such file.
        pass

    try:
        with open(error_log_name,'w') as fout:
            json.dump(error_reasons,fout)
        #with open('log_error_reasons.pkl','wb') as fout:
        #    pickle.dump(error_reasons,fout)
    except:
        pass

def main_menu (client_sockets):
    running = True
    all_experiments = []
    global error_reasons
    global exp_duration_seconds
    print_help_message()
    while running:
        command = input()
        if len(command) < 1:
            # empty command
            continue
        if command[0] == 'r':
            if command[1] == 'b':
                all_experiments = get_all_bulk_experiments()
            elif command[1] == 'w':
                all_experiments = get_all_WAN_experiments()
            elif command[1] == 't':
                all_experiments=['B4-6-3-interactive-0.1-0']
            elif command[1] == 's':
                exp_name = input('enter the exp_name: ')
                all_experiments=[exp_name]
            elif command[1] == 'f':
                all_experiments = []
                for exp_name in error_reasons.keys():
                    all_experiments.append(exp_name)
                error_reasons = {}        
            
            if command[1] == 'r':
                all_experiments = []
                
                rerun_experiments = {}
                try:
                    with open('rerun.json','r') as frerun:
                        rerun_experiments = json.load(frerun)
                    
                    for exp_name in rerun_experiments.keys():
                        #print('rerun %s ' % exp_name)
                        #print(rerun_experiments[exp_name])
                        experiment(client_sockets, exp_name, rerun_experiments[exp_name])
                        # also log_error_messages
                        log_error_messages()
                except:
                    print('rerun.json is missing')
                    continue

            else:
                start_from = False
                if start_from_exp != '':
                    start_from = True
                for exp_name in all_experiments:
                    if start_from:
                        if exp_name != start_from_exp:
                            print('skip %s' % exp_name)
                            continue
                        else:
                            start_from = False
                    experiment(client_sockets, exp_name)

                    # also log_error_messages
                    log_error_messages()

            list_error_messages()
        elif command == 'pf':
            list_error_messages()
        elif command == 'lf':
            try:
                with open('error_logs/log_error_reasons.json','r') as fin:
                    error_reasons = json.load(fin)
                #with open('log_error_reasons.pkl','rb') as fin:
                #    error_reasons = pickle.load(fin)
                print('load successfully')
            except:
                print('load fails: error_logs/log_error_reasons.json does not exist -> remove the timestamp to select')
                #print('load fails: log_error_reasons.pkl does not exist')

        elif command == 'h':
            print_help_message()
        elif command == 'u':
            user_interaction(client_sockets)
        elif command == 'ce':
            # change experiment duration
            time = input('set the duration (seconds): ')
            time = int(time)
            if time > 0:
                print('change duration to %d' % time)
                exp_duration_seconds = time

        elif command == 'q':
            running = False
    list_error_messages()

def server_ips_settings(client_sockets):
    for index, ip in enumerate(server_ips.keys()):
        print('connecting server %d at %s' % (index, ip))
        client_socket = socket(AF_INET, SOCK_STREAM)
        client_socket.connect((ip,connection_port))

        client_socket.send(('%02d' % index).encode('utf-8'))

        # for tc settings
        for outgoing_ip in server_ips[ip]:
            client_socket.send(b'nw')
            client_socket.send(outgoing_ip.encode('utf-8'))
            client_socket.recv(2)
        client_socket.send(b'ed')
        
        client_sockets.append(client_socket)

        print('server %d at %s is connected' % (index, ip))
    
    for client_socket in client_sockets:
        client_socket.recv(2)
        client_socket.send(b'bw')
        client_socket.send(str(bandwidth).encode('utf-8'))
    for client_socket in client_sockets:
        client_socket.recv(2)

    print('finish server settings')

def local_controller():
    # the controller that controls multiple remote servers
    client_sockets = []

    reset_servers()
    os.system('mkdir -p error_logs')

    server_ips_settings(client_sockets)

    main_menu(client_sockets)

    for client_socket in client_sockets:
        # quit
        client_socket.send(b'q\n')
        client_socket.close()

if __name__ == '__main__':
    #main_menu(None)
    local_controller()
