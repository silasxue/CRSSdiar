#! /usr/bin/bash

data_dir=$1 # kaldi format data directory
xml_dir=$2 # AMI segments directory
out_dir=$3

## Generate labels:
label_dir=$out_dir/labels/
mkdir -p $label_dir
mkdir -p $label_dir/tmp/ # to store all temp files.
python local/generate_labels.py $data_dir/wav.scp $xml_dir $label_dir/tmp/
bash $label_dir/tmp/xml2txt.jobs
bash $label_dir/tmp/seg2labels.jobs
mv $label_dir/tmp/labels_*.txt $label_dir/
ls $PWD/$label_dir/labels_*.txt | sort -u > $label_dir/labels.scp
rm -rf $label_dir/tmp


## Generate segments:
segment_dir=$out_dir/segments/
if [ -f $segment_dir/segments.scp ]; then
    rm $segment_dir/segments.scp
fi
if [ -d $segment_dir/tmp ]; then
   rm -rf $segment_dir/tmp
fi
mkdir -p $segment_dir/tmp/
while read f;do
    cat $f >> $segment_dir/tmp/labels.txt
done<$label_dir/labels.scp
labelToSegment ark:$segment_dir/tmp/labels.txt $segment_dir
rm -rf $segment_dir/tmp


## Generate rttm:
rttm_dir=$out_dir/rttm/
if [ -f $rttm_dir/rttms.scp ]; then
    rm $rttm_dir/rttms.scp
fi
if [ -d $rttm_dir/tmp ]; then
    rm -rf $rttm_dir/tmp
fi
mkdir -p $rttm_dir/tmp/
while read f;do
    cat $f >> $rttm_dir/tmp/labels.txt
done<$label_dir/labels.scp
labelToRTTM ark:$rttm_dir/tmp/labels.txt $rttm_dir
rm -rf $rttm_dir/tmp
ls $PWD/$rttm_dir/*.rttm | sort -u > $rttm_dir/rttms.scp
