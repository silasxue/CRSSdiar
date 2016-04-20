// diarbin/ivectorTest.cc

#include <glpk.h>
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

    if (po.NumArgs() != 1) {
        po.PrintUsage();
        exit(1);
    }

    std::string ivector_rspecifier = po.GetArg(1),

    SequentialDoubleVectorReader ivector_reader(ivector_rspecifier);

    // create ilpCluster object
    ilpCluster ilpObj;

    // collect i-vectors from speech segments
    std::vector< Vector<double> > ivectorCollect;
    std::vector<std::string> ivectorKeyList;
    for (; !ivector_reader.Done(); ivector_reader.Next()) {
        string ivectorKey = ivector_reader.Key();   
        ivectorKeyList.push_back(ivectorKey);
        ivectorCollect.push_back(ivector_reader.Value());
    }

    // generate distant matrix from i-vectors
    Matrix<BaseFloat> distMatrix;
    ilpObj.computIvectorDistMatrix(ivectorCollect, distMatrix, ivectorKeyList);

    // Generate glpk format ILP problem representation
    vector<string> iplProblem;
    ilpObj.glpkIlpProblem(distMatrix, ilpProblem);

}