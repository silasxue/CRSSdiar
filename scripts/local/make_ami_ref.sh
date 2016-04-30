#! /bin/bash

data_dir=$1 # kaldi format data directory
xml_dir=$2 # AMI segments directory
out_dir=$3

echo "writing reference functions in $out_dir"

## Generate labels:
label_dir=$out_dir/labels/
echo "Labels: $label_dir"
if [ -d $label_dir ]; then
    rm -rf $label_dir
fi
mkdir -p $label_dir/tmp/ # to store all temp files.
echo "wav.scp: "
echo "*****"
cat $data_dir/wav.scp
echo "*****"
python local/generate_labels.py $data_dir/wav.scp $xml_dir $label_dir/tmp/
bash $label_dir/tmp/xml2txt.jobs
bash $label_dir/tmp/seg2labels.jobs
mv $label_dir/tmp/*.label $label_dir/
ls $PWD/$label_dir/*.label | sort -u > $label_dir/labels.scp
rm -rf $label_dir/tmp


## Generate segments:
segment_dir=$out_dir/segments/
echo "Segments: $segment_dir"
if [ -d $segment_dir ]; then
    rm -rf $segment_dir
fi
mkdir -p $segment_dir/tmp/
while read f;do
    cat $f >> $segment_dir/tmp/labels.txt
done<$label_dir/labels.scp
labelToSegment ark:$segment_dir/tmp/labels.txt $segment_dir
rm -rf $segment_dir/tmp


## Generate rttm:
rttm_dir=$out_dir/rttms/
echo "RTTMs: $rttm_dir"
if [ -d $rttm_dir ]; then
    rm -rf $rttm_dir
fi
mkdir -p $rttm_dir/tmp/
while read f;do
    cat $f >> $rttm_dir/tmp/labels.txt
done<$label_dir/labels.scp
labelToRTTM ark:$rttm_dir/tmp/labels.txt $rttm_dir
rm -rf $rttm_dir/tmp
ls $PWD/$rttm_dir/*.rttm | sort -u > $rttm_dir/rttms.scp
