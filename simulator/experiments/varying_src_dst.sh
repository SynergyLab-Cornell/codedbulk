#!/bin/bash
NETWORKS=("B4" "Internet2")
NODES=(13 9)

counter=1

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
    if [ $((counter%5)) -eq 0 ]
    then
      ./waf --run "$net-varying-dst --d=$n"
    else
      ./waf --run "$net-varying-dst --d=$n" &
      sleep 3
    fi
    counter=$((counter+1))
  done

  rm -f results/$net-src.dat
  seq 1 $node | while read n
  do
    if [ $((counter%5)) -eq 0 ]
    then
      ./waf --run "$net-varying-src --s=$n"
    else
      ./waf --run "$net-varying-src --s=$n" &
      sleep 3
    fi
    counter=$((counter+1))
  done
done
