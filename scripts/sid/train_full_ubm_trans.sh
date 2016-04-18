#!/bin/bash
# Copyright 2012  Johns Hopkins University (Author: Daniel Povey).  Apache 2.0.
#           2013  Daniel Povey

# This trains a full-covariance UBM from an existing (diagonal or full) UBM,
# for a specified number of iterations.  This is for speaker-id systems
# (we use features specialized for that, and vad).

# Begin configuration section.
nj=16
cmd=run.pl
stage=-2
num_gselect=20 # cutoff for Gaussian-selection that we do once at the start.
subsample=5
num_iters=4
min_gaussian_weight=1.0e-04
remove_low_count_gaussians=true # set this to false if you need #gauss to stay fixed.
cleanup=true
delta_window=3
delta_order=2

# End configuration section.

echo "$0 $@"  # Print the command line for logging

if [ -f path.sh ]; then . ./path.sh; fi
. parse_options.sh || exit 1;

if [ $# != 3 ]; then
  echo "Usage: steps/train_full_ubm.sh <data> <old-ubm-dir> <new-ubm-dir>"
  echo "Trains a full-covariance UBM starting from an existing diagonal or"
  echo "full-covariance UBM system."
  echo " e.g.: steps/train_full_ubm.sh --num-iters 8 data/train exp/diag_ubm exp/full_ubm"
  echo "main options (for others, see top of script file)"
  echo "  --config <config-file>                           # config containing options"
  echo "  --cmd (utils/run.pl|utils/queue.pl <queue opts>) # how to run jobs."
  echo "  --nj <n|16>                                      # number of parallel training jobs"
  echo "  --num-gselect <n|20>                             # Number of Gaussians to select using"
  echo "                                                   # initial model (diagonalized if needed)"
  echo "  --subsample <n|5>                                # Take every n'th sample, for efficiency"
  echo "  --num-iters <n|4>                                # Number of iterations of E-M"
  echo "  --min-gaussian-weight <weight|1.0e-05>           # Minimum Gaussian weight (below this,"
  echo "                                                   # we won't update, and will remove Gaussians"
  echo "                                                   # if --remove-low-count-gaussians is true"
  echo "  --remove-low-count-gaussians <true,false|true>   # If true, remove Gaussians below min-weight"
  echo "                                                   # (will only happen on last iteration, in any case"
  echo "  --cleanup <true,false|true>                      # If true, clean up accumulators, intermediate"
  echo "                                                   # models and gselect info"
  exit 1;
fi

data=$1
alidir=$2
dir=$3

mkdir -p $dir/log
echo $nj > $dir/num_jobs
sdata=$data/split$nj;
utils/split_data.sh $data $nj || exit 1;

gmm-info $alidir/final.mdl | grep pdfs | awk '{print $4}' > $dir/num_gauss
num_gauss=`cat $dir/num_gauss`
nj=`cat $alidir/num_jobs`

delta_opts="--delta-window=$delta_window --delta-order=$delta_order"
echo $delta_opts > $dir/delta_opts

## Set up features.
feats="ark,s,cs:add-deltas $delta_opts scp:$sdata/JOB/feats.scp ark:- | apply-cmvn-sliding --norm-vars=false --center=true --cmn-window=300 ark:- ark:- |"

stat_posts="ark,s,cs:ali-to-pdf $alidir/final.mdl \"ark:gunzip -c $alidir/ali.JOB.gz|\"  ark:- | ali-to-post ark:- ark:- |"

$cmd JOB=1:$nj $dir/log/acc.$x.JOB.log \
  fgmm-global-acc-stats-post --binary=false "$stat_posts" $num_gauss "$feats" $dir/stats.JOB.acc || exit 1;

#  fgmm-global-init-acc-stats-post --binary=false "$stat_posts" $num_gauss "$feats" $dir/stats.JOB.acc || exit 1;


$cmd $dir/log/init.log \
  fgmm-global-init-from-accs --verbose=2 \
  "fgmm-global-sum-accs - $dir/stats.*.acc |" $num_gauss \
  $dir/final.ubm || exit 1;

exit 0;
