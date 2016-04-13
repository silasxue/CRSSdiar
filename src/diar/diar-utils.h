#include <vector>
#include "util/common-utils.h"

namespace kaldi{
typedef std::vector< std::pair<std::string, std::vector<int32> > > segType;

class Diarzation{
public:
	void LabelsToSegments(const std::vector<int32>& labels, segType& segments);
	void SegmentsToLabels(const segType& segments, std::vector<int32>& labels);
	void BicSegmentation(std::vector<int32> segment, Matrix<BaseFloat>& feats, segType* bicsegments);    			


	int minBicWindowLength = 500; // msec 
	int BicWindowIncrement = 500; // msec

}; 



}
