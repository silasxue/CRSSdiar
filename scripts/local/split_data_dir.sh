#! /bin/bash

# Split wav.scp, utt2spk, and spk2utt from input data dir into multiple directories. 
# one per each line. 
indata=$1
nfiles=`cat $indata/wav.scp | wc -l`
echo "$nfiles found in $indata"

fileidx=1
while [ $fileidx -le $nfiles ]; do
    mkdir ${indata}_file_${fileidx}
    head -n $fileidx $indata/wav.scp | tail -n 1 > ${indata}_file_${fileidx}/wav.scp
    head -n $fileidx $indata/utt2spk | tail -n 1 > ${indata}_file_${fileidx}/utt2spk
    head -n $fileidx $indata/spk2utt | tail -n 1 > ${indata}_file_${fileidx}/spk2utt
    head -n $fileidx $indata/feats.scp | tail -n 1 > ${indata}_file_${fileidx}/feats.scp
    if [ -f $indata/vad.scp ]; then
        head -n $fileidx $indata/vad.scp | tail -n 1 > ${indata}_file_${fileidx}/vad.scp
    else
        echo "VAD files not generated for $indata!"
        exit 1
    fi
    fileidx=$[$fileidx+1]
done

