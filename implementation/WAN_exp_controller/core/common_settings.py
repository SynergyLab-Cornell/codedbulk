#!/usr/bin/env python
connection_port = 999
exp_duration_seconds=60

bandwidth=200
tcp_wait_timeout=60

target_src=6
target_dst=3

loads = [0.05,0.1,0.15,0.2]

scenarios = {}
scenarios[0] = 'Steiner_tree_multicast'
scenarios[1] = 'single_path_multicast'
scenarios[2] = 'multi_path_multicast'
scenarios[3] = 'coded_multicast'
#scenarios[4] = 'Steiner_tree_without_coding'
#scenarios[5] = 'Steiner_tree_store-and-forward_disk'
scenarios[6] = 'Steiner_tree_store-and-forward_memory'

total_cases=5