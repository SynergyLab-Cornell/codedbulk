#!/bin/bash
NETWORKS=("B4" "Internet2")
NODES=(13 9)

cd ..
./waf

for index in ${!NETWORKS[*]}
do
  net=${NETWORKS[$index]}
  node=${NODES[$index]}
  nodea=$((${NODES[$index]}-1))

  rm -f results/$net-dst.dat
  seq 2 $nodea | while read n
  do
    ./waf --run "$net-varying-dst --d=$n"
  done

  rm -f results/$net-src.dat
  seq 1 $node | while read n
  do
    ./waf --run "$net-varying-src --s=$n"
  done
done
