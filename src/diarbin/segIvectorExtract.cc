// diarbin/segIvectorExtract.cc

#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "gmm/am-diag-gmm.h"
#include "ivector/ivector-extractor.h"
#include "diar/ilp.h"
#include "diar/diar-utils.h"
#include "hmm/posterior.h"

int main(int argc, char *argv[]) {
	typedef kaldi::int32 int32;
	using namespace kaldi;

	const char *usage = "Ivector Clustering Using ILP \n";

    kaldi::ParseOptions po(usage);
	po.Read(argc, argv);

	if (po.NumArgs() != 5) {
        po.PrintUsage();
        exit(1);
    }

    std::string feature_rspecifier = po.GetArg(1),
    	        label_rspecifier = po.GetArg(2),
    	        posterior_rspecifier = po.GetArg(3),
    	        ivector_extractor_rxfilename = po.GetArg(4),
    	        ivector_wspecifier = po.GetArg(5);

    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
    SequentialBaseFloatVectorReader label_reader(label_rspecifier);
    RandomAccessPosteriorReader posterior_reader(posterior_rspecifier);
    DoubleVectorWriter ivector_writer(ivector_wspecifier);

    IvectorExtractor extractor;
	ReadKaldiObject(ivector_extractor_rxfilename, &extractor);

	for (; !feature_reader.Done(); feature_reader.Next()) {

		std::string key = feature_reader.Key();

		if(label_reader.Key() != key){
		        KALDI_ERR << "Feature and label mismatch";
		}

	 	Diarization diarObj;
		IlpCluster ilpObj;

		const Matrix<BaseFloat> &feats = feature_reader.Value();
		Posterior posterior = posterior_reader.Value(key);   

		// convert labels into segemt format
		segType inputSegments;
		diarObj.LabelsToSegments(label_reader.Value(), inputSegments);

		// extract i-vectors for all segments of given utterance, save them into a matrix 
		ilpObj.ExtractSegmentIvectors(feats, inputSegments, posterior, extractor, ivector_writer, key);

		label_reader.Next();

	}
}