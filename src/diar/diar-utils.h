#ifndef KALDI_DIAR_UTILS_H_
#define KALDI_DIAR_UTILS_H_

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "util/common-utils.h"
#include "matrix/matrix-lib.h"
#include "hmm/posterior.h"
#include "gmm/am-diag-gmm.h"
#include "ivector/ivector-extractor.h"


namespace kaldi{
typedef std::pair<std::string, std::vector<int32> > segUnit;
typedef std::vector< segUnit > segType;

#ifndef FRAMESHIFT
#define FRAMESHIFT 0.01
#endif
#ifndef FRAMELENGTH
#define FRAMELENGTH 0.025
#endif

class Segments {
public:
	Segments(const Vector<BaseFloat>& labels, const std::string uttid);
	Segments();
	Segments(const std::string uttid);
	std::string SegKey(int32 index); // return speaker label
	std::vector<int32> SegStartEnd(int32 index) const;
	segUnit GetSeg(int32 index) const;
	int32 Size() const;
	segUnit End();
	segUnit Begin();
	std::string GetUttID();
	Vector<double> GetIvector(int32 index); // return ith i-vector
	void NormalizeIvectors();
	void ToLabels(Vector<BaseFloat>&);
	void ToRTTM(const std::string& uttid, const std::string& rttmName);
	Segments GetSpeechSegments();
	Segments GetLargeSegments(int32 segMin);
	void ExtractIvectors(const Matrix<BaseFloat>& feats,
						 const Posterior& posterior,
						 const IvectorExtractor& extractor);
	void GetSegmentIvector(const Matrix<BaseFloat>& segFeats, 
						   const Posterior& segPosterior, 
						   const IvectorExtractor& extractor, 
						   const std::vector<int32>& segmentStartEnd);
	void Append(segUnit& segment);
	void SetLabel(int32 index, std::string label);
	void Read(const std::string& segments_rxfilename);
	void ReadIvectors(const std::string& ivector_rxfilename); // NOTE: Improvement needed for parallel processing
	void Write(const std::string& segments_dirname);
	void WriteIvectors(const std::string& ivector_wxfilename); // NOTE: Improvement needed for parallel processing
private:
	segType _segmentList;
	std::string _uttid;
	std::vector< Vector<double> > _ivectorList; 
}; 


template<class T>
std::string numberToString(T number){
	// convert number of different types to string
    std::stringstream tmpStream;
    tmpStream << number;
    return tmpStream.str();
}


template<class T>
SpMatrix<T> computeCovariance(const std::vector< Vector<T> >& vectorOfFeatures, 
								   const Vector<T>& mean) {
	// Compute covariance (sparse matrix) of vector of features. 
	size_t N = vectorOfFeatures.size(); 
	int32 dim = vectorOfFeatures[0].Dim(); // doesn't matter which vectorOfFeatures[i] we use.
	Matrix<T> matrixOfFeatures(N,dim);
	for (size_t i = 0; i < N; i++) {
		matrixOfFeatures.CopyRowFromVec(vectorOfFeatures[i], i);
	}
	matrixOfFeatures.AddVecToRows(-1., mean);		
	SpMatrix<T> covariance(dim);
	covariance.AddMat2(1.0/N, matrixOfFeatures, kTrans, 1.0);
	return covariance;
}


template<class T>
SpMatrix<T> computeWithinCovariance(const std::vector< Vector<T> >& vectorOfFeatures,
									const std::vector<std::string>& vectorOfLabels) {
	// Calculate: W  = (1/N) Sum_{s=1}_{S} Sum_{i=1}_{Ns} (w_i - m_s)((w_i - m_s)^T)
	// inputs contain all development i-vectors and a vector of the same length
	// that contains the corresponding speaker for each i-vector.
	size_t N = vectorOfLabels.size(); 
	if (N != vectorOfFeatures.size()) {
		KALDI_ERR << "Number of labels " << N 
					<< " does NOT match number of i-vectors "
					<< vectorOfFeatures.size();  
	}

	// count number of instances for each speakers while 
	// calculating means. 
	// We don't want to use a class if we don't have 
	// enough i-vectors for the corresponding speaker.
	std::map< std::string, int > spkr_count;
	std::map< std::string, Vector<T> > spkr_means;
	for (size_t i = 0; i < N; i++) {
		if (spkr_count.find(vectorOfLabels[i])==spkr_count.end()) {
			spkr_count[vectorOfLabels[i]] = 1;
			spkr_means[vectorOfLabels[i]] = vectorOfFeatures[i];
		}else {
			spkr_count[vectorOfLabels[i]]++;
			spkr_means[vectorOfLabels[i]].AddVec(1., vectorOfFeatures[i]);
		}
	}

	int32 ivecDim = spkr_means[vectorOfLabels[0]].Dim();
	// We still need to divide the means by number of utterances for
	// each speaker.
	for (std::map<std::string,int>::iterator i = spkr_count.begin(); i != spkr_count.end(); i++) {
		Vector<T> z(ivecDim);
		z.CopyFromVec(spkr_means[i->first]);
		spkr_means[i->first].SetZero();
		spkr_means[i->first].AddVec(1./i->second, z);
	}
	
	int NminSpkrs = 0; 
	SpMatrix<T> W(ivecDim);
	W.SetZero();
	for (size_t i = 0; i < N; i++) {
		if (spkr_count[vectorOfLabels[i]] > NminSpkrs) {
			Vector<T> x(ivecDim);
			x.CopyFromVec(vectorOfFeatures[i]);
			x.AddVec(-1.,spkr_means[vectorOfLabels[i]]);
			W.AddVec2(1./N,x);
		}
	}
	return W;
}


template <class T>
T logDetCovariance(Matrix<T>& data) {
	// Calculates the covariance of data and returns its 
	// determinant, assuming a diagonal covariance matrix.
	int32 numFrames = data.NumRows();
	int32 dim = data.NumCols();

	Vector<T> meanVec(dim), covVec(dim); 
	for (size_t i = 0; i < numFrames; i++) {
		meanVec.AddVec(1./numFrames,data.Row(i));
		covVec.AddVec2(1./numFrames,data.Row(i));
	}
	covVec.AddVec2(-1.0, meanVec);
	T logCovDet = 0;
	for (size_t i = 0; i < dim; i++) {
		logCovDet += log(covVec(i));
	}
	return logCovDet;
}


template<class T>
void computeMean(const std::vector< Vector<T> >& vectorOfFeatures,
				 Vector<T>& mean) {
	// Compute mean vector of features. 
	size_t N = vectorOfFeatures.size();
	int32 dim = vectorOfFeatures[0].Dim(); // doesn't matter which vectorOfFeatures[i] we use.
	mean.Resize(dim);
	mean.SetZero();
	for (size_t i = 0; i < N; i++) {
		mean.AddVec(1./N, vectorOfFeatures[i]);
	}		
}


BaseFloat FrameIndexToSeconds(int32 frame);

template<class T>
int32 SecondsToFrameIndex(T timeStamp) {
	// Find closest frame start of given time stamp.
	return int32(timeStamp/FRAMESHIFT);
} 
 

std::string makeSegKey(const std::vector<int32>& segmentStartEnd, 
							  const std::string uttid);


std::vector<std::string>& split(const std::string& s, 
								char delim, 
								std::vector<std::string>& elems);


std::vector<std::string> split(const std::string& s, char delim);


std::vector<std::string> returnNonEmptyFields(const std::vector<std::string>& fields);

// compute distant matrix from i-vector collections, return distant matrix, and list of corresponding keys of ivectors
void computeDistanceMatrix(const std::vector< Vector<double> >& vectorList, 
							Matrix<BaseFloat>& distanceMatrix,
							const std::vector< Vector<double> >& backgroundIvectors,
							const std::vector< std::string >& backgroundIvectorLabels);


// compute the Mahalanobis distance between two i-vectors
BaseFloat mahalanobisDistance(const Vector<double>& v1, const Vector<double>& v2, const SpMatrix<double>& totalCov);

// compute the cosine distance between two i-vectors
BaseFloat cosineDistance(const Vector<double>& v1, const Vector<double>& v2);

// Mahalanobis Distance using a averaged within-class covariance (assumes homoscedasticity)
BaseFloat conditionalBayesDistance(const Vector<double>& v1, const Vector<double>& v2, const SpMatrix<double>& withinCov);

}
#endif
