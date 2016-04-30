#!/bin/bash

# Copyright     2013  Daniel Povey
#               2014  David Snyder
# Apache 2.0.

# This script extracts iVectors for a set of utterances, given
# features and a trained iVector extractor.

# Begin configuration section.
nj=1
cmd="run.pl"
# End configuration section.

echo "$0 $@"  # Print the command line for logging

if [ -f path.sh ]; then . ./path.sh; fi
. parse_options.sh || exit 1;

if [ $# != 3 ]; then
  echo "Usage: $0 <input:ilp_template_dir><input:segment_dir> <output:ilp_dir>"
  exit 1;
fi

srcdir=$1
segdir=$2
dir=$3


# Set various variables.
mkdir -p $dir/log; mkdir -p $dir/rttm; rm -f $dir/log/*; rm -f $dir/rttm/*

# Perform ILP clustering using GLPK tool
glpsol --lp $srcdir/glpk.ilp.template -o $dir/glpk.ilp.solution 2>&1 | tee $dir/log/glpk.ilp.log

# Interperate ILP clustering result in GLPK format into rttm for compute DER infuture
glpkToRTTM $dir/glpk.ilp.solution $segdir/segments.scp $dir/rttm

