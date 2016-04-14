#include <vector>
#include "util/common-utils.h"
#include "matrix/matrix-lib.h"

namespace kaldi{
typedef std::vector< std::pair<std::string, std::vector<int32> > > segType;

class Diarization{
public:
	void LabelsToSegments(const Vector<BaseFloat>& labels, segType& segments);
	//void SegmentsToLabels(const segType& segments, Vector<int32>& labels);
	//void BicSegmentation(std::vector<int32> &segment, const Matrix<BaseFloat>& feats, segType& bicsegments);    			

//Diarization::LabelsToSegments(const T&, kaldi::segType&)

	//int minBicWindowLength = 500; // msec 
	//int BicWindowIncrement = 500; // msec

}; 



}
