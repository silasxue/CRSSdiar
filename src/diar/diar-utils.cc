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
			switch (prevState) {
				case 0:
					segmentStartEnd[2] = i-1;
					segments.push_back(std::make_pair("nonspeech",segmentStartEnd));
					segmentStartEnd[1] = i;
					break;
				case 1:
					segmentStartEnd[2] = i-1;
					segments.push_back(std::make_pair("speech",segmentStartEnd));
					segmentStartEnd[1] = i;
					break;
				case -1:
					segmentStartEnd[2] = i-1;
					segments.push_back(std::make_pair("overlap",segmentStartEnd));
					segmentStartEnd[1] = i;
					break;
			}
		}
	}
}


//void Diarization::SegmentsToLabels(const segType& segments, std::vector<int32>& labels){
	// NOTE: At this stage we only consider the case were labels includes VAD 
	// and/or overlap marks (spch:1, nonspch:0, overlap:-1). 
	// Other conditions may be added in the future. 

	// for (size_t i=0; i<segments.size(); i++) {
	// 	std::vector segLabels;
	// 	switch (segments[i].first) {
	// 		case "speech":
				
	// 	}
	// }
//}


}