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

    const char *usage = "Obtain glp ILP problem representation template \n";

    kaldi::ParseOptions po(usage);
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
        string uttid = line;
        Segments uttSegments;
        uttSegments.Read(segments_scpfile);
        uttSegments.GetSpeechSegments();
        uttSegments.ExtractIvectors(feature_reader.Value(uttSegments.GetUttID()), posterior_reader.Value(uttSegments.GetUttID()), extractor);
        for (size_t i = 0; i<uttSegments.Size(); i++) {
            ivectorCollect.push_back(uttSegments.GetIvector(i));
        }
    }

    KALDI_LOG << ivectorCollect.size();


    // // collect i-vectors from speech segments
    // for (; !ivector_reader.Done(); ivector_reader.Next()) {
    //     string ivectorKey = ivector_reader.Key();   
    //     ivectorKeyList.push_back(ivectorKey);
    //     ivectorCollect.push_back(ivector_reader.Value());
    // }

    // // generate distant matrix from i-vectors
    // Matrix<BaseFloat> distMatrix;
    // ilpObj.computIvectorDistMatrix(ivectorCollect, distMatrix, ivectorKeyList);

    // // Generate glpk format ILP problem representation
    // std::vector<std::string> ilpProblem;
    // ilpObj.glpkIlpProblem(distMatrix, ilpProblem);

    // // Write glpk format ILP problem template to text file
    // {
    //     ilpObj.Write(ilpTemplateOutfile, ilpProblem);
    // }

    // KALDI_LOG << "Written ILP optimization problem template to " << ilpTemplateOutfile;
}