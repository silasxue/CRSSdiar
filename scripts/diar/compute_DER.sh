#!/bin/bash 

# Copyright    2013  Daniel Povey
# Apache 2.0
# To be run from .. (one directory up from here)
# see ../run.sh for example

# Compute energy based VAD output 
# We do this in just one job; it's fast.
#

cmd=run.pl

echo "$0 $@"  # Print the command line for logging

if [ -f path.sh ]; then . ./path.sh; fi
. parse_options.sh || exit 1;

if [ $# != 3 ]; then
   echo "Usage: $0 [options] <data-dir> <log-dir> <path-to-vad-dir>";
   exit 1;
fi

ref_dir=$1
match_dir=$2
result_dir=$3


for f in $ref_dir/rttm.scp $match_dir/rttm.scp; do
  if [ ! -f $f ]; then
    echo "Generate rttm.scp file for either reference or matching does not exist"
    exit 1;
  fi
done

mkdir -p $result_dir/log; rm -f $result_dir/* ; rm -f $result_dir/log/*

for x in $match_dir/rttm.scp; do
    cat $match_dir/$x >> $result_dir/match.rttm
    cat $ref_dir/$x >> $result_dir/ref.rttm
done

perl local/md-eval-v21.pl -r $result_dir/ref.rttm -s $result_dir/match.rttm 2>&1 | tee $result_dir/diar_err   	
 
