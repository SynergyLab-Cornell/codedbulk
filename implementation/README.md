# CodedBulk c++ implementation codes

This file explains how to run the c++ implementation of the network coding project. The codes have been verified under Linux operating systems Fedora 32 and Ubuntu 14.04.01 LTS.

## Table of Contents:

1. Requirements
2. Experiment Setup
3. WAN Experiment Generation
4. Deployment
5. Run Experiments

## 1. Requirements

To compile the codes, the system needs to have GNU Make
    https://www.gnu.org/software/make/
and GNU g++ that supports c++11 or more recent versions.

The WAN experiment controller requires Python 3.

## 2. Experiment Setup

To create a CodedBulk experiment, add a folder under the folder 'settings' with a desired name (referred as network name in the following context).

link.csv describes the network topology.
public_ips.csv and private_ips.csv are the public/private ips held by each interface at each node.
The workload describes the interactive traffic, formatted as described in settings/format.

## 3. WAN Experiment Generation

Run
```
    python generate_exp.py
```
and enter the network name to generate the WAN experiment and the controller under WAN_exp_controller/exp_\<network name\>.

## 4. Deployment

To deploy the experiment codes to run on servers, modify common_settings.py and server_information.py in WAN_exp_controller/core, and put the server ssh key files in WAN_exp_controller/exp_\<network name\>. Name each key file "node_\<server number\>.pem" where the server number is defined as in link.csv/public_ips.csv/private_ips.csv.

Then go into the folder WAN_exp_controller/exp_\<network name\> and run
```
    python deployment.py
```
to deploy the experiments to the servers.

## 5. Run Experiments

In the folder WAN_exp_controller/exp_\<network name\>, run
```
    python local_controller.py
```
type "rw" to run the WAN experiments.