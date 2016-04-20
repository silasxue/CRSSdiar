#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <math.h>
#include "ilp.h"


namespace kaldi{

	void IlpCluster::ExtractSegmentIvectors(const Matrix<BaseFloat>& feats, 
								const segType& inputSegments, 
								const Posterior& posterior, 
								const IvectorExtractor& extractor, 
								DoubleVectorWriter& ivectorWriter,
								const std::string key){

		int32 featsDim = extractor.FeatDim();
		size_t numSegs = inputSegments.size();

		int32 numSpeechSeg = 0;
		for (size_t i=0; i<numSegs; i++){

			std::string segmentLabel = inputSegments[i].first;

			if (segmentLabel != "nonspeech" && segmentLabel != "overlap"){

				numSpeechSeg++;

				std::vector<int32> segmentStartEnd = inputSegments[i].second;
				Matrix<BaseFloat> segFeats(segmentStartEnd[1] - segmentStartEnd[0] +1, featsDim);
				segFeats.CopyFromMat(feats.Range(segmentStartEnd[0], segmentStartEnd[1] - segmentStartEnd[0] +1, 0, featsDim ));

				Posterior::const_iterator startIter = posterior.begin() + segmentStartEnd[0];
				Posterior::const_iterator endIter = posterior.begin() + segmentStartEnd[1]+1;
				Posterior segPosterior(startIter, endIter);

				KALDI_LOG << " Segment Range : segmentStartEnd[0]" << " <-> " << segmentStartEnd[1] << " The seg size is: " << segPosterior.size();

				GetSegmentIvector(segFeats, segPosterior, extractor, ivectorWriter, key, segmentStartEnd);

			}

		}
	}


	void IlpCluster::GetSegmentIvector(const Matrix<BaseFloat>& segFeats, 
						   const Posterior& segPosterior, 
						   const IvectorExtractor& extractor, 
						   DoubleVectorWriter& ivectorWriter,
						   std::string uttid, 
						   const std::vector<int32>& segmentStartEnd) {


		Vector<double> ivector;

	    bool need_2nd_order_stats = false;
	    
	    IvectorExtractorUtteranceStats utt_stats(extractor.NumGauss(),
	                                             extractor.FeatDim(),
	                                             need_2nd_order_stats);
	      
	    utt_stats.AccStats(segFeats, segPosterior);
	    
	    ivector.Resize(extractor.IvectorDim());
	    ivector(0) = extractor.PriorOffset();

	    extractor.GetIvectorDistribution(utt_stats, &ivector, NULL);

	    std::string key;
	    makeSegKey(segmentStartEnd, uttid, key);

		ivectorWriter.Write(key, ivector);
	}


	void IlpCluster::makeSegKey( const std::vector<int32>& segmentStartEnd, const std::string uttid, std::string& segIvectorKey) { 
		// Make unique key for each segment of each utterance, by concatenating uttid with segment start and end
		// Such that the key is format of "uttid_segStartFrame_segEndFrame".

	    std::string segStartEndString;
	    std::stringstream tmp; 
	    tmp << segmentStartEnd[0];
	    tmp << "_";
	    tmp << segmentStartEnd[1];
	    segStartEndString = tmp.str();

	    segIvectorKey = uttid + "_" + segStartEndString;       
	}

	void IlpCluster::computIvectorDistMatrix(const std::vector< Vector<double> >& ivectorCollect, Matrix<BaseFloat>& distMatrix, std::vector<string>& ivectorKeyList) {

		distMatrix.Resize(ivectorCollect.size(),ivectorCollect.size());
		for (size_t i=0; i<ivectorCollect.size();i++){
			for (size_t j=0;j<ivectorCollect.size();j++){
				if (i == j){
					distMatrix(i,j) = 0;
				}else{
					// distmatrix(i,j) = ivectorMahalanobisDistance(ivectorCollect[i], ivectorCollect[j]);
					distmatrix(i,j) = 1 - ivectorCosineDistance(ivectorCollect[i], ivectorCollect[j]);
				}
			}
		}

	}

	BaseFloat IlpCluster::ivectorCosineDistance(Vector<double>& ivec1, Vector<double>& ivec2) {
		 BaseFloat dotProduct = VecVec(ivec1, ivec2);
		 BaseFloat norm1 = VecVec(ivec1, ivec1) + eps;
		 BaseFloat norm2 = VecVec(ivec2, ivec2) + eps;

		 return dotProduct / (sqrt(norm1)*sqrt(norm2));  
	}

	void IlpCluster::glpkIlpProblem(const Matrix<BaseFloat>& distMatrix, std::vector<std::string>& ilpProblem) {

		ilpProblem.push_back("Minimize");
		ilpProblem.push_back(problemMinimize(distmatrix));
		ilpProblem.push_back("Subject To");
		problemConstraintsColumnSum(distMatrix, ilpProblem);
		problemConstraintsCenter(distmatrix, ilpProblem);
		ilpProblem.push_back("Binary");
		listBinaryVariables(distmatrix, ilpProblem);
		ilpProblem.push_back("End");
	}

	std::string IlpCluster::problemMinimize(const Matrix<BaseFloat>& distMatrix) {

		std::string objective = "problem : " + indexToVarName("x",0,0);
		for (size_t i = 1; i < distMatrix.NumRows(); i++) {
			objective += " + " + indexToVarName("x",i,i);
		}

		for (size_t i = 0; i < distmatrix.NumRows(); i++) {
			for (size_t j = 0; j < distmatrix.NumRows(); j++) {
				if (i != j) {
					BaseFloat d = distMatrix(i, j) / delta;
					if ((d > 0) && (d <= 1)) {
						objective += " + " + d + " " + indexToVarName("x",i,j);
					}
				} 
			}
		}

		return objective;
	}

	void IlpCluster::problemConstraintsColumnSum(const Matrix<BaseFloat>& distMatrix, std::vector<std::string>& ilpProblem) {
		for (size_t i = 0; i < distMatrix.NumRows(); i++) {
			std::string constraint = "C" + i + ": " + indexToVarName("x",i,i);
			for (size_t j = 0; j < distMatrix.NumRows(); j++) {
				if (i != j) {
					BaseFloat d = distMatrix(i, j);
					if (d <= delta) {
						constraint += " + " + indexToVarName("x",i,j);
					}
				}
			}
			constraint += " = 1 ";
			ilpProblem.push_back(constraint);
		}
	}

	void IlpCluster::problemConstraintsCenter(Matrix<BaseFloat>& distMatrix, std::vector<std::string>& ilpProblem) {
		for (size_t i = 0; i < distMatrix.NumRows(); i++) {
			for (size_t j = 0; j < distMatrix.NumRows(); j++) {
				if (i != j) {
					BaseFloat d = distMatrix(i, j);
					if (d <= delta) {
						ilpProblem.push_back(indexToVarName("x",i,j) + " - " + indexToVarName("x",j,j) + " <= 0");
					}
				}
			}
		}
	}

	void IlpCluster::listBinaryVariables(Matrix<BaseFloat> distMatrix, std::vector<std::string>& ilpProblem) {
		for (size_t i = 0; i < distMatrix.NumRows(); i++) {
			for (size_t j = 0; j < distMatrix.NumRows(); j++) {
				BaseFloat d = distMatrix(i,j);
				if (d <= delta) {
					ilpProblem.push_back(indexToVarName("x",i,j));
				}
			}
		}
	}

	std::string IlpCluster::indexToVarName( std::string prefix, int32 i, int32 j) { 

	    std::string varName;
	    std::stringstream tmp;
	    tmp << prefix;
	    tmp << "_"; 
	    tmp << i;
	    tmp << "_";
	    tmp << j;
	    varName = tmp.str();

	    return varName;
	}


}