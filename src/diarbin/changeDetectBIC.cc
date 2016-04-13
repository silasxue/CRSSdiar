// diarbin/changeDetectBIC.cc

#include "util/common-utils.h"
#include "diar/diar-utils.h"

int main(int argc, char *argv[]) {
	typedef kaldi::int32 int32;
	
	using namespace kaldi;

	const char *usage = "Detect Change Point In Speech Segments With BIC \n";

	po.Read(argc, argv);

	if (po.NumArgs() != 3) {
        po.PrintUsage();
        exit(1);
    }

    std::string feature_rspecifier = po.GetArg(1),
    	        label_rspecifier = po.GetArg(2),
    	        label_wspecifier = po.GetArg(3);


    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
    SequentialBaseFloatMatrixReader label_reader(label_rspecifier);
    SequentialBaseFloatMatrixWriter label_writter(label_wspecifier);

    for (; !feature_reader.Done(); feature_reader.Next()) {

    	std::string key = feature_reader.Key();

    	if(label_reader.Key() != key){
    		KALDI_ERROR << "Feature and label mismatch";
    	}

    	segType segments;

    	LabelsToSegments(label_reader.Value(), &segments);

    	segType bicSegments;

    	for(int i=0; i<segments.size(); i++){

    		if (segments[i].first == 'nonspeech'){
    			bicSegments.push_back(std::make_pair("nonspeech",segments[i]));
    		} else if (segments[i].first == 'speech'){
    			BicSegmentation(segments[i], &bicSegments);    			
    		} else{
    			KALDI_ERROR << "Unknown label. Only speech/nonspeech are acceptable.";
    		}
    	}

    	SegmentsToLabels(&bicSegments, &label_writter);

    }

}