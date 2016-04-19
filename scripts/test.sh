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

test_l2s(){

    #changeDetectBIC scp:data/toy/feats.scp scp:exp/vad/vad_toy.1.scp ark,scp,t:./tmp.ark,./tmp.scp
    changeDetectBIC scp:data/toy/feats.scp ark:local/label.ark ark,scp,t:./tmp.ark,./tmp.scp
}
#test_l2s

test_changedetection() {
    changeDetectBIC scp:data/toy/feats.scp ark:local/label.ark ark,scp,t:./tmp.ark,./tmp.scp
}
test_changedetection

testsegIvector(){
    
    #sid/test_seg_ivector.sh --nj 1 exp/extractor_2048 data/toy local/label.ark exp/test_seg_ivector
    ivector-subtract-global-mean ark:exp/test_seg_ivector/ivector.1.ark ark:- | ivector-normalize-length ark:- ark:- | ivectorTest ark:- ark:local/label.ark 	    		
    #ivector-subtract-global-mean ark:exp/test_seg_ivector/ivector.1.ark ark:- |  ivectorTest ark:- ark:local/label.ark 	    		
}
#test_segIvector




