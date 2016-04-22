#ifndef KALDI_IVECTOR_DIAR_UTILS_H_
#define KALDI_IVECTOR_DIAR_UTILS_H_

#include <vector>
#include "util/common-utils.h"
#include "matrix/matrix-lib.h"

namespace kaldi{
typedef std::vector< std::pair<std::string, std::vector<int32> > > segType;

// convert number to string
// template<class T> std::string numberToStr(T number);

template<class T>
inline std::string numberToStr(T number){

    std::stringstream tmpStream;
    tmpStream << number;
    return tmpStream.str();
}

class Diarization{
public:
	Diarization(){
		Nmin 	= 300,
		Nmax 	= 2000;
		Nsecond = 300;
		Nshift 	= 200;
		Nmargin = 100;
		Ngrow 	= 100;
		lambda 	= 1.25;
		lowResolution = 25;
		highResolution = 5;

		//
		frameShift 	= 0.010;
		frameLength = 0.025;

	}
	void LabelsToSegments(const Vector<BaseFloat>&, segType&);
	void SegmentsToLabels(const segType&, Vector<BaseFloat>&);
	void getSpeechSegments(const segType& segments, segType& speechSegments);
	void SegmentsToRTTM(const std::string& fileName, const segType& segments, const std::string& outName);
	BaseFloat FrameIndexToSeconds(int32 frame);
	bool BicSegmentation(std::vector<int32>&, const Matrix<BaseFloat>&, segType&);    			
	BaseFloat compareSegments(segType& segments1, segType& segments2);
	std::pair<int32, BaseFloat> computeBIC(const std::vector<int32>&, const Matrix<BaseFloat>&, int32);
	BaseFloat detCovariance(Matrix<BaseFloat>&);
	std::vector<int32> initWindow(int32, int32);
	void growWindow(std::vector<int32>&, int32);
	void shiftWindow(std::vector<int32>&, int32);
	void centerWindow(std::vector<int32>&, int32, int32);

	// BIC parameteres
	int32 Nmin;
	int32 Nmax;
	int32 Nsecond;
	int32 Nshift;
	int32 Nmargin;
	int32 Ngrow;
	BaseFloat lambda; // penalty factor for model complexity in BIC  
	int32 lowResolution;
	int32 highResolution;

	// windowing parameters
	BaseFloat frameShift;
	BaseFloat frameLength;
}; 



}

#endif