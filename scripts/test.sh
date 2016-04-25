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
    for x in toy; do
      steps/make_mfcc.sh --cmd "$train_cmd" --nj 1 data/$x exp/make_mfcc/$x $mfccdir || exit 1;
      steps/compute_cmvn_stats.sh data/$x exp/make_mfcc/$x $mfccdir || exit 1;
      #utils/fix_data_dir.sh data/$x
    done
}
#run_mfcc

run_vad(){

    log_start "Doing VAD"	

    vaddir=exp/vad	
    sid/compute_vad_decision.sh --nj 1 data/toy $vaddir/log $vaddir 	

    log_end "Finish VAD"	
}
#run_vad

test_changedetection() {
    changeDetectBIC scp:data/toy/feats.scp ark:local/label.ark ark,scp,t:./tmp.ark,./tmp.scp
}
#test_changedetection

test_ivectors(){
    sid/test_ivector_score.sh --nj 1 exp/extractor_1024 data/toy local/label.ark exp/test_seg_ivector
}
#test_ivectors;

IvectorExtract(){
    sid/extract_segment_ivector.sh --nj 1 exp/extractor_1024 data/toy local/label.ark exp/segment_ivectors
}
#IvectorExtract

glpk_dir=exp/glpk
test_glpkIlpTemplate(){
   mkdir -p $glpk_dir; rm -rf $glpk_dir/*; mkdir -p exp/segment.true

   labelToSegment ark:local/label.ark exp/segment.true	

   sid/generate_ILP_template.sh --nj 1 exp/extractor_1024 data/toy exp/segment.true/segments.scp $glpk_dir
   #ivector_feats="ark:exp/test_seg_ivector/ivector.1.ark"		
   #writeTemplateILP "$ivector_feats" $glpk_dir/glp.template
   #glpsol --lp $glpk_dir/glp.template -o $glpk_dir/glp.sol
}
test_glpkIlpTemplate

rttm_dir=exp/rttm
rttm_dir=exp/rttm
test_DER(){
   mkdir -p $rttm_dir; rm -f $rttm_dir/*

   labelToRTTM ark:local/label.ark $rttm_dir/rttm.true	
   glpkToRTTM $glpk_dir/glp.sol ark:local/label.ark $rttm_dir/rttm.est
   perl local/md-eval-v21.pl -r $rttm_dir/rttm.true -s $rttm_dir/rttm.est	
}
#test_DER
