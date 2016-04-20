#include <iostream>
#include <sstream>
#include <string>
#include "diar-utils.h"


namespace kaldi {

bool Diarization::BicSegmentation(std::vector<int32>& segment, const Matrix<BaseFloat>& feats, segType& bicsegments) {
	// Implement the pseudocode suggested by Cettolo and Vescovi 
	// (in Efficient audio seg. algo. based on the BIC). 

	int32 endStream = segment[1];
	int32 startStream = segment[0];
	int32 endOfDetectedSegment = segment[0];
	int32 startOfDetectedSegment = segment[0];
	int32 segmentLength = endStream - startStream + 1;

	if (Nmin >= segmentLength) {
		bicsegments.push_back(std::make_pair("1",segment));
		return false;
	}

	std::vector<int32> window = initWindow(startStream,Nmin);
	std::pair<int32, BaseFloat> maxBICIndexValue;
	while (window[1] <= endStream) {
		maxBICIndexValue = computeBIC(window, feats, lowResolution);

		while (maxBICIndexValue.second <= 0 && ((window[1] - window[0]) < Nmax) && window[1] <= endStream) {
			growWindow(window,Ngrow);
			maxBICIndexValue = computeBIC(window, feats, lowResolution);
		}

		while (maxBICIndexValue.second <= 0 && window[1] <= endStream) {
			shiftWindow(window, Nshift);
			maxBICIndexValue = computeBIC(window, feats, lowResolution);
		}

		if (maxBICIndexValue.second > 0  && window[1] <= endStream) {
			centerWindow(window, maxBICIndexValue.first, Nsecond);
			std::pair<int32,BaseFloat> maxBICIndexValueHighRes = computeBIC(window, feats, highResolution);
			if (maxBICIndexValueHighRes.second > 0) {
				endOfDetectedSegment = maxBICIndexValueHighRes.first;
				std::vector<int32> detectedSegment;
				detectedSegment.push_back(startOfDetectedSegment);
				detectedSegment.push_back(endOfDetectedSegment);
				bicsegments.push_back(std::make_pair("1",detectedSegment));
				startOfDetectedSegment = endOfDetectedSegment + 1;
				window = initWindow(startOfDetectedSegment, Nmin);
			} else {
				window = initWindow(maxBICIndexValue.first - Nmargin + 1, Nmin);
			}
		}
	}
	std::vector<int32> lastSegment;
	lastSegment.push_back(startOfDetectedSegment);
	lastSegment.push_back(endStream);
	bicsegments.push_back(std::make_pair("1",lastSegment));
	return true;
}

std::pair<int32, BaseFloat> Diarization::computeBIC(const std::vector<int32>& win, const Matrix<BaseFloat>& features, int32 resolution) {
	std::vector<std::pair<int32, BaseFloat> > deltaBIC;
	int32 N = win[1] - win[0];
	int32 d = features.NumCols(); // d: feature dimension 
	BaseFloat P = 0.5*(d + 0.5*(d*(d+1.)))*log(N);
	Matrix<BaseFloat> segmentFeatures(N, d);
	segmentFeatures.CopyFromMat(features.Range(win[0], N, 0, d));

	BaseFloat sigma = detCovariance(segmentFeatures);
	int32 idx = Nmargin;
	for (size_t i = win[0] + Nmargin; i < win[1] - Nmargin; i = i + resolution) {
		Matrix<BaseFloat> feat1(i - win[0], d);
		feat1.CopyFromMat(features.Range(win[0], i - win[0], 0, d));
		Matrix<BaseFloat> feat2(win[1] - i, d);
		feat2.CopyFromMat(features.Range(i, win[1] - i, 0, d));
		BaseFloat sigma1 = detCovariance(feat1);
		BaseFloat sigma2 = detCovariance(feat2);
		int32 location = i;
		deltaBIC.push_back(std::make_pair(location, 0.5*(N*(sigma) - idx*(sigma1) - (N - idx)*(sigma2)) - lambda*P));
		idx += resolution;
	}

	// find maximum deltaBIC:
	std::pair<int32, BaseFloat> bicOutput = deltaBIC[0];
	for (size_t i = 1; i < deltaBIC.size(); i++) {
		if (deltaBIC[i].second > bicOutput.second) {
			bicOutput = deltaBIC[i];
		}
	}

	return bicOutput;
}

BaseFloat Diarization::detCovariance(Matrix<BaseFloat>& data) {
	// Calculates the covariance of data and returns its 
	// determinant, assuming a diagonal covariance matrix.
	int32 numFrames = data.NumRows();
	int32 dim = data.NumCols();

	Vector<BaseFloat> meanVec(dim), covVec(dim); 
	for (size_t i = 0; i < numFrames; i++) {
		meanVec.AddVec(1./numFrames,data.Row(i));
		covVec.AddVec2(1./numFrames,data.Row(i));
	}
	covVec.AddVec2(-1.0, meanVec);
	BaseFloat logCovDet = 0;
	for (size_t i = 0; i < dim; i++) {
		logCovDet += log(covVec(i));
	}
	return logCovDet;
}

std::vector<int32> Diarization::initWindow(int32 start, int32 length) {
	std::vector<int32> win;
	win.push_back(start);
	win.push_back(start+length);
	return win;
}

void Diarization::growWindow(std::vector<int32>& win, int32 N) {
	win[1] = win[1] + N;
}

void Diarization::shiftWindow(std::vector<int32>& win, int32 N) {
	win[0] = win[0] + N;
	win[1] = win[1] + N;
}

void Diarization::centerWindow(std::vector<int32>& win, int32 center, int32 N) {
	if (N % 2 == 0) {
		win[0] = center - N/2;
		win[1] = center + N/2;
	} else {
		win[0] = center - N/2;
		win[1] = center + N/2 + 1;
	}
}

void Diarization::LabelsToSegments(const Vector<BaseFloat>& labels, segType& segments) {
	// NOTE: The rules of input label is as follow
	// -1 -> overlap
	// 0 -> nonspeech
	// 1,2,..,n -> speaker1, speaker2, speakern   
	// Other conditions may be added in the future. 

	int state;
	int prevState;
	std::vector<int32> segmentStartEnd;
	int startSeg = 0;
	int endSeg = 0;
	segmentStartEnd.push_back(startSeg);
	segmentStartEnd.push_back(endSeg);
	for (size_t i=1; i<labels.Dim(); i++) {
		state = labels(i);
		prevState = labels(i-1);
		if (state != prevState || i==labels.Dim()-1) {
			if (i==labels.Dim()-1) {
				i++;
			}

			if (prevState == -1) {
				segmentStartEnd[1] = i-1;
				segments.push_back(std::make_pair("overlap",segmentStartEnd));
				segmentStartEnd[0] = i;
			}else if (prevState == 0) {
				segmentStartEnd[1] = i-1;
				segments.push_back(std::make_pair("nonspeech",segmentStartEnd));
				segmentStartEnd[0] = i;				
			}else if (prevState > 0) {

				// covert int to string
				std::string stateStr;
				std::stringstream tmp; 
				tmp << prevState;
				stateStr = tmp.str();

				segmentStartEnd[1] = i-1;
				segments.push_back(std::make_pair(stateStr,segmentStartEnd));
				segmentStartEnd[0] = i;
			} 
		}

	}
}


void Diarization::SegmentsToLabels(const segType& segments, Vector<BaseFloat>& labels){
	// NOTE: The rules of input label is as follow
	// -1 -> overlap
	// 0 -> nonspeech
	// 1,2,..,n -> speaker1, speaker2, speakern   
	// Other conditions may be added in the future. 
	std::vector<int32> lastSegment = segments.back().second;
	labels.Resize(lastSegment[1]+1);
	
	for (size_t i=0; i<segments.size(); i++) {
		KALDI_LOG << segments[i].first << " : " << segments[i].second[0] << " - " << segments[i].second[1];		
		std::vector<int32> segmentStartEnd = segments[i].second;
		int32 segLen = segmentStartEnd[1] - segmentStartEnd[0] + 1;
		Vector<BaseFloat> segLabels(segLen);		

		if (segments[i].first == "nonspeech") { 
			segLabels.Set(0.0);
		}else if (segments[i].first == "overlap") {
			segLabels.Set(-1.0);
		}else {
			// convert string to int
			std::istringstream tmp(segments[i].first);
			int32 stateInt;
			tmp >> stateInt;
			segLabels.Set(stateInt);
		}
		labels.Range(segmentStartEnd[0],segLen).CopyFromVec(segLabels);
	}
}


}