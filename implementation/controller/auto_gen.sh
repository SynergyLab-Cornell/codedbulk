#!/bin/bash
if [ $# -lt 2 ] 
then
  echo "./auto_gen.sh <network name> <number of nodes>"
  echo "e.g. ./auto_gen B4 13"
  exit -1
fi

network=$1
max_src=$2
max_dst=$(($max_src-1))
target_src=6
target_dst=3

./executable/sim-traffic-gen-$network $target_src $target_dst 4

for ((src=1; src <= $max_src; src++))
do
  ./executable/sim-traffic-gen-$network $src $target_dst 1
done

for ((dst=2; dst <= $max_dst; dst++))
do
  if [ "$dst" = "$target_dst" ]
  then
    continue
  fi
  ./executable/sim-traffic-gen-$network $target_src $dst 1
done