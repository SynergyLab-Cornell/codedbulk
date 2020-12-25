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
from common_settings import *
import os, glob

def read_stdout(process):
    ''' usage:
        return_code = None
        output = b''
        while (return_code is None) or (output != b''):
            return_code, output = read_stdout(proxy)
            print ('%s' % output)
        print('return_code = %d' % return_code)
    '''
    return process.poll(), process.stdout.readline().strip()

def wait_and_print(process):
    return_code = None
    output = b''
    last_error_msg = ''
    while (return_code is None) or (output != b''):
        return_code = process.poll()
        output = process.stdout.readline().strip()
        if output != b'':
            error_msg = output.decode('utf-8')
            if error_msg != 'terminate called without an active exception':
                # last meaningful error message
                if 'what():' in error_msg:
                    last_error_msg += error_msg
                else:
                    last_error_msg = error_msg

            # ignore empty lines
            print ('%s' % error_msg)
            # the programs should not print out anything if no error
            return_code = -1
    return return_code, last_error_msg

def tc_run(arg):
    arg='tc '+arg
    print(arg)
    os.system(arg)

def tc_commands(index,ip,bandwidth):
    handle = index+1
    subhandle = index+6
    tc_run('qdisc del dev eth%d root 2> /dev/null' % index)
    tc_run('qdisc add dev eth%d root handle %d: htb default 300' % (index,handle))
    tc_run('class add dev eth%d parent %d: classid %d:1 htb rate %dmbit burst %dkbit' % (index, handle, handle, bandwidth, bandwidth))
    tc_run('qdisc add dev eth%d parent %d:1 handle %d: prio bands 4 priomap 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0' % (index, handle, subhandle))
    for i in range(1,4):
        tc_run('qdisc add dev eth%d parent %d:%d handle %d%d: pfifo' % (index, subhandle, i, subhandle, i))
    tc_run('filter add dev eth%d parent %d: protocol ip prio 2 u32 match ip src %s flowid %d:1' % (index, handle, ip, handle))
    for i in range(1,4):
        tc_run('filter add dev eth%d parent %d: protocol ip prio 3 u32 match ip dsfield 0x%d0 0xff flowid %d:%d' % (index, subhandle, (4-i)*2, subhandle, i))

def measure_latency_ip(ping):
    return_code = None
    last_msg = '' #'rtt min/avg/max/mdev = 999.999/999.999/999.999/999.999 ms'
    while (return_code is None) or ('rtt' not in output):
        return_code = ping.poll()
        output = ping.stdout.readline().strip().decode('utf-8')
        if 'rtt' in output:
            # ping outputs some empty lines
            # so we should not simply check output != ''
            last_msg = output
            break
    # e.g. last_msg = 
    #rtt min/avg/max/mdev = 1.714/1.752/1.792/0.026 ms
    #0       1   2   3            4     5     6
    items = last_msg.split('/')
    avg_latency = float(items[4])
    return avg_latency

def measure_latency(server_socket):
    print('measure latency')
    outgoing_ips = []
    command = server_socket.recv(2)
    while command != b'ed':
        outgoing_ip = server_socket.recv(20).decode('utf-8')
        print('to %s' % outgoing_ip)
        outgoing_ips.append(outgoing_ip)
        server_socket.send(b'r\n')
        command = server_socket.recv(2)

    pings = []
    for ip in outgoing_ips:
        ping = Popen(['ping', '-c', '10', '%s' % ip], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
        pings.append(ping)
    
    for ping in pings:
        latency = measure_latency_ip(ping)
        # reply latency back to the local controller
        server_socket.send(('%8.3f' % latency).encode('utf-8'))
        server_socket.recv(2)

def command_parsing(server_socket):
    running = True

    proxies = {}
    receivers = {}
    senders = {}

    in_exp_folder = False
    exp_name = None
    scenario = 0
    bandwidth = 200
    node_id = int(server_socket.recv(2))
    print('server id = %d' % node_id)
    outgoing_ips = []
    command = server_socket.recv(2)
    while command != b'ed':
        outgoing_ip = server_socket.recv(20).decode('utf-8')
        print('%s' % outgoing_ip)
        outgoing_ips.append(outgoing_ip)
        server_socket.send(b'r\n')
        command = server_socket.recv(2)
    server_socket.send(b'r\n')

    os.chdir('../apps')
    # create the symbolic link for easier downloading
    os.system('rm -f ~/apps')
    os.system('ln -rs . ~/apps')
    #print ('assign node_id %d\n' % node_id)
    while running:
        command = server_socket.recv(2)
        #print(command)
        if command == b'q\n':
            running = False
            break
        elif command == b'bw':
            bandwidth = int(server_socket.recv(6).decode('utf-8'))
            print('set bandwidth to %d' % bandwidth)
        elif command == b'tc':
            # do tc settings
            for index, ip in enumerate(outgoing_ips):
                tc_commands(index,ip,bandwidth)

        elif command == b'c\n':
            # clear sockets and results
            if in_exp_folder:
                print('clear local sockets and results')
                make_clears = Popen(['sudo', 'make', 'clears', 'NODE=%d' % node_id], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
                make_clears.wait()
                make_clears = Popen(['sudo', 'make', 'cleanresults', 'NODE=%d' % node_id], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
                make_clears.wait()
        elif command == b'cs':
            if in_exp_folder:
                print('clear local sockets')
                make_clears = Popen(['sudo', 'make', 'clears', 'NODE=%d' % node_id], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
                make_clears.wait()
        elif command == b'cr':
            if in_exp_folder:
                print('clear results')
                make_clears = Popen(['sudo', 'make', 'cleanresults', 'NODE=%d' % node_id], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
                make_clears.wait()
        elif command == b'e\n':
            #print('experiment setup')
            exp_name = server_socket.recv(40).decode('utf-8')
            #print(exp_name)
            try:
                if in_exp_folder:
                    os.chdir('../%s' % exp_name)
                else:
                    os.chdir('%s' % exp_name)
                    in_exp_folder = True

                # make the applications
                make_apps = Popen(['make', 'clean', 'NODE=%d' % node_id], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
                make_apps.wait()
                make_apps = Popen(['make', 'NODE=%d' % node_id], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
                make_apps.wait()

                # get the corresponding names 
                proxies = {}
                for file_name in glob.glob('exec-proxy'):
                    proxies[file_name] = None
                #for proxy in proxies.keys():
                #    print('find proxy %s' % proxy)

                receivers = {}
                for file_name in glob.glob('exec-traffic*recv'):
                    receivers[file_name] = None
                #for receiver in receivers.keys():
                #    print('find receiver %s' % receiver)

                senders = {}
                for file_name in glob.glob('exec-traffic*send'):
                    senders[file_name] = None
                #for sender in senders.keys():
                #    print('find sender %s' % sender)

            except:
                print('exp does not exist')
                server_socket.send(b'r\n')
                print('make fails')
                continue

            print('finish making %s' % exp_name)

        elif command == b'sc':
            scenario = int(server_socket.recv(2))
            print('set scenario %d' % int(scenario))

        elif command == b'p\n':
            # run proxies
            for proxy_name in proxies.keys():
                proxies[proxy_name] = Popen(['./%s' % proxy_name, '%d' % scenario], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
            print('proxies started')

        elif command == b'tp':
            # terminate the proxies
            for proxy in proxies.values():
                if proxy is not None:
                    if proxy.poll() is None:
                        # still running
                        proxy.stdin.write(b'q\n')
                        proxy.stdin.flush()
            for proxy_name in proxies.keys():
                #print('  stop %s' % proxy_name)
                if proxies[proxy_name] is not None:
                    return_code, last_error_msg = wait_and_print(proxies[proxy_name])
                    if return_code != 0:
                        server_socket.send(b'e\n')
                        server_socket.send(
                            ('v%d: proxy fails (%d)' % (node_id, return_code) + 
                            ('' if last_error_msg == '' else ', '+last_error_msg)
                        ).encode('utf-8'))
                        server_socket.recv(2)
                    proxies[proxy_name] = None
            os.system('rm -rf tmp_store_and_forward')
            print('proxies stopped')

        elif command == b'r\n':
            # run receivers
            for receiver_name in receivers.keys():
                receivers[receiver_name] = Popen(['./%s' % receiver_name, '%d' % scenario], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
            print('receivers started')

        elif command == b'tr':
            # terminate the receivers
            for receiver in receivers.values():
                if receiver is not None:
                    if receiver.poll() is None:
                        # still running
                        receiver.stdin.write(b'q\n')
                        receiver.stdin.flush()
            for receiver_name in receivers.keys():
                #print('  stop %s' % receiver_name)
                if receivers[receiver_name] is not None:
                    return_code, last_error_msg = wait_and_print(receivers[receiver_name])
                    if return_code != 0:
                        server_socket.send(b'e\n')
                        server_socket.send(
                            ('v%d: %s fails (%d)' % (node_id, receiver_name, return_code) + 
                            ('' if last_error_msg == '' else ', '+last_error_msg)
                        ).encode('utf-8'))
                        server_socket.recv(2)
                    receivers[receiver_name] = None
            print('receivers stopped')

        elif command == b's\n':
            # run senders
            for sender_name in senders.keys():
                senders[sender_name] = Popen(['./%s' % sender_name, '%d' % scenario], stdout=PIPE, stdin=PIPE, stderr=STDOUT)
            print('senders started')

        elif command == b'ts':
            # terminate the senders
            for sender in senders.values():
                if sender is not None:
                    if sender.poll() is None:
                        # still running
                        sender.stdin.write(b'q\n')
                        sender.stdin.flush()
            for sender_name in senders.keys():
                #print('  stop %s' % sender_name)
                if senders[sender_name] is not None:
                    return_code, last_error_msg = wait_and_print(senders[sender_name])
                    if return_code != 0:
                        server_socket.send(b'e\n')
                        server_socket.send(
                            ('v%d: %s fails (%d)' % (node_id, sender_name, return_code) +
                            ('' if last_error_msg == '' else ', '+last_error_msg)
                        ).encode('utf-8'))
                        server_socket.recv(2)
                    senders[sender_name] = None
            print('senders stopped')

        elif command == b'ml':
            # measure latency to the neighboring nodes
            measure_latency(server_socket)

        server_socket.send(b'r\n')

def remote_server():
    print('server starts')

    # the server side controller that launches applications
    listening_socket = socket(AF_INET, SOCK_STREAM)
    # listen to all interfaces on connection_port
    listening_socket.bind(('',connection_port))
    listening_socket.listen(1)

    server_socket, local_controller_address = listening_socket.accept()

    print('connected to controller')

    command_parsing(server_socket)

    server_socket.close()
    print('server ends')

if __name__ == '__main__':
    remote_server()
