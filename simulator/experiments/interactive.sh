#!/bin/bash
NETWORKS=("B4" "Internet2")
LOADS=(0.1 0.2 0.3 0.4 0.5)

cd ..
./waf

for net in $NETWORKS
do
  rm -f results/$net-interactive.dat
  for load in $LOADS
  do
    ./waf --run "$net-interactive --l=$load"
  done
done