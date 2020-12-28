# CodedBulk c++ implementation codes

This file explains how to run the c++ implementation of the network coding project. The codes have been verified under Linux operating systems Fedora 27 and Ubuntu 14.04.01 LTS.

## Table of Contents:

1. Requirements
2. Deployment and Compilation
3. Creating New Applications
4. Running the Applications
5. Creating Batch Experiments

## 1. Requirements

To compile the codes, the system needs to have GNU Make
    https://www.gnu.org/software/make/
and GNU g++ that supports c++11 or more recent versions.


## 2. Deployment and Compilation

Choose the distribution of interest, e.g., with_thread_pool,
and go into the folder controller/.

Specify the IP addresses of the machines that will run the 
implementations in the topology file, which is under
settings/topology/. For example, to create a one link topology
with two machines of IP addresses 10.0.0.1 and 10.0.0.2,
modify the file onelink.cc_part to register the addresses by
    // machine 0, IP = 10.0.0.1
    REG_SW_ADDR(0,"10.0.0.1") 
    // machine 1, IP = 10.0.0.2
    REG_SW_ADDR(1,"10.0.0.2")

Compile the provided controller files under the folder
controller/ by
    make test
and run the controller by
    ./executable/test-controller-onelink
to generate the applications.

Upload the whole distribution folder, e.g., with_thread_pool,
to the machines that will run the implementations.

Setup the .node_info file in the folder to identify the 
machine id.

Find the compilation details in the README in apps/. 


## 3. Creating New Applications

Choose the distribution of interest, e.g., with_thread_pool,
and go into the folder controller/.

Create a topology file in Setup the IP information of the topology
in the folder settings/topology/.

Create a controller file in the folder main/. Example controllers
can be found under the folder tests/.

Make the controller by
    make
and run the resulting controller by 
    ./executable/sim-<controller name>
to create the applications.


## 4. Running the Applications

Find the running instructions in the folder apps/.


## 5. Creating Batch Experiments
-----------------------------

- Firstly, a topology file should be created and put under controller/settings/topology/.
REG_SW_ADDR( <node_id>, <one addresses for local socket> )
LINK_SW_TO_SW_IP( <head_node_id>, <head_IP_address>,  <tail_node_id>, <tail_IP_address>, 1.0)

- For varying sources/destinations, copy and modify the example file
    traffic-gen-<topology name>.cc
in the main folder. 
line 30: change the simulation name "B4-" to other name like "B4_WAN-"
    sim_name << "B4-" << total_num_sources << "-" << total_num_destinations << "-" << experiment;
line 36: change the topology file to use
    #include "../settings/topology/B4.cc_part"

- Modify the script <topology name>_auto_gen.sh.
line 2:  max_src=13
line 9:    ./executable/sim-traffic-gen-B4 $src $target_dst
line 18:   ./executable/sim-traffic-gen-B4 $target_src $dst
Run <topology name>_auto_gen.sh to generate the experiments.

- For interactive traffic experiments, put the workload files under workloads/
Remember, the time is in milliseconds!
Copy and modify the example file
    traffic-gen-<topology name>-interactive.cc
line 34:
    sim_name << "B4-interactive-" << workload[load];
line 40:
    #include "../settings/topology/B4.cc_part"
line 43:
    workload_name << "B4-180Mbps-" << workload[load];

Simply run the file by ./executable/sim-traffic-gen-<topology name>-interactive
to generate the experiment applications.

- Modify the corresponding helper scripts:
auto_compile.sh   -> auto compilation
examine.sh        -> doing one experiment, which is the basis for the following scripts
auto_exp.sh       -> auto experimenting
auto_redo.sh      -> auto redoing few experiments
under apps/helper_scripts will help do experiments more easily.
Put the modified scripts under apps/.

- download_scripts is used at the computer that collects information.
conclude.cpp       -> read the traffic_summary and compute the throughput
gen_summary.sh     -> generate the summary files using conclude
dev-tmux.sh        -> start a tmux session
examine_results.sh -> download one result, the basis for dl_results.sh
dl_results.sh      -> download the results and put the results in a folder structure 
                      that conclude.cpp can read
