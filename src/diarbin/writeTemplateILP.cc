// diarbin/ivectorTest.cc

#include <vector>
#include <string>
#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "ivector/ivector-extractor.h"
#include "diar/diar-utils.h"
#include "gmm/am-diag-gmm.h"
#include "hmm/posterior.h"
#include "diar/ilp.h"

int main(int argc, char *argv[]) {
    typedef kaldi::int32 int32;
    using namespace kaldi;

    const char *usage = "Obtain glp ILP problem representation template \n";

    BaseFloat delta = 30.0; 

    kaldi::ParseOptions po(usage);
    po.Register("delta", &delta, "delta parameter for ILP clustering");
    po.Read(argc, argv);

    if (po.NumArgs() != 5) {
        po.PrintUsage();
        exit(1);
    }

    std::string segments_scpfile = po.GetArg(1),
                feature_rspecifier = po.GetArg(2),
                posterior_rspecifier = po.GetArg(3),
                ivector_extractor_rxfilename = po.GetArg(4),
                ilpTemplate_wspecifier = po.GetArg(5);

    RandomAccessBaseFloatMatrixReader feature_reader(feature_rspecifier);
    RandomAccessPosteriorReader posterior_reader(posterior_rspecifier);
    IvectorExtractor extractor;
    ReadKaldiObject(ivector_extractor_rxfilename, &extractor);

    std::vector< Vector<double> > ivectorCollect;

    // read in segments from each file
    Input ki(segments_scpfile);  // no binary argment: never binary.
    std::string line;
    while (std::getline(ki.Stream(), line)) {
        Segments uttSegments;
        uttSegments.Read(line);
        Segments speechSegments = uttSegments.GetSpeechSegments();
        speechSegments.ExtractIvectors(feature_reader.Value(speechSegments.GetUttID()), posterior_reader.Value(speechSegments.GetUttID()), extractor);
        speechSegments.NormalizeIvectors();
        for (size_t i = 0; i<speechSegments.Size(); i++) {
            ivectorCollect.push_back(speechSegments.GetIvector(i));
        }
    }  

    // generate distant matrix from i-vectors
    Matrix<BaseFloat> distMatrix;
    computeDistanceMatrix(ivectorCollect, distMatrix);

    // Generate glpk format ILP problem representation
    GlpkILP ilpObj(distMatrix, delta);
    ilpObj.glpkIlpProblem();

    // Write glpk format ILP problem template to text file
    ilpObj.Write(ilpTemplate_wspecifier);
    
    KALDI_LOG << "Written ILP optimization problem template to " << ilpTemplate_wspecifier;
}