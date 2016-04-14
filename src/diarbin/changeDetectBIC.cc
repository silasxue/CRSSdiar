// diarbin/changeDetectBIC.cc

#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "diar/diar-utils.h"

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
    Int32VectorWriter label_writter(label_wspecifier);

    Diarization diarObj;

    for (; !feature_reader.Done(); feature_reader.Next()) {

    	std::string key = feature_reader.Key();

    	if(label_reader.Key() != key){
    		KALDI_ERR << "Feature and label mismatch";
    	}

    	segType segments; // Speech/Nonspeech/Overlap segmentations

        //const Matrix<BaseFloat> &mat = feature_reader.Value();
        //const Matrix<BaseFloat> &labelVector = label_reader.Value();
    	diarObj.LabelsToSegments(label_reader.Value(), segments);        

    	segType bicSegments; // Segmentations after bic change detection

    	for(int i=0; i<segments.size(); i++){

    		if (segments[i].first == "nonspeech"){
    			bicSegments.push_back(std::make_pair("nonspeech",segments[i].second));
    		} else if (segments[i].first == "speech"){
    			// diarObj.BicSegmentation(segments[i].second, mat, bicSegments);    			
    		} else{
    			KALDI_ERR << "Unknown label. Only speech/nonspeech are acceptable.";
    		}
    	}

    	//SegmentsToLabels(&bicSegments, label_writter);

        label_reader.Next();

    }

}