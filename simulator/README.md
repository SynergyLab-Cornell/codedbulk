# CodedBulk ns-3 simulation codes

This file explains how to run the network coding project
experiments in ns-3.30. The codes have been verified under Fedora 27, a Linux operating system.

## Table of Contents:

1. Requirements
2. Installation
3. Running the Experiments

## 1. Requirements

The first program needed is ns3.30, which can be downloaded
from
    https://www.nsnam.org/ns-3-27/
Once the ns-allinone-3.30.tar.bz2 is extracted, there will
be a ns-allinone-3.30 folder. Inside the folder there will
be ns-3.30.

In ns-3.30, we will need to install the OpenFlow support,
the instructions can be found at
    https://www.nsnam.org/docs/release/3.30/models/html/openflow-switch.html

After the installation, there should be a folder named 
``openflow'' under ``ns-3.30''. Then we can configure ns-3.30
to support OpenFlow by typing
    ./waf configure --with-openflow=openflow

Then we can test if ns-3.30 is successfully settled by compiling
    ./waf

It is possible that the system may be lack of some libraries.
Follow the below instructions to fix the library dependency
based on the running system
    https://www.nsnam.org/wiki/Installation

After a successful compilation, ns-3.30 will list a list of 
built modules.


## 2. Installation

To install the network coding layer ns-3 implementation,
copy the files in the folder ``ns-3'' into the previously
compiled ``ns-3.30'' folder. And compile ns-3.30 using the
same commands
    ./waf configure -d optimized --with-openflow=openflow
    ./waf


## 3. Running the Experiments

The experiments are within the folder ``scratch.'' To run
individual experiments, just type
    ./waf --run=<experiment_name>

Shell scripts to run existing experiments are available in
the folder ``experiment.'' Go into the folder and run the
corresponding scripts by
    ./<script_name>
such as
    ./varying_src_dst.sh

