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
import os, math, json, sys
from common_settings import *
from load_exp_settings import load_network_name_and_total_nodes

network_name, total_nodes = load_network_name_and_total_nodes()

suggested_rerun_cases_and_reasons = {}

def parse_exp_name(exp_name):
    segments = exp_name.split('-')
    # network_name = segments[0] 
    src = int(segments[1])
    dst = int(segments[2])
    # 'interactive': segment[3]
    load = segments[4]
    return src, dst, load

def parse_traffic(folder_name):
    # return total_bulk_Mbps, total_interactive_Mbps, valid (Boolean)
    try:
        with open(folder_name+'traffic_summary.txt','r') as fsum:
            # parse the summary to understand the results

            total_bulk_Mbps = {}
            total_interactive_Mbps = {}
            for scenario in scenarios.keys():
                total_bulk_Mbps[scenario] = 0.0
                total_interactive_Mbps[scenario] = 0.0

            for line in fsum:
                info = line.split()

                start_signal = info[0]

                if start_signal == '-1':
                    is_bulk = True
                    filename_prefix = ''
                elif start_signal == '-3':
                    is_bulk = False
                    filename_prefix = 'interactive_'
                else:
                    continue

                info.pop(-1)

                traffic_id = int(info[1])
                src_id = int(info[2])
                total_dst = int(info[3])
                pos = 4
                dst_counter = 0

                bulk_Mbps = {}
                interactive_Mbps = {}
                for scenario in scenarios.keys():
                    bulk_Mbps[scenario] = 0.0
                    interactive_Mbps[scenario] = 0.0

                #print('src %d = %d' % (src_id,src_id))
                while dst_counter < total_dst:
                    recv_id = int(info[pos])
                    dst_id = int(info[pos+1])
                    #print('  dst %d: %d' % (recv_id, dst_id))

                    for scenario in scenarios.keys():
                        recv_total_Bps = 0.0
                        with open((folder_name+
                            ('v%d/' % dst_id)+
                            ('%straffic%d_recv%d_throughput_%s.dat' % (filename_prefix,traffic_id,recv_id,scenarios[scenario]))
                        ), 'r') as fdata:
                            for data in fdata:
                                data = data.split()
                                #hour = data[0]
                                #min = data[1]
                                #sec = data[2]
                                #Bps = data[3]
                                recv_total_Bps += float(data[3])
                            recv_Mbps = recv_total_Bps/exp_duration_seconds * 8.0 / 1000000

                            if is_bulk:
                                bulk_Mbps[scenario] += recv_Mbps
                            else:
                                interactive_Mbps[scenario] += recv_Mbps

                    pos += 2
                    dst_counter += 1

                for scenario in scenarios.keys():
                    if is_bulk:
                        bulk_Mbps[scenario] /= total_dst
                        total_bulk_Mbps[scenario] += bulk_Mbps[scenario]
                    else:
                        interactive_Mbps[scenario] /= total_dst
                        total_interactive_Mbps[scenario] += interactive_Mbps[scenario]
                # now we have total_bulk_Mbps and total_interactive_Mbps for this exp

            return total_bulk_Mbps, total_interactive_Mbps, True
        return None, None, False
    except:
        # no file
        return None, None, False

def conclude(exp_name,fout=None,fout_raw=None,suggested_rerun=None):
    # here exp_name without individual scenario
    print('deal with exp name %s' % exp_name)
    # parse the exp_name:
    src, dst, load = parse_exp_name(exp_name)

    total_mean = {}
    total_std_dev = {}
    bulk_mean = {}
    bulk_std_dev = {}
    interactive_mean = {}
    interactive_std_dev = {}

    for scenario in scenarios.keys():
        total_mean[scenario] = 0.0
        total_std_dev[scenario] = 0.0
        bulk_mean[scenario] = 0.0
        bulk_std_dev[scenario] = 0.0
        interactive_mean[scenario] = 0.0
        interactive_std_dev[scenario] = 0.0
    
    is_bulk = False
    filename_prefix = ''

    tota_valid_case = 0
    for case in range(total_cases):
        exp_name_case = '%s-%d' % (exp_name,case)
        folder_name = 'results/raw/%s/' % (exp_name_case)
        # get the traffic_summary
        total_bulk_Mbps, total_interactive_Mbps, valid = parse_traffic(folder_name)
        if valid:
            # calculate mean and std_dev
            for scenario in scenarios.keys():
                total_Mbps = total_bulk_Mbps[scenario] + total_interactive_Mbps[scenario]

                total_mean[scenario] += total_Mbps
                total_std_dev[scenario] += total_Mbps*total_Mbps

                bulk_mean[scenario] += total_bulk_Mbps[scenario]
                bulk_std_dev[scenario] += total_bulk_Mbps[scenario]*total_bulk_Mbps[scenario]

                interactive_mean[scenario] += total_interactive_Mbps[scenario]
                interactive_std_dev[scenario] += total_interactive_Mbps[scenario]*total_interactive_Mbps[scenario]

            if fout_raw is not None:
                fout_raw.write('%d\t%d\t%s\t%d' % (src,dst,load,case))
                for scenario in scenarios.keys():
                    fout_raw.write('\t%.3f\t%.3f\t%.3f' % (total_bulk_Mbps[scenario] + total_interactive_Mbps[scenario],total_bulk_Mbps[scenario],total_interactive_Mbps[scenario]))
                fout_raw.write('\n')

            tota_valid_case += 1

            # identify weird cases
            if (suggested_rerun is not None) and (network_name in exp_name):
                global suggested_rerun_cases_and_reasons
                suggested_rerun_cases_and_reasons[exp_name_case] = []

                rerun_flags = [False] * (max(scenarios.keys()) + 1)
                has_rerun_flag = False
                if total_bulk_Mbps[2] < 0.5*total_bulk_Mbps[1]:
                    reason = '-- multipath (%.3f) < 0.5 single path (%.3f)' % (total_bulk_Mbps[2], total_bulk_Mbps[1])
                    print(reason)
                    suggested_rerun_cases_and_reasons[exp_name_case].append('2: %s' % reason)
                    rerun_flags[2] = True
                    has_rerun_flag = True
                if total_bulk_Mbps[3] < 1.1*total_bulk_Mbps[1]:
                    reason = '-- coded (%.3f) < 1.1 single path (%.3f)' % (total_bulk_Mbps[3], total_bulk_Mbps[1])
                    print(reason)
                    suggested_rerun_cases_and_reasons[exp_name_case].append('3: %s' % reason)
                    rerun_flags[3] = True
                    has_rerun_flag = True
                if total_bulk_Mbps[3] < 0.95*total_bulk_Mbps[2]:
                    reason = '-- coded (%.3f) < 0.95 multipath (%.3f)' % (total_bulk_Mbps[3], total_bulk_Mbps[2])
                    print(reason)
                    suggested_rerun_cases_and_reasons[exp_name_case].append('3: %s' % reason)
                    rerun_flags[3] = True
                    has_rerun_flag = True
                if total_bulk_Mbps[6] < 0.9*total_bulk_Mbps[0]:
                    reason = '-- Steiner tree inf mem (%.3f) < 0.9 Steiner tree (%.3f)' % (total_bulk_Mbps[6], total_bulk_Mbps[0])
                    print(reason)
                    suggested_rerun_cases_and_reasons[exp_name_case].append('6: %s' % reason)
                    rerun_flags[6] = True
                    has_rerun_flag = True
                #if total_bulk_Mbps[] < total_bulk_Mbps[]:
                #    print('-- (%.3f) < (%.3f)' % (total_bulk_Mbps[], total_bulk_Mbps[]))
                #    rerun_flags[] = True
                #    has_rerun_flag = True

                # check interactive traffic: min < 0.9 max, min -> error
                max_interactive_Mbps = total_interactive_Mbps[0]
                #max_interactive_scenario = 0
                min_interactive_Mbps = total_interactive_Mbps[0]
                min_interactive_scenario = 0
                for scenario in scenarios.keys():
                    if scenario == 0:
                        continue
                    if max_interactive_Mbps < total_interactive_Mbps[scenario]:
                        max_interactive_Mbps = total_interactive_Mbps[scenario]
                        #max_interactive_scenario = scenario
                    if min_interactive_Mbps > total_interactive_Mbps[scenario]:
                        min_interactive_Mbps = total_interactive_Mbps[scenario]
                        min_interactive_scenario = scenario
                
                # min < 0.9 max, min -> error
                if min_interactive_Mbps < 0.9*max_interactive_Mbps:
                    reason = '-- min_interactive_Mbps (%.3f) < 0.9 max_interactive_Mbps (%.3f)' % (min_interactive_Mbps, max_interactive_Mbps)
                    print(reason)
                    suggested_rerun_cases_and_reasons[exp_name_case].append('%d: %s' % (min_interactive_scenario,reason))
                    rerun_flags[min_interactive_scenario] = True
                    has_rerun_flag = True

                if has_rerun_flag == True:
                    suggested_rerun[exp_name_case] = []
                    for scenario in scenarios.keys():
                        if rerun_flags[scenario]:
                            suggested_rerun[exp_name_case].append(scenario)

    if tota_valid_case > 0:
        for scenario in scenarios.keys():
            total_mean[scenario] /= tota_valid_case
            total_std_dev[scenario] /= tota_valid_case
            total_std_dev[scenario] -= total_mean[scenario]*total_mean[scenario]
            total_std_dev[scenario] = math.sqrt(total_std_dev[scenario])

            bulk_mean[scenario] /= tota_valid_case
            bulk_std_dev[scenario] /= tota_valid_case
            bulk_std_dev[scenario] -= bulk_mean[scenario]*bulk_mean[scenario]
            bulk_std_dev[scenario] = math.sqrt(bulk_std_dev[scenario])

            interactive_mean[scenario] /= tota_valid_case
            interactive_std_dev[scenario] /= tota_valid_case
            interactive_std_dev[scenario] -= interactive_mean[scenario]*interactive_mean[scenario]
            interactive_std_dev[scenario] = math.sqrt(interactive_std_dev[scenario])

    if fout is not None:
        fout.write('%d\t%d\t%s' % (src,dst,load))
        for scenario in scenarios.keys():
            #fout.write('\t%.3f\t%.3f' % (total_mean[scenario],total_std_dev[scenario]))
            fout.write('\t%.3f\t%.3f' % (bulk_mean[scenario],bulk_std_dev[scenario]))
            #fout.write('\t%.3f\t%.3f' % (interactive_mean[scenario],interactive_std_dev[scenario]))
        fout.write('\n')

    print('%d\t%d\t%s' % (src,dst,load),end='')
    for scenario in scenarios.keys():
        print('\t%.3f\t%.3f' % (bulk_mean[scenario],bulk_std_dev[scenario]),end='')
    print('')
    for scenario in scenarios.keys():
        print('\t%.3f\t%.3f' % (interactive_mean[scenario],interactive_std_dev[scenario]),end='')
    print('')

def add_fout_headline(fout):
    fout.write('total_num_traffic\ttotal_num_destinations\tload')
    for scenario_name in scenarios.values():
        fout.write('\t%s(total_mean,total_std_dev)' % scenario_name)
    fout.write('\n')

def add_fout_raw_headline(fout_raw):
    fout_raw.write('total_num_traffic\ttotal_num_destinations\tload\tcase')
    for scenario_name in scenarios.values():
        fout_raw.write('\t%s(total_Mbps,bulk_Mbps,interactive_Mbps)' % scenario_name)
    fout_raw.write('\n')

def summary(network_name, varying,suggested_rerun=None):
    #if network_name == 'B4':
    #    total_nodes = 13
    #elif network_name == 'Internet2':
    #    total_nodes = 9

    global total_nodes
    
    os.system('mkdir -p results/summary')
    os.system('mkdir -p results/summary/raw')
    fout = None
    fout_raw = None
    if varying == 's':
        fout = open('results/summary/%s-src.dat' % network_name, 'w')
        fout_raw = open('results/summary/raw/%s-src.dat' % network_name, 'w')
        add_fout_headline(fout)
        add_fout_raw_headline(fout_raw)
        for src in range(1,total_nodes+1):
            exp_name = network_name+"-"+str(src)+"-"+str(target_dst)+"-interactive-0.1"
            conclude(exp_name,fout,fout_raw,suggested_rerun)
    elif varying == 'd':
        fout = open('results/summary/%s-dst.dat' % network_name, 'w')
        fout_raw = open('results/summary/raw/%s-dst.dat' % network_name, 'w')
        add_fout_headline(fout)
        add_fout_raw_headline(fout_raw)
        for dst in range(2,total_nodes):
            exp_name = network_name+"-"+str(target_src)+"-"+str(dst)+"-interactive-0.1"
            conclude(exp_name,fout,fout_raw,suggested_rerun)
    else:
        fout = open('results/summary/%s-interactive.dat' % network_name, 'w')
        fout_raw = open('results/summary/raw/%s-interactive.dat' % network_name, 'w')
        add_fout_headline(fout)
        add_fout_raw_headline(fout_raw)
        for load in loads:
            exp_name = network_name+"-"+str(target_src)+"-"+str(target_dst)+"-interactive-"+str(load)
            conclude(exp_name,fout,fout_raw,suggested_rerun)
    fout.close()
    fout_raw.close()


def menu():
    global total_cases
    if len(sys.argv) > 1:
        total_cases = int(sys.argv[1])

    print('total summarized cases = %d ' % total_cases)
    auto = input('auto (empty is yes)? ')
    suggested_rerun = {}

    global network_name

    if auto == '':
        #for network_name in ['B4', 'Internet2']:
        for varying in ['s','d','i']:
            summary(network_name,varying,suggested_rerun)
    else:
        network_name = input('network_name: ')
        varying = input('varying src (s), dst (d), or interactive (i)?')
        summary(network_name,varying,suggested_rerun)

    if len(suggested_rerun) > 0:
        # check if we need to rerun
        with open('suggested_rerun.json','w') as frerun:
            json.dump(suggested_rerun,frerun)
    else:
        # clear suggested_rerun.json
        with open('suggested_rerun.json','w') as frerun:
            pass

    # summarize the suggested reruns and reasons
    for exp_name_case in suggested_rerun_cases_and_reasons.keys():
        if len(suggested_rerun_cases_and_reasons[exp_name_case]) > 0:
            print(exp_name_case)
            for reason in suggested_rerun_cases_and_reasons[exp_name_case]:
                print(reason)

if __name__ == '__main__':
    menu()
    #conclude_src_dst('B4-6-4-interactive-0.1')