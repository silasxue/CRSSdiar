#! /bin/bash

data_dir=$1
ref_dir=$2 # ground-truth
out_dir=$3 # bic decisions

if [ -d $out_dir ]; then
    rm -rf $out_dir
fi
mkdir -p $out_dir/tmp
while read x;do
    cat $x >> $out_dir/tmp/labels.ark
done<$ref_dir/labels/labels.scp

changeDetectBIC scp:$data_dir/feats.scp ark:$out_dir/tmp/labels.ark ark,scp,t:$out_dir/bic.ark,$out_dir/bic.scp
rm -rf $out_dir/tmp
