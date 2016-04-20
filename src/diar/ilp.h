#ifndef KALDI_IVECTOR_DIAR_ILP_H_
#define KALDI_IVECTOR_DIAR_ILP_H_

#include <vector>
#include <string>
#include "base/kaldi-common.h"
#include "matrix/matrix-lib.h"
#include "util/common-utils.h"
#include "ivector/ivector-extractor.h"
#include "gmm/am-diag-gmm.h"
#include "hmm/posterior.h"
#include "diar-utils.h"

namespace kaldi{
typedef kaldi::int32 int32;
BaseFloat eps = 1.0e-40;

class IlpCluster {
public:

	// extract ivectors for all nonspeech segments
	void ExtractSegmentIvectors(const Matrix<BaseFloat>&, const segType& , const Posterior& , const IvectorExtractor&, DoubleVectorWriter&, const std::string);
	
	// extract ivector for a segment
	void GetSegmentIvector(const Matrix<BaseFloat>&, const Posterior&, const IvectorExtractor&, DoubleVectorWriter&, const std::string, const std::vector<int32>&);

	// create unique key of given segment, such that the key is format of "uttid_segStartFrame_segEndFrame"
	void makeSegKey( const std::vector<int32>& segStartEnd, const std::string uttid, std::string& segIvectorKey );

	// generate ILP problem description in CPLEX LP format
	void glpkIlpProblem(const Matrix<BaseFloat>& , std::vector<std::string>&);

	// compute distant matrix from i-vector collections, return distant matrix, and list of corresponding keys of ivectors
	void computIvectorDistMatrix(const std::vector< Vector<double> >&, Matrix<BaseFloat>&, std::vector<std::string>&); 

	// compute the Mahalanobis distance between two i-vectors
	// double ivectorMahalanobisDistance(Vector<double>& , Vector<double>& );

	// compute the cosine distance between two i-vectors
	double ivectorCosineDistance(Vector<double>& , Vector<double>& );

	BaseFloat delta = 0.5;
};

}


#endif 