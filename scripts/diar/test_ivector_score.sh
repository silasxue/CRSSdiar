#!/bin/bash

# Copyright     2013  Daniel Povey
#               2014  David Snyder
# Apache 2.0.

# This script extracts iVectors for a set of utterances, given
# features and a trained iVector extractor.

# Begin configuration section.
nj=30
cmd="run.pl"
stage=0
num_gselect=20 # Gaussian-selection using diagonal model: number of Gaussians to select
min_post=0.025 # Minimum posterior to use (posteriors below this are pruned out)
posterior_scale=1.0 # This scale helps to control for successve features being highly
                    # correlated.  E.g. try 0.1 or 0.3.
# End configuration section.

echo "$0 $@"  # Print the command line for logging

if [ -f path.sh ]; then . ./path.sh; fi
. parse_options.sh || exit 1;


if [ $# != 4 ]; then
  echo "Usage: $0 <extractor-dir> <data> <ivector-dir>"
  echo " e.g.: $0 exp/extractor_2048_male data/train_male exp/ivectors_male"
  echo "main options (for others, see top of script file)"
  echo "  --config <config-file>                           # config containing options"
  echo "  --cmd (utils/run.pl|utils/queue.pl <queue opts>) # how to run jobs."
  echo "  --num-iters <#iters|10>                          # Number of iterations of E-M"
  echo "  --nj <n|10>                                      # Number of jobs (also see num-processes and num-threads)"
  echo "  --num-threads <n|8>                              # Number of threads for each process"
  echo "  --stage <stage|0>                                # To control partial reruns"
  echo "  --num-gselect <n|20>                             # Number of Gaussians to select using"
  echo "                                                   # diagonal model."
  echo "  --min-post <min-post|0.025>                      # Pruning threshold for posteriors"
  exit 1;
fi

srcdir=$1
data=$2
label_dir=$3
dir=$4


for f in $srcdir/final.ie $srcdir/final.ubm $data/feats.scp ; do
  [ ! -f $f ] && echo "No such file $f" && exit 1;
done

if [ ! -d $dir/tmp ]; then
    mkdir -p $dir/tmp
fi

if [ -f $dir/tmp/labels.ark ]; then
    rm  $dir/tmp/labels.ark
fi

while read x;do
    cat $x >> $dir/tmp/labels.ark
done<$label_dir/labels.scp


# Set various variables.
mkdir -p $dir/log
sdata=$data/split$nj;
utils/split_data.sh $data $nj || exit 1;

delta_opts=`cat $srcdir/delta_opts 2>/dev/null`

## Set up features.
#feats="ark,s,cs:add-deltas $delta_opts scp:$sdata/JOB/feats.scp ark:- | apply-cmvn-sliding --norm-vars=false --center=true --cmn-window=300 ark:- ark:- | select-voiced-frames ark:- scp,s,cs:$sdata/JOB/vad.scp ark:- |"
feats="ark,s,cs:add-deltas $delta_opts scp:$sdata/JOB/feats.scp ark:- | apply-cmvn-sliding --norm-vars=false --center=true --cmn-window=300 ark:- ark:- |"


if [ $stage -le 0 ]; then
  echo "$0: extracting iVectors"
  dubm="fgmm-global-to-gmm $srcdir/final.ubm -|"

  $cmd JOB=1:$nj $dir/log/extract_posterior.JOB.log \
    gmm-gselect --n=$num_gselect "$dubm" "$feats" ark:- \| \
    fgmm-global-gselect-to-post --min-post=$min_post $srcdir/final.ubm "$feats" \
	ark,s,cs:- ark:- \| scale-post ark:- $posterior_scale ark,t:$dir/posterior.JOB || exit 1;

  $cmd JOB=1:$nj $dir/log/ivector_score.JOB.log \
    ivectorTest ark:$dir/tmp/labels.ark "$feats" ark,s,cs:$dir/posterior.JOB $srcdir/final.ie scp:exp/dev.iv/ivector.scp ark:data/dev/utt2spk || exit 1;

fi


# plot true/false hypotheses: (P(d|H0) and P(d|H1)):
grep "TRUE Cosine scores" $dir/log/ivector_score.1.log > $dir/tmp/true_scores
grep "FALSE Cosine scores" $dir/log/ivector_score.1.log > $dir/tmp/false_scores
grep "TRUE Mahalanobis scores" $dir/log/ivector_score.1.log > $dir/tmp/true_scores
grep "FALSE Mahalanobis scores" $dir/log/ivector_score.1.log > $dir/tmp/false_scores
python local/plot_distributions.py $dir/tmp/true_scores $dir/tmp/false_scores

#rm -rf $dir/tmp


