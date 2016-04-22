#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <math.h>
#include "diar-utils.h"
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


		// Calculate total mean and covariance:
		Vector<double> ivectorMean;
		getMean(ivectorCollect, ivectorMean);
		SpMatrix<double> ivectorCovariance = getCovariance(ivectorCollect,ivectorMean);

		for (size_t i=0; i<ivectorCollect.size();i++){
			for (size_t j=0;j<ivectorCollect.size();j++){
				if (i == j){
					distMatrix(i,j) = 0;
				}else{
					distMatrix(i,j) = ivectorMahalanobisDistance(ivectorCollect[i], ivectorCollect[j], ivectorCovariance);
					//distMatrix(i,j) = 1 - ivectorCosineDistance(ivectorCollect[i], ivectorCollect[j], totalCov);
				}
			}
		}

	}


	SpMatrix<double> IlpCluster::getCovariance(const std::vector< Vector<double> >& ivectorCollect, const Vector<double>& totalMean) {
		// Convert ivector collection vector into sparse matrix (Because we use SpMatrix methods).
		size_t N = ivectorCollect.size();
		int32 dim = ivectorCollect[0].Dim(); // doesn't matter which ivectorCollect[i] we use.
		Matrix<double> ivectorCollectionMatrix(N,dim);
		for (size_t i = 0; i < N; i++) {
			ivectorCollectionMatrix.CopyRowFromVec(ivectorCollect[i], i);
		}
		ivectorCollectionMatrix.AddVecToRows(-1., totalMean);		
		SpMatrix<double> totalCov(dim);
		totalCov.AddMat2(1.0/N, ivectorCollectionMatrix, kTrans, 1.0);
		return totalCov;
	}


	void IlpCluster::getMean(const std::vector< Vector<double> >& ivectorCollect, Vector<double>& totalMean) {
		size_t N = ivectorCollect.size();
		int32 dim = ivectorCollect[0].Dim(); // doesn't matter which ivectorCollect[i] we use.
		totalMean.Resize(dim);
		totalMean.SetZero();
		for (size_t i = 0; i < N; i++) {
			totalMean.AddVec(1./N, ivectorCollect[i]);
		}		
	}


	BaseFloat IlpCluster::ivectorMahalanobisDistance(const Vector<double>& ivec1, const Vector<double>& ivec2, const SpMatrix<double>& totalCov) {

		Vector<double> iv1(ivec1.Dim());
		iv1.CopyFromVec(ivec1);
		Vector<double> iv2(ivec2.Dim());
		iv2.CopyFromVec(ivec2);
		SpMatrix<double> Sigma(ivec2.Dim());
		Sigma.CopyFromSp(totalCov);
		Sigma.Invert();
		iv1.AddVec(-1.,iv2);

		// Now, calculate the quadratic term: (iv1 - iv2)^T Sigma (iv1-iv2)
		Vector<double> S_iv1(iv1.Dim());
   		S_iv1.AddSpVec(1.0, Sigma, iv1, 0.0);
   		return sqrt(VecVec(iv1, S_iv1));
	}


	BaseFloat IlpCluster::ivectorCosineDistance(const Vector<double>& ivec1, const Vector<double>& ivec2) {
		 BaseFloat dotProduct = VecVec(ivec1, ivec2);
		 BaseFloat norm1 = VecVec(ivec1, ivec1) + FLT_EPSILON;
		 BaseFloat norm2 = VecVec(ivec2, ivec2) + FLT_EPSILON;

		 return dotProduct / (sqrt(norm1)*sqrt(norm2));  
	}


	void IlpCluster::glpkIlpProblem(const Matrix<BaseFloat>& distMatrix, std::vector<std::string>& ilpProblem) {

		ilpProblem.push_back("Minimize");
		ilpProblem.push_back(problemMinimize(distMatrix));
		ilpProblem.push_back("Subject To");
		problemConstraintsColumnSum(distMatrix, ilpProblem);
		problemConstraintsCenter(distMatrix, ilpProblem);
		ilpProblem.push_back("Binary");
		listBinaryVariables(distMatrix, ilpProblem);
		ilpProblem.push_back("End");
	}

	std::string IlpCluster::problemMinimize(const Matrix<BaseFloat>& distMatrix) {

		std::string objective = "problem : " + indexToVarName("x",0,0);
		for (size_t i = 1; i < distMatrix.NumRows(); i++) {
			objective += " + " + indexToVarName("x",i,i);
		}

		for (size_t i = 0; i < distMatrix.NumRows(); i++) {
			for (size_t j = 0; j < distMatrix.NumRows(); j++) {
				if (i != j) {
					BaseFloat d = distMatrix(i, j) / delta;
					if ((d > 0) && (d <= 1)) {
						objective += " + " + numberToStr(d) + " " + indexToVarName("x",i,j);
					}
				} 
			}
		}

		return objective;
	}

	void IlpCluster::problemConstraintsColumnSum(const Matrix<BaseFloat>& distMatrix, std::vector<std::string>& ilpProblem) {
		for (size_t i = 0; i < distMatrix.NumRows(); i++) {
			std::string constraint = "C" + numberToStr(i) + ": " + indexToVarName("x",i,i);
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

	void IlpCluster::problemConstraintsCenter(const Matrix<BaseFloat>& distMatrix, std::vector<std::string>& ilpProblem) {
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

	void IlpCluster::listBinaryVariables(const Matrix<BaseFloat> distMatrix, std::vector<std::string>& ilpProblem) {
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

	    return prefix + "_" + numberToStr(i) + "_" + numberToStr(j);
	}


	void IlpCluster::Write(std::string outName, const std::vector<std::string>& ilpProblem){

		std::ofstream fout;
		fout.open(outName.c_str());
		for (size_t i =0; i<ilpProblem.size(); i++){
			fout << ilpProblem[i] << "\n";
		}

		fout.close();

	}
}