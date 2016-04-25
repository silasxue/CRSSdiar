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

// The ILP clustering approach implemented in this file uses refers to the paper
// [1] "Recent Improvements on ILP-based Clustering for Broadcast news speaker diarization",
// by Gregor Dupuy, Sylvain Meignier, Paul Deleglise, Yannic Esteve

typedef kaldi::int32 int32;

class IlpCluster {
public:

	IlpCluster() { 
		_delta  = 50;
	}

	// generate ILP problem description in CPLEX LP format
	void glpkIlpProblem(const Matrix<BaseFloat>& , std::vector<std::string>&);

	// compute distant matrix from i-vector collections, return distant matrix, and list of corresponding keys of ivectors
	void computIvectorDistMatrix(const std::vector< Vector<double> >&, Matrix<BaseFloat>&, std::vector<std::string>&);

	// compute the Mahalanobis distance between two i-vectors
	BaseFloat ivectorMahalanobisDistance(const Vector<double>& ivec1, const Vector<double>& ivec2, const SpMatrix<double>& totalCov);

	// compute the cosine distance between two i-vectors
	BaseFloat ivectorCosineDistance(const Vector<double>& , const Vector<double>& );

	// write objective function of ILP in glpk format, refer to equation (2) in the paper [1]
	std::string problemMinimize(const Matrix<BaseFloat>& );

	// write constraint function for unique center assigment as in equation (2.3) in the paper[1]
	void problemConstraintsColumnSum(const Matrix<BaseFloat>&, std::vector<std::string>&);

	//  write constraint function as in equation (2.4) in the paper[1]
	void problemConstraintsCenter(const Matrix<BaseFloat>&, std::vector<std::string>&);

	// list all binary variables as in equation (2.2) in the paper [1]
	void listBinaryVariables(const Matrix<BaseFloat>, std::vector<std::string>&);

	// generate variable names represent ILP problem in glpk format
	std::string indexToVarName( std::string, int32, int32);

	// generate variable names represent ILP problem in glpk format
	std::vector<int32> varNameToIndex( std::string var);

	// write template into filse
	void Write(std::string outName, const std::vector<std::string>& ilpProblem);

private:
	BaseFloat _delta;

};

}


#endif 