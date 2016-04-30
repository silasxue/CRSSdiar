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
    mfccdir=mfcc
    for x in IS100.small; do
      steps/make_mfcc.sh --cmd "$train_cmd" --nj 1 data/$x exp/make_mfcc/$x $mfccdir || exit 1;
      steps/compute_cmvn_stats.sh data/$x exp/make_mfcc/$x $mfccdir || exit 1;
    done
}
run_mfcc

run_vad(){
    log_start "Doing VAD"	

    for x in IS100.small; do
   	 vaddir=exp/vad/$x
   	 diar/compute_vad_decision.sh --nj 1 data/$x $vaddir/log $vaddir 	
    done	
    log_end "Finish VAD"	
}
run_vad

make_ref(){
    log_start "Generate Reference Segments/Labels/RTTM files"	

    ami_annotated_segment=/home/chengzhu/work/SpeechCorpus/ami_dir/segments	

    for x in IS100.small; do
    	local/make_ami_ref.sh data/$x $ami_annotated_segment exp/ref/$x 
    done	

    log_end "Generate Reference Segments/Labels/RTTM files"	
}
make_ref

#test_ivectors(){
#    sid/test_ivector_score.sh --nj 1 exp/extractor_1024 data/toy local/label.ark exp/test_seg_ivector
#}
#test_ivectors;

#IvectorExtract(){
#    sid/extract_segment_ivector.sh --nj 1 exp/extractor_1024 data/toy local/label.ark exp/segment_ivectors
#}
#IvectorExtract

run_changedetection() {
    log_start "Run Change Detection Using BIC"	

    for x in IS100.small; do
	change_detect_bic.sh data/$x exp/ref/$x exp/change_detect/$x
    	#changeDetectBIC scp:data/toy/feats.scp ark:local/label.ark ark,scp,t:./tmp.ark,./tmp.scp
    done
	
    log_end "Run Change Detection Using BIC"	
}
run_changedetection

run_glpkIlpTemplate(){
    log_start "Generate GLPK Template of ILP problem "	

    for x in IS100.small; do
   	diar/generate_ILP_template.sh --nj 1 --delta 20 exp/extractor_1024 data/$x exp/ref/$x/segment exp/glpk_template/$x
    done

    log_end "Generate GLPK Template of ILP problem "	
}
#run_glpkIlpTemplate

run_glpk_Ilp(){
    log_start "Run ILP Clustering"	

    for x in IS100.small; do
	diar/ILP_clustering.sh exp/glpk_template/$x exp/glpk_ilp/$x
   	#glpsol --lp exp/glpk/$x -o exp/glpk/$x
       #glpkToRTTM $glpk_dir/glp.sol exp/segment.true/segments.scp $rttm_dir_true	
    done

    log_end "Run ILP Clustering"	
}
#run_glpk_Ilp

run_DER(){
    log_start "Compute Diarization Error Rate (DER)"	
 
    for x in IS100.small; do
       diar/compute_DER.sh $exp/glpk_ilp/$x/rttm $exp/result_DER/$x/rttm	
       #perl local/md-eval-v21.pl -r $rttm_dir_true/IS1000b.Mix-Headset.rttm -s $rttm_dir_est/IS1000b.Mix-Headset.rttm
    done	

    log_end "Compute Diarization Error Rate (DER)"	
}
#run_DER
