#include <iostream>
#include <sstream>
#include <string>
#include "diar-utils.h"


namespace kaldi {
//void Diarization::BicSegmentation(std::vector<int32> segment, Matrix<BaseFloat>& feats, segType* bicsegments) {
	/*
	For a given segment/break-point, calculate 3 GMMs. Z: X V Y. 
	The break-point c partitions Z into X and Y. 
	Delta-bic is the difference between the hypothesis of 1) Z vs. 2) X and Y separate. 
	*/

	// int segmentLength = segment[1] - segment[0];

	// // How do I get access to fs?

	// if (minWindowLength >= segmentLength) {
	// 	bicsegments.push_back(std::make_pair())
	// }

//}


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