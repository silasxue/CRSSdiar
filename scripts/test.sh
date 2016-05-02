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
    for x in toy_2; do
      steps/make_mfcc.sh --cmd "$train_cmd" --nj 1 data/$x exp/make_mfcc/$x $mfccdir || exit 1;
      steps/compute_cmvn_stats.sh data/$x exp/make_mfcc/$x $mfccdir || exit 1;
    done

    log_end "Extract MFCC features"	
}
#run_mfcc

run_vad(){
    log_start "Doing VAD"	

    for x in toy_2; do
   	 vaddir=exp/vad/$x
   	 diar/compute_vad_decision.sh --nj 1 data/$x $vaddir/log $vaddir 	
    done	
    log_end "Finish VAD"	
}
#run_vad

make_ref(){
    log_start "Generate Reference Segments/Labels/RTTM files"	

    ami_annotated_segment=/home/nxs113020/Downloads/ami_dir/segments

    for x in toy_2; do
    	local/make_ami_ref.sh data/$x $ami_annotated_segment exp/ref/$x 
    done	

    log_end "Generate Reference Segments/Labels/RTTM files"	
}
make_ref

test_ivectors(){
    for x in toy_2; do
        diar/test_ivector_score.sh --nj 1 exp/extractor_1024 data/$x exp/ref/$x/labels exp/temp/test_ivectors
    done
}
test_ivectors;
#
#IvectorExtract(){
#    sid/extract_segment_ivector.sh --nj 1 exp/extractor_1024 data/toy local/label.ark exp/segment_ivectors
#}
#IvectorExtract

run_changedetection() {
    log_start "Run Change Detection Using BIC"	

    for x in toy_2; do
	    local/change_detect_bic.sh data/$x exp/ref/$x exp/change_detect/$x
    done
	
    log_end "Run Change Detection Using BIC"	
}
#run_changedetection

run_glpkIlpTemplate(){
    log_start "Generate GLPK Template of ILP problem "	

    for x in toy_2; do
   	diar/generate_ILP_template.sh --nj 1 --delta 20 exp/extractor_1024 data/$x exp/ref/$x/segments exp/glpk_template/$x
    done

    log_end "Generate GLPK Template of ILP problem "	
}
#run_glpkIlpTemplate

run_glpk_Ilp(){
    log_start "Run ILP Clustering"	

    for x in toy_2; do
	diar/ILP_clustering.sh exp/glpk_template/$x exp/ref/$x/segments exp/glpk_ilp/$x
    done

    log_end "Run ILP Clustering"	
}
#run_glpk_Ilp

run_DER(){
    log_start "Compute Diarization Error Rate (DER)"	
 
    for x in toy_2; do
       diar/compute_DER.sh exp/ref/$x/rttms exp/glpk_ilp/$x/rttms exp/result_DER/$x	
       #perl local/md-eval-v21.pl -r $rttm_dir_true/IS1000b.Mix-Headset.rttm -s $rttm_dir_est/IS1000b.Mix-Headset.rttm
    done	

    log_end "Compute Diarization Error Rate (DER)"	
}
#run_DER
