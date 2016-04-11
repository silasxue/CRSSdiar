// diarbin/changeDetectBIC.cc

#include "util/common-utils.h"


int main(int argc, char *argv[]) {
	typedef kaldi::int32 int32;
	typedef std::vector< <std::string, std::vector<int> > > segType;
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

    for (; !feature_reader.Done(); feature_reader.Next()) {

    	std::string key = feature_reader.Key();

    	if(label_reader.Key() != key){
    		KALDI_ERROR << "Feature and label mismatch";
    	}

    	segType segments;

    	label2segments(label_reader.Value(), &segments);

    }


}