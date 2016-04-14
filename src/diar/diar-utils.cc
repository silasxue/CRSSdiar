#include <iostream>
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
	// NOTE: At this stage we only consider the case were labels includes VAD 
	// and/or overlap marks (spch:1, nonspch:0, overlap:-1). 
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

			switch (prevState) {
				case 0:
					segmentStartEnd[1] = i-1;
					segments.push_back(std::make_pair("nonspeech",segmentStartEnd));
					segmentStartEnd[0] = i;
					break;
				case 1:
					segmentStartEnd[1] = i-1;
					segments.push_back(std::make_pair("speech",segmentStartEnd));
					segmentStartEnd[0] = i;
					break;
				case -1:
					segmentStartEnd[1] = i-1;
					segments.push_back(std::make_pair("overlap",segmentStartEnd));
					segmentStartEnd[0] = i;
					break;
			}
		}

	}
}


void Diarization::SegmentsToLabels(const segType& segments, Vector<BaseFloat>& labels){
	//NOTE: At this stage we only consider the case were labels includes VAD 
	//and/or overlap marks (spch:1, nonspch:0, overlap:-1). 
	//Other conditions may be added in the future. 

	std::vector<int32> lastSegment = segments.back().second;
	labels.Resize(lastSegment[1]+1);
	for (size_t i=0; i<segments.size(); i++) {
		
		std::vector<int32> segmentStartEnd = segments[i].second;
		int32 segLen = segmentStartEnd[1] - segmentStartEnd[0] + 1;
		Vector<BaseFloat> segLabels(segLen);		

		if (segments[i].first == "speech") segLabels.Set(1.0);
		if (segments[i].first == "nonspeech") segLabels.Set(0.0);
		if (segments[i].first == "overlap") segLabels.Set(-1.0);

		labels.Range(segmentStartEnd[0],segLen).CopyFromVec(segLabels);
	}
}


}