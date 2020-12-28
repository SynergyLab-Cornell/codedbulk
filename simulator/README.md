# CodedBulk ns-3 simulation codes

This file explains how to run the CodedBulk simulations in ns-3.30. The codes have been verified under Fedora 32, a Linux operating system.

## Table of Contents:

1. Requirements and Compilation
2. Running the Experiments

## 1. Requirements and Compilation

Follow the instructions in 
    https://www.nsnam.org/wiki/Installation
to install necessary library dependency according to the host operating system.

Compile the simulation codes by
```
    make
```

After a successful compilation, ns-3.30 will show a list of built modules.

## 2. Running the Experiments

Shell scripts to run existing experiments are available in the folder ``experiment.'' Go into the folder and run the corresponding scripts by
    ./<script_name>
such as
    ./varying_src_dst.sh
the results will be available in the folder ``results.''