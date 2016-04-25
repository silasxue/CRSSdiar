#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include "diar-utils.h"
#include "bic.h"

namespace kaldi {


BICOptions::BICOptions(): 
	  Nmin(300),
	  Nmax(2000),
	  Nsecond(300),
	  Nshift(200),
	  Nmargin(100),
	  Ngrow(100),
	  lambda(1.25),
	  lowResolution(25),
	  highResolution(5) { 
};


void BIC::BICSegmentation(const Segments& uttSegments, 
					 const Matrix<BaseFloat>& feats, 
					 Segments& bicSegments) {
	// Pefroming BIC change detection for each segments of given utterance
	for (size_t i = 0; i < uttSegments.Size(); i++) {
		segUnit seg = uttSegments.GetSeg(i);
		SplitSegment(seg, feats, bicSegments);
	}
}


bool BIC::SplitSegment(const segUnit& origSegment,
					   const Matrix<BaseFloat>& feats, 
					   Segments& bicSegments) {
	// Perform BIC change dectect in single speech segment.
	std::vector<int32> segment = origSegment.second;
	int32 endStream = segment[1];
	int32 startStream = segment[0];
	int32 endOfDetectedSegment = segment[0];
	int32 startOfDetectedSegment = segment[0];
	int32 segmentLength = endStream - startStream + 1;
	if (this->_opts.Nmin >= segmentLength) {
		segUnit segmentTmp = std::make_pair("1",segment);
		bicSegments.Append(segmentTmp);
		return false;
	}
	Window window(startStream,this->_opts.Nmin);
	std::pair<int32, BaseFloat> maxBICIndexValue;
	while (window.End() <= endStream) {
		maxBICIndexValue = ComputeBIC(window, feats, this->_opts.lowResolution);
		while (maxBICIndexValue.second <= 0 && (window.Length() < this->_opts.Nmax) && window.End() <= endStream) {
			window.GrowWindow(this->_opts.Ngrow);
			maxBICIndexValue = ComputeBIC(window, feats, this->_opts.lowResolution);
		}
		while (maxBICIndexValue.second <= 0 && window.End() <= endStream) {
			window.ShiftWindow(this->_opts.Nshift);
			maxBICIndexValue = ComputeBIC(window, feats, this->_opts.lowResolution);
		}
		if (maxBICIndexValue.second > 0  && window.End() <= endStream) {
			window.CenterWindow(maxBICIndexValue.first, this->_opts.Nsecond);
			std::pair<int32,BaseFloat> maxBICIndexValueHighRes = ComputeBIC(window, feats, this->_opts.highResolution);
			if (maxBICIndexValueHighRes.second > 0) {
				endOfDetectedSegment = maxBICIndexValueHighRes.first;
				std::vector<int32> detectedSegment;
				detectedSegment.push_back(startOfDetectedSegment);
				detectedSegment.push_back(endOfDetectedSegment);
				segUnit detectedSegmentTmp;
				detectedSegmentTmp = std::make_pair("1",detectedSegment);
				bicSegments.Append(detectedSegmentTmp);
				startOfDetectedSegment = endOfDetectedSegment + 1;
				window.ResetWindow(startOfDetectedSegment, this->_opts.Nmin);
			} else {
				window.ResetWindow(maxBICIndexValue.first - this->_opts.Nmargin + 1, this->_opts.Nmin);
			}
		}
	}
	std::vector<int32> lastSegment;
	lastSegment.push_back(startOfDetectedSegment);
	lastSegment.push_back(endStream);
	segUnit lastSegmentTmp = std::make_pair("1",lastSegment);
	bicSegments.Append(lastSegmentTmp);
	return true;
}


std::pair<int32, BaseFloat> BIC::ComputeBIC(Window& win,
											const Matrix<BaseFloat>& features, 
											int32 resolution) {
	// Return <index, value> pair. index being the location of the maximum BIC and value being the maximum value of DeltaBIC computed
	// over window. 
	std::vector<std::pair<int32, BaseFloat> > deltaBIC;
	int32 N = win.Length();
	int32 d = features.NumCols(); // d: feature dimension 
	BaseFloat P = 0.5*(d + 0.5*(d*(d+1.)))*log(N);
	Matrix<BaseFloat> segmentFeatures(N, d);
	segmentFeatures.CopyFromMat(features.Range(win.Start(), N, 0, d));
	BaseFloat sigma = logDetCovariance(segmentFeatures);
	int32 idx = this->_opts.Nmargin;
	for (size_t i = win.Start() + this->_opts.Nmargin; i < win.End() - this->_opts.Nmargin; i = i + resolution) {
		Matrix<BaseFloat> feat1(i - win.Start(), d);
		feat1.CopyFromMat(features.Range(win.Start(), i - win.Start(), 0, d));
		Matrix<BaseFloat> feat2(win.End() - i, d);
		feat2.CopyFromMat(features.Range(i, win.End() - i, 0, d));
		BaseFloat sigma1 = logDetCovariance(feat1);
		BaseFloat sigma2 = logDetCovariance(feat2);
		int32 location = i;
		deltaBIC.push_back(std::make_pair(location, 0.5*(N*(sigma) - idx*(sigma1) - (N - idx)*(sigma2)) - this->_opts.lambda*P));
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


BaseFloat BIC::CompareSegments(const Segments& refSegments, const Segments& bicSegments) {
	int32 j = 0;
	int32 i = 0;
	int32 unwantedChanges = 0;
	while (i < bicSegments.Size()) {
		std::vector<int32> bicStartEnd = bicSegments.SegStartEnd(i);
		std::vector<int32> refStartEnd = refSegments.SegStartEnd(j);
		if (bicStartEnd[0] >= refStartEnd[0] &&
				bicStartEnd[1] <= refStartEnd[1]) {
			i++;
			unwantedChanges++;
		}else {
			j++;
			i++;
		}
	}
	return 100.*unwantedChanges/bicSegments.Size();
}




Window::Window(const int32 start, const int32 length) {
	this->_start = start;
	this->_end 	= start+length;
}


void Window::ResetWindow(const int32 start, const int32 length) {
	this->_start = start;
	this->_end 	= start+length;
}


int32 Window::Length() {
	return this->_end - this->_start; 
}


int32 Window::Start() {
	return this->_start;
}


int32 Window::End() {
	return this->_end;
}


void Window::GrowWindow(int32 N) {
	this->_end += N;
}


void Window::ShiftWindow(int32 N) {
	this->_start += N;
	this->_end += N;
}


void Window::CenterWindow(int32 center, int32 length){
	if (length % 2 == 0) {
		this->_start = center - length/2;
		this->_end = center + length/2;
	} else {
		this->_start = center - length/2;
		this->_end = center + length/2 + 1;
	}
}


}