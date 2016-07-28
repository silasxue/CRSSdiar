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

	if (po.NumArgs() != 6) {
        po.PrintUsage();
        exit(1);
    }

    std::string label_rspecifier = po.GetArg(1),
                feature_rspecifier = po.GetArg(2),
                posterior_rspecifier = po.GetArg(3),
                ivector_extractor_rxfilename = po.GetArg(4),
                background_ivectors_rspecifier = po.GetArg(5),
                utt2spk_rspecifier = po.GetArg(6);

    SequentialBaseFloatVectorReader label_reader(label_rspecifier);
    SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
    RandomAccessPosteriorReader posterior_reader(posterior_rspecifier);
    SequentialDoubleVectorReader ivector_reader(background_ivectors_rspecifier);
    IvectorExtractor extractor;
    ReadKaldiObject(ivector_extractor_rxfilename, &extractor);

    // read Background i-vectors:
    SequentialTokenReader utt2spk_reader(utt2spk_rspecifier);
    std::map<std::string, std::string> utt2spk_map;
    for (; !utt2spk_reader.Done(); utt2spk_reader.Next()) {
        std::string utt = utt2spk_reader.Key();
        std::string spk = utt2spk_reader.Value();
        utt2spk_map[utt] = spk;
    }

    std::vector< Vector<double> > backgroundIvectors;
    std::vector<std::string> backgroundLabels;
    for (; !ivector_reader.Done(); ivector_reader.Next()) {
         std::string utt_label = ivector_reader.Key();
         Vector<double> utt_ivector = ivector_reader.Value();
         backgroundIvectors.push_back(utt_ivector); 
         backgroundLabels.push_back(utt2spk_map[utt_label]); 
    }

    SpMatrix<double> withinCovariance = computeWithinCovariance(backgroundIvectors,
                                                                backgroundLabels);

    
    BaseFloat TrueScore=0.0;
    int32 TureCount=0;
    BaseFloat FalseScore=0.0;
    int32 FalseCount=0;
    size_t loopMax = 50;

    for (; !label_reader.Done(); label_reader.Next()) {
        Segments allSegments(label_reader.Value(), label_reader.Key());
        Segments speechSegments = allSegments.GetSpeechSegments();
        speechSegments.ExtractIvectors(feature_reader.Value(),posterior_reader.Value(label_reader.Key()),extractor);
        speechSegments.NormalizeIvectors();

        std::vector< Vector<double> > ivectorCollect;
        // read in segments from each file
        for (size_t i = 0; i<speechSegments.Size(); i++) {
            ivectorCollect.push_back(speechSegments.GetIvector(i));
        }
        Vector<double> totalMean;
        computeMean(ivectorCollect, totalMean);
        SpMatrix<double> totalCov = computeCovariance(ivectorCollect, 
                                   totalMean);
        for (size_t i=0; i<loopMax;i++){
            for (size_t j=0; j<loopMax;j++){
                std::string jLabel = speechSegments.SegKey(j);
                std::string iLabel = speechSegments.SegKey(i);
                if (i != j && (iLabel == jLabel) && iLabel != "nonspeech" && iLabel != "overlap" && jLabel != "nonspeech" && jLabel != "overlap") {
                    const Vector<double> &iIvector = speechSegments.GetIvector(i);
                    const Vector<double> &jIvector = speechSegments.GetIvector(j);
                    BaseFloat dotProduct = VecVec(iIvector, jIvector);
                    //TrueScore += dotProduct; TureCount++;
                    //KALDI_LOG << "TRUE Target: " << iSegmentKey << " vs " << jSegmentKey << ":" << dot_prod;
                    //BaseFloat distance = mahalanobisDistance(iIvector, jIvector, totalCov);
                    BaseFloat distance = conditionalBayesDistance(iIvector, jIvector, withinCovariance);
                    KALDI_LOG << "TRUE Mahalanobis scores: " << distance;
                    KALDI_LOG << "TRUE Cosine scores: " << dotProduct;
                }
                if (i != j && iLabel != jLabel && iLabel != "nonspeech" && iLabel != "overlap" && jLabel != "nonspeech" && jLabel != "overlap") {
                    const Vector<double> &iIvector = speechSegments.GetIvector(i);
                    const Vector<double> &jIvector = speechSegments.GetIvector(j);
                    BaseFloat dotProduct = VecVec(iIvector, jIvector);
                    //FalseScore += dotProduct; FalseCount++;
                    //KALDI_LOG << "FALSE ERROR: " << iSegmentKey << " vs " << jSegmentKey << ":" << dot_prod;
                    //BaseFloat distance = mahalanobisDistance(iIvector, jIvector, totalCov);
                    BaseFloat distance = conditionalBayesDistance(iIvector, jIvector, withinCovariance);
                    KALDI_LOG << "FALSE Mahalanobis scores: " << distance;
                    KALDI_LOG << "FALSE Cosine scores: " << dotProduct;
                }
            }
        }
        //feature_reader.Next();
    }
    KALDI_LOG << "Total Sum Of TRUE Target Score: " << TrueScore/TureCount;
    KALDI_LOG << "Total Sum Of False Detection Score: " << FalseScore/FalseCount;
    KALDI_LOG << "Count of TRUE Target: " << TureCount;
    KALDI_LOG << "Count Of FALSE Target: " << FalseCount;
}