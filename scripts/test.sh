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

run_mfcc(){
    log_start "Extract MFCC features"	

    mfccdir=mfcc
    for x in IS1000a; do
      steps/make_mfcc.sh --cmd "$train_cmd" --nj 1 data/$x exp/make_mfcc/$x $mfccdir || exit 1;
      steps/compute_cmvn_stats.sh data/$x exp/make_mfcc/$x $mfccdir || exit 1;
    done

    log_end "Extract MFCC features"	
}
#run_mfcc

run_vad(){
    log_start "Doing VAD"	

    for x in toy IS1000a; do
   	 vaddir=exp/vad/$x
   	 diar/compute_vad_decision.sh --nj 1 data/$x $vaddir/log $vaddir 	
    done	
    log_end "Finish VAD"	
}
#run_vad

make_ref(){
    log_start "Generate Reference Segments/Labels/RTTM files"	

    ami_annotated_segment=/home/chengzhu/work/SpeechCorpus/ami_dir/segments

    for x in IS1000a; do
    	local/make_ami_ref.sh data/$x $ami_annotated_segment exp/ref/$x 
    done	

    log_end "Generate Reference Segments/Labels/RTTM files"	
}
make_ref

test_ivectors(){
    for x in toy; do
        diar/test_ivector_score.sh --nj 1 exp/extractor_1024 data/$x exp/ref/$x/labels exp/temp/test_ivectors
    done
}
#test_ivectors

#IvectorExtract(){
#    sid/extract_segment_ivector.sh --nj 1 exp/extractor_1024 data/toy local/label.ark exp/segment_ivectors
#}
#IvectorExtract

run_changedetection() {
    log_start "Run Change Detection Using BIC"	

    for x in IS1000a; do
	    local/change_detect_bic.sh data/$x exp/ref/$x exp/change_detect/$x
    done
	
    log_end "Run Change Detection Using BIC"	
}
#run_changedetection

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

run_glpkIlpTemplate(){
    log_start "Generate GLPK Template of ILP problem "	

    for x in IS1000a; do
   	#diar/generate_ILP_template.sh --nj 1 --seg_min 0 --delta 30 exp/extractor_1024.swbd data/$x exp/ref/$x/segments exp/glpk_template/$x
   	#diar/generate_ILP_template.sh --nj 1 --seg_min 50 --delta 30 exp/extractor_1024.swbd data/$x exp/change_detect/$x/segments exp/glpk_template/$x
   	diar/generate_ILP_template.sh --nj 1 --seg_min 0 --delta 30 exp/extractor_1024.swbd data/$x exp/ref/$x/vad_label/segments exp/glpk_template/$x
    done

    log_end "Generate GLPK Template of ILP problem "	
}
run_glpkIlpTemplate

run_glpk_Ilp(){
    log_start "Run ILP Clustering"	

    for x in IS1000a; do
	#diar/ILP_clustering.sh --seg_min 0 exp/glpk_template/$x exp/ref/$x/segments exp/glpk_ilp/$x
	#diar/ILP_clustering.sh --seg_min 50 exp/glpk_template/$x exp/change_detect/$x/segments exp/glpk_ilp/$x
	diar/ILP_clustering.sh --seg_min 0 exp/glpk_template/$x exp/ref/$x/vad_label/segments exp/glpk_ilp/$x
    done

    log_end "Run ILP Clustering"	
}
run_glpk_Ilp

run_DER(){
    log_start "Compute Diarization Error Rate (DER)"	
 
    for x in IS1000a; do
       diar/compute_DER.sh exp/ref/$x/rttms exp/glpk_ilp/$x/rttms exp/result_DER/$x	
    done	

    log_end "Compute Diarization Error Rate (DER)"	
}
run_DER
