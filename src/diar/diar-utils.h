#include <vector>
#include "util/common-utils.h"

namespace kaldi{
typedef std::vector< std::pair<std::string, std::vector<int32> > > segType;

class Diarzation{
public:
	void LabelsToSegments(std::vector<BaseFloat> labels, segType* segments);
	void SegmentsToLabels(segType* segments, std::vector<BaseFloat> labels);
	void BicSegmentation(std::vector<int32> segment, Matrix<BaseFloat>& feats, segType* bicsegments);    			
}; 



}
