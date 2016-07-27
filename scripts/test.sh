#!/bin/bash

log_start(){
  echo "#####################################################################"
  echo "Spawning *** $1 *** on" `date` `hostname`
  echo ---------------------------------------------------------------------
}

log_end(){
  echo ---------------------------------------------------------------------
  echo "Done *** $1 *** on" `date` `hostname`
  echo "#####################################################################"
}

. cmd.sh
. path.sh

set -e # exit on error

data="toy"
run_mfcc(){
    log_start "Extract MFCC features"

    mfccdir=mfcc
    datadir=$1
    for x in $datadir; do
      steps/make_mfcc.sh --cmd "$train_cmd" --nj 1 data/$x exp/make_mfcc/$x $mfccdir || exit 1;
      steps/compute_cmvn_stats.sh data/$x exp/make_mfcc/$x $mfccdir || exit 1;
    done

    log_end "Extract MFCC features"
}
#run_mfcc $data

run_vad(){
    log_start "Doing VAD"
    datadir=$1
    for x in $datadir; do
     vaddir=exp/vad/$x
     diar/compute_vad_decision.sh --nj 1 data/$x $vaddir/log $vaddir
    done
    log_end "Finish VAD"
}
#run_vad $data

make_ref(){
    log_start "Generate Reference Segments/Labels/RTTM files"

    #ami_annotated_segment=/home/chengzhu/work/SpeechCorpus/ami_dir/segments
    ami_annotated_segment=/home/nxs113020/Downloads/ami_dir/segments

    x=$1
    local/make_ami_ref.sh data/$x $ami_annotated_segment exp/ref/$x

    log_end "Generate Reference Segments/Labels/RTTM files"
}
make_ref $data 

test_ivectors(){

    x=$1
    diar/test_ivector_score.sh --nj 1 exp/extractor_1024 data/$x exp/ref/$x/labels exp/temp/test_ivectors
}
test_ivectors $data


run_changedetection() {
    log_start "Run Change Detection Using BIC"

    x=$1
    local/change_detect_bic.sh data/$x exp/ref/$x exp/change_detect/$x

    log_end "Run Change Detection Using BIC"
}
#run_changedetection $data

train_extractor(){
    ubmdim=1024
    ivdim=60

    sid/train_diag_ubm.sh --parallel-opts "" --nj 1 --cmd "$train_cmd" data/ES20 ${ubmdim} \
    exp/diag_ubm_${ubmdim} || exit 1;

    sid/train_full_ubm.sh --nj 1 --cmd "$train_cmd" data/ES20 \
       exp/diag_ubm_${ubmdim} exp/full_ubm_${ubmdim} || exit 1;

    sid/train_ivector_extractor.sh --nj 1 --cmd "$train_cmd" --num-gselect 15 \
      --ivector-dim $ivdim --num-iters 5 exp/full_ubm_${ubmdim}/final.ubm data/ES20 \
      exp/extractor_ES20 || exit 1;
}
#train_extractor

too_long(){
    x=$1
    n_samples=`cat data/$x/wav.scp | cut -d ' ' -f 4 | perl -ne 'if(m/(\S+)/){print \`soxi -s $1\`}'`
    is_too_long=0
    if [ $n_samples -gt 35000000 ]; then
       is_too_long=1 
    fi 
    echo "$is_too_long"
}
run_glpkIlpTemplate(){
    log_start "Generate GLPK Template of ILP problem "

    x=$1
    diar/generate_ILP_template.sh --nj 1 --seg_min 50 --delta 30 \
      exp/extractor_1024 data/$x exp/change_detect/$x/segments exp/glpk_template/$x

    log_end "Generate GLPK Template of ILP problem "
}
#run_glpkIlpTemplate $data

run_glpk_Ilp(){
    log_start "Run ILP Clustering"

    x=$1
    diar/ILP_clustering.sh --seg_min 50 exp/glpk_template/$x exp/change_detect/$x/segments exp/glpk_ilp/$x

    log_end "Run ILP Clustering"
}
#run_glpk_Ilp $data

run_DER(){
    log_start "Compute Diarization Error Rate (DER)"
    x=$1    
    diar/compute_DER.sh exp/ref/$x/rttms exp/glpk_ilp/$x/rttms exp/result_DER/$x

    log_end "Compute Diarization Error Rate (DER)"
}
#run_DER $data


run_diarization(){
    # Perform diarization on each file (separately):
    datadir=$1
    # split data directory into individual files. 
    nfiles=`local/split_data_dir.sh data/$datadir | cut -d ' ' -f 1`
    fileidx=1
    while [ $fileidx -le $nfiles ]; do
        make_ref ${datadir}_file_${fileidx}
        run_changedetection ${datadir}_file_${fileidx}
        test_ivectors ${datadir}_file_${fileidx}
        long=$(too_long ${datadir}_file_${fileidx})
        if [ $long -eq 0 ]; then
            run_glpkIlpTemplate ${datadir}_file_${fileidx}
            run_glpk_Ilp ${datadir}_file_${fileidx}
            run_DER ${datadir}_file_${fileidx}
        fi
        fileidx=$[$fileidx+1]
    done
    
    grep "OVERALL SPEAKER DIARIZATION ERROR" exp/result_DER/${datadir}_file_*/diar_err 
}
#run_diarization $data







