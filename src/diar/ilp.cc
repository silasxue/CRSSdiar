#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include "ilp.h"

namespace kaldi{

	void IlpCluster::ExtractSegmentIvectors(const Matrix<BaseFloat>& feats, 
								const segType& inputSegments, 
								const Posterior& posterior, 
								const IvectorExtractor& extractor, 
								DoubleVectorWriter& ivectorWriter,
								const std::string key){

		int32 featsDim = extractor.FeatDim();
		size_t numSegs = inputSegments.size();

		int32 numSpeechSeg = 0;
		for (size_t i=0; i<numSegs; i++){

			std::string segmentLabel = inputSegments[i].first;

			if (segmentLabel != "nonspeech" && segmentLabel != "overlap"){

				numSpeechSeg++;

				std::vector<int32> segmentStartEnd = inputSegments[i].second;
				Matrix<BaseFloat> segFeats(segmentStartEnd[1] - segmentStartEnd[0] +1, featsDim);
				segFeats.CopyFromMat(feats.Range(segmentStartEnd[0], segmentStartEnd[1] - segmentStartEnd[0] +1, 0, featsDim ));

				Posterior::const_iterator startIter = posterior.begin() + segmentStartEnd[0];
				Posterior::const_iterator endIter = posterior.begin() + segmentStartEnd[1]+1;
				Posterior segPosterior(startIter, endIter);

				KALDI_LOG << " Segment Range : segmentStartEnd[0]" << " <-> " << segmentStartEnd[1] << " The seg size is: " << segPosterior.size();

				GetSegmentIvector(segFeats, segPosterior, extractor, ivectorWriter, key, segmentStartEnd);

			}

			//if(segmentIvectors.size() != numSpeechSeg){
			//	KALDI_ERR << "Number of speech segments and the number of extracted i-vectors does not match, \n"
			//			  << "number of speech segments: " << numSpeechSeg 
			//			  << " ,number of ivecotrs: " << segmentIvectors.size() << "\n"; 
			//
			//}

		}



	}

	void IlpCluster::GetSegmentIvector(const Matrix<BaseFloat>& segFeats, 
						   const Posterior& segPosterior, 
						   const IvectorExtractor& extractor, 
						   DoubleVectorWriter& ivectorWriter,
						   std::string uttid, 
						   const std::vector<int32>& segmentStartEnd){


		Vector<double> ivector;

	    bool need_2nd_order_stats = false;
	    
	    IvectorExtractorUtteranceStats utt_stats(extractor.NumGauss(),
	                                             extractor.FeatDim(),
	                                             need_2nd_order_stats);
	      
	    utt_stats.AccStats(segFeats, segPosterior);
	    
	    ivector.Resize(extractor.IvectorDim());
	    ivector(0) = extractor.PriorOffset();


	    extractor.GetIvectorDistribution(utt_stats, &ivector, NULL);

		std::string segStartEndString;
		std::stringstream tmp; 
		tmp << segmentStartEnd[0];
		tmp << "_";
		tmp << segmentStartEnd[1];
		segStartEndString = tmp.str();		


	    std::string key = uttid + "_" + segStartEndString;

		ivectorWriter.Write(key, ivector);

	}

	void IlpCluster::makeSegKey( const std::vector<int32>& segmentStartEnd, const std::string uttid, std::string& segIvectorKey ){ 
		// Make unique key for each segment of each utterance, by concatenating uttid with segment start and end
		// Such that the key is format of "uttid_segStartFrame_segEndFrame".

	    std::string segStartEndString;
	    std::stringstream tmp; 
	    tmp << segmentStartEnd[0];
	    tmp << "_";
	    tmp << segmentStartEnd[1];
	    segStartEndString = tmp.str();

	    segIvectorKey = uttid + "_" + segStartEndString;       
	}




}