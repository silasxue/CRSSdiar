// diarbin/ivectorTest.cc

#include <vector>
#include <string>
#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "ivector/ivector-extractor.h"
#include "diar/ilp.h"
#include "diar/diar-utils.h"
#include "gmm/am-diag-gmm.h"
#include "hmm/posterior.h"


int main(int argc, char *argv[]) {
	typedef kaldi::int32 int32;
	using namespace kaldi;

	const char *usage = "Ivector Test \n";

    kaldi::ParseOptions po(usage);
	po.Read(argc, argv);

	if (po.NumArgs() != 2) {
        po.PrintUsage();
        exit(1);
    }

    std::string ivector_rspecifier = po.GetArg(1),
    	        label_rspecifier = po.GetArg(2);

    SequentialBaseFloatVectorReader label_reader(label_rspecifier);
    RandomAccessDoubleVectorReader ivector_reader(ivector_rspecifier);


    BaseFloat TrueScore=0.0;
    int32 TureCount=0;
    BaseFloat FalseScore=0.0;
    int32 FalseCount=0;
    size_t loopMax = 500;
    for (; !label_reader.Done(); label_reader.Next()) {

        Diarization diarObj;
        IlpCluster ilpObj;

        segType inputSegments;
        diarObj.LabelsToSegments(label_reader.Value(), inputSegments);

        for (size_t i=0; i<loopMax;i++){
            for (size_t j=0; j<loopMax;j++){

                string jLabel = inputSegments[j].first;
                string jSegmentKey;
                ilpObj.makeSegKey( inputSegments[j].second, label_reader.Key(), jSegmentKey );

                string iLabel = inputSegments[i].first;
                string iSegmentKey;
                ilpObj.makeSegKey( inputSegments[i].second, label_reader.Key(), iSegmentKey );

                if (i != j && (iLabel == jLabel) && iLabel != "nonspeech" && iLabel != "overlap" && jLabel != "nonspeech" && jLabel != "overlap") {

                    const Vector<double> &iIvector = ivector_reader.Value(iSegmentKey);
                    const Vector<double> &jIvector = ivector_reader.Value(jSegmentKey);
                    BaseFloat dot_prod = VecVec(iIvector, jIvector);
                    TrueScore += dot_prod; TureCount++;
                    //KALDI_LOG << "TRUE Target: " << iSegmentKey << " vs " << jSegmentKey << ":" << dot_prod;
                }

                if (i != j && iLabel != jLabel && iLabel != "nonspeech" && iLabel != "overlap" && jLabel != "nonspeech" && jLabel != "overlap") {

                    const Vector<double> &iIvector = ivector_reader.Value(iSegmentKey);
                    const Vector<double> &jIvector = ivector_reader.Value(jSegmentKey);
                    BaseFloat dot_prod = VecVec(iIvector, jIvector);
                    FalseScore += dot_prod; FalseCount++;
                    //KALDI_LOG << "FALSE ERROR: " << iSegmentKey << " vs " << jSegmentKey << ":" << dot_prod;
                }

            }

        }
    }

    KALDI_LOG << "Total Sum Of TRUE Target Score: " << TrueScore/TureCount;
    KALDI_LOG << "Total Sum Of False Detection Score: " << FalseScore/FalseCount;
    KALDI_LOG << "Count of TRUE Target: " << TureCount;
    KALDI_LOG << "Count Of FALSE Target: " << FalseCount;
}