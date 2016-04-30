// diarbin/changeDetectBIC.cc

#include <vector>
#include <iostream>
#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "diar/diar-utils.h"
#include "diar/bic.h"

int main(int argc, char *argv[]) {
	typedef kaldi::int32 int32;
	
	using namespace kaldi;

	const char *usage = "Detect Change Point In Speech Segments With BIC \n";

    kaldi::ParseOptions po(usage);
	po.Read(argc, argv);

	if (po.NumArgs() != 3) {
        po.PrintUsage();
        exit(1);
    }

    std::string feature_rspecifier = po.GetArg(1),
    	        label_rspecifier = po.GetArg(2),
    	        label_wspecifier = po.GetArg(3);

    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
    SequentialBaseFloatVectorReader label_reader(label_rspecifier);
    BaseFloatVectorWriter label_writer(label_wspecifier);

    for (; !feature_reader.Done(); feature_reader.Next()) {

    	std::string key = feature_reader.Key();
        KALDI_LOG << key;
    	if(label_reader.Key() == key){
    		//KALDI_ERR << "Feature and label mismatch";
        }    
        	
    	Segments allSegments(label_reader.Value(), key); // Speech/Nonspeech/Overlap segmentations
        Segments speechSegments = allSegments.GetSpeechSegments();
        const Matrix<BaseFloat> &mat = feature_reader.Value();

        BICOptions bicOpt;
        BIC bicObj(bicOpt);
        Segments bicSegments(key); // Segmentations after bic change detection
        bicObj.BICSegmentation(speechSegments, mat, bicSegments);    

        Vector<BaseFloat> bicLabels;
        bicSegments.ToLabels(bicLabels);

        label_writer.Write(key, bicLabels);
        label_reader.Next();

        speechSegments.ToRTTM(key, "tmp.rttm");
        bicSegments.ToRTTM(key, "tmp1.rttm");

        bicSegments.Write("tmp.seg");

        KALDI_LOG << "bic false alarms:" << bicObj.CompareSegments(speechSegments, bicSegments);

    }
}