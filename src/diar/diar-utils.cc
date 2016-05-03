#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include "diar-utils.h"


namespace kaldi {


Segments::Segments(){}

Segments::Segments(const std::string uttid){
	this->_uttid = uttid;
}


Segments::Segments(const Vector<BaseFloat>& labels, const std::string uttid) {
	// NOTE: The rules of input label is as follow
	// -1 -> overlap
	// 0 -> nonspeech
	// 1,2,..,n -> speaker1, speaker2, speakern   
	// Other conditions may be added in the future.
	this->_uttid = uttid;
	int32 state;
	int32 prevState;
	std::vector<int32> segmentStartEnd;
	int32 startSeg = 0;
	int32 endSeg = 0;
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
				this->_segmentList.push_back(std::make_pair("overlap",segmentStartEnd));
				segmentStartEnd[0] = i;
			}else if (prevState == 0) {
				segmentStartEnd[1] = i-1;
				this->_segmentList.push_back(std::make_pair("nonspeech",segmentStartEnd));
				segmentStartEnd[0] = i;				
			}else if (prevState > 0) {
				std:: string stateStr = numberToString(prevState);
				segmentStartEnd[1] = i-1;
				this->_segmentList.push_back(std::make_pair(stateStr,segmentStartEnd));
				segmentStartEnd[0] = i;
			} 
		}
	}
}

std::string Segments::GetUttID() {
	return this->_uttid;
}


Vector<double> Segments::GetIvector(int32 index) {
	return this->_ivectorList[index];
}


int32 Segments::Size() const {
	return int32(this->_segmentList.size());
}


std::string Segments::SegKey(int32 index) {
	return this->_segmentList[index].first;
}


std::vector<int32> Segments::SegStartEnd(int32 index) const {
	return this->_segmentList[index].second;
}


segUnit Segments::GetSeg(int32 index) const {
	return this->_segmentList[index];
}

segUnit Segments::End() {
	return this->_segmentList.back();
}


segUnit Segments::Begin() {
	return this->_segmentList[0];
}


void Segments::ToLabels(Vector<BaseFloat>& labels){
	// NOTE: The rules of input label is as follow
	// -1 -> overlap
	// 0 -> nonspeech
	// 1,2,..,n -> speaker1, speaker2, speakern   
	// Other conditions may be added in the future. 
	std::vector<int32> lastSegment = this->_segmentList.back().second;
	labels.Resize(lastSegment[1]+1);
	for (size_t i=0; i< this->_segmentList.size(); i++) {
		std::vector<int32> segmentStartEnd = this->_segmentList[i].second;
		int32 segLen = segmentStartEnd[1] - segmentStartEnd[0] + 1;
		Vector<BaseFloat> segLabels(segLen);		
		if (this->_segmentList[i].first == "nonspeech") { 
			segLabels.Set(0.0);
		}else if (this->_segmentList[i].first == "overlap") {
			segLabels.Set(-1.0);
		}else {
			int32 stateInt = std::atoi(this->_segmentList[i].first.c_str());
			segLabels.Set(stateInt);
		}
		labels.Range(segmentStartEnd[0],segLen).CopyFromVec(segLabels);
	}
}


void Segments::ToRTTM(const std::string& uttid, const std::string& rttmName) {
	std::ofstream fout;
	fout.open(rttmName.c_str());
	for (size_t i =0; i < this->_segmentList.size(); i++){
		std::string spkrID = this->_segmentList[i].first;
		BaseFloat segStart = FrameIndexToSeconds(this->_segmentList[i].second[0]);
		BaseFloat segLength = FrameIndexToSeconds(this->_segmentList[i].second[1]) - segStart;
		fout << "SPEAKER ";
		fout << uttid << " ";
		fout << 1 << " ";
		fout << std::fixed << std::setprecision(3);
		fout << segStart << " ";
		fout << segLength << " ";
		fout << "<NA> <NA> ";
		fout << spkrID << " ";
		fout << "<NA>\n";
	}
	fout.close();
}


Segments Segments::GetSpeechSegments() {
	Segments speechSegments(_uttid);
	for (size_t i = 0; i < this->_segmentList.size(); i++) {
		if (this->_segmentList[i].first != "nonspeech" && this->_segmentList[i].first != "overlap") {
			speechSegments.Append(this->_segmentList[i]);
		}
	}
	return speechSegments;
}

Segments Segments::GetLargeSegments(int32 segMin) {
	Segments largeSegments(_uttid);
	for (size_t i = 0; i < this->_segmentList.size(); i++) {
		segUnit currSeg = this->_segmentList[i];
		int32 currSegSize = currSeg.second[1] - currSeg.second[0] + 1;
		if (currSegSize >= segMin) {
			largeSegments.Append(this->_segmentList[i]);
		}
	}
	return largeSegments;
}


void Segments::ExtractIvectors(const Matrix<BaseFloat>& feats,
							   const Posterior& posterior,
							   const IvectorExtractor& extractor) {
	int32 featsDim = extractor.FeatDim();
	size_t numSegs = this->_segmentList.size();
	for (size_t i=0; i < numSegs; i++){
		std::string segmentLabel = this->_segmentList[i].first;
		std::vector<int32> segmentStartEnd = this->_segmentList[i].second;
		Matrix<BaseFloat> segFeats(segmentStartEnd[1] - segmentStartEnd[0] +1, featsDim);
		segFeats.CopyFromMat(feats.Range(segmentStartEnd[0], segmentStartEnd[1] - segmentStartEnd[0] +1, 0, featsDim));

		Posterior::const_iterator startIter = posterior.begin() + segmentStartEnd[0];
		Posterior::const_iterator endIter = posterior.begin() + segmentStartEnd[1]+1;
		Posterior segPosterior(startIter, endIter);
		
		KALDI_LOG << " Segment Range : segmentStartEnd[0]" << " <-> " << segmentStartEnd[1] << " The seg size is: " << segPosterior.size();
		GetSegmentIvector(segFeats, segPosterior, extractor, segmentStartEnd);
	}
}


void Segments::GetSegmentIvector(const Matrix<BaseFloat>& segFeats, 
							     const Posterior& segPosterior, 
							     const IvectorExtractor& extractor, 
							     const std::vector<int32>& segmentStartEnd) {
	Vector<double> ivector;
    bool need_2nd_order_stats = false;
    IvectorExtractorUtteranceStats utt_stats(extractor.NumGauss(),
                                             extractor.FeatDim(),
                                             need_2nd_order_stats);
    utt_stats.AccStats(segFeats, segPosterior);
    ivector.Resize(extractor.IvectorDim());
    ivector(0) = extractor.PriorOffset();
    extractor.GetIvectorDistribution(utt_stats, &ivector, NULL);
    _ivectorList.push_back(ivector);
}


void Segments::NormalizeIvectors() {
	// NOTE: Add variance normalization to this function. 
	Vector<double> ivectorMean;
	computeMean(this->_ivectorList, ivectorMean);
	//SpMatrix<double> ivectorCovariance = computeCovariance(this->_ivectorList, ivectorMean);
	for (size_t i = 0; i < this->_ivectorList.size(); i++) {
		this->_ivectorList[i].AddVec(-1, ivectorMean);
	}
}


void Segments::Append(segUnit& segment) {
	this->_segmentList.push_back(segment);
}


void Segments::SetLabel(int32 index, std::string label) {
	this->_segmentList[index].first = label;
}

void Segments::Read(const std::string& segments_rxfilename) {
	// segments_rxfilename contains only segments information from single audio stream.
    Input ki(segments_rxfilename);  // no binary argment: never binary.
    std::string line;
    /* read each line from segments file */
    while (std::getline(ki.Stream(), line)) {
		std::vector<std::string> split_line;
		// Split the line by space or tab and check the number of fields in each
		// line. There must be 4 fields--segment name , reacording wav file name,
		// start time, end time; 5th field (speaker ID info) is optional.
		SplitStringToVector(line, " \t\r", true, &split_line);
		if (split_line.size() != 4 && split_line.size() != 5) {
			KALDI_WARN << "Invalid line in segments file: " << line;
			continue;
		}

		std::string segment = split_line[0],
		recording = split_line[1],
		start_str = split_line[2],
		end_str = split_line[3];

		if (this->_uttid.empty()) {
			this->_uttid = recording;
		}else {
			if (this->_uttid != recording) {
				KALDI_ERR << "Only one audio stream is permitted per segment file.";
			}
		}
		  
		// Convert the start time and endtime to real from string. Segment is
		// ignored if start or end time cannot be converted to real.
		double start, end;
		if (!ConvertStringToReal(start_str, &start)) {
			KALDI_WARN << "Invalid line in segments file [bad start]: " << line;
			continue;
		}
		if (!ConvertStringToReal(end_str, &end)) {
			KALDI_WARN << "Invalid line in segments file [bad end]: " << line;
			continue;
		}
		// start time must not be negative; start time must not be greater than
		// end time, except if end time is -1
		if (start < 0 || (end != -1.0 && end <= 0) || ((start > end) && (end > 0))) {
			KALDI_WARN << "Invalid line in segments file [empty or invalid segment]: "
		           << line;
		continue;
		}
		std::string spkrLabel = "unk";  // default speaker label is unknown.
		// if each line has 5 elements then 5th element must be speaker label
		if (split_line.size() == 5) {
			spkrLabel = split_line[4];
		}

		std::vector<int32> segStartEnd;
		segStartEnd.push_back(SecondsToFrameIndex(BaseFloat(start)));
		segStartEnd.push_back(SecondsToFrameIndex(BaseFloat(end)));
		this->_segmentList.push_back(std::make_pair(spkrLabel,segStartEnd));
	}	
}


void Segments::ReadIvectors(const std::string& ivector_rxfilename) {
	SequentialDoubleVectorReader ivector_reader(ivector_rxfilename);
    for (; !ivector_reader.Done(); ivector_reader.Next()) {
        std::string ivectorKey = ivector_reader.Key();   
        this->_ivectorList.push_back(ivector_reader.Value());
    }
    if (this->_ivectorList.size() != this->_segmentList.size()) {
    	KALDI_ERR << "Number of ivectors doesn't match number of segments!";
    }
}


void Segments::Write(const std::string& segments_dirname) {
	std::string segments_wxfilename = segments_dirname + "/" + this->_uttid + ".seg";
	std::string segments_scpfilename = segments_dirname + "/" + "segments.scp";
	std::ofstream fout;
	std::ofstream fscp;
	fout.open(segments_wxfilename.c_str());
	fscp.open(segments_scpfilename.c_str(), std::ios::app);
	for (size_t i =0; i < this->_segmentList.size(); i++){
		std::string segID = makeSegKey(this->_segmentList[i].second, this->_uttid);
		std::string spkrLabel = this->_segmentList[i].first;
		BaseFloat segStart = FrameIndexToSeconds(this->_segmentList[i].second[0]);
		BaseFloat segEnd = FrameIndexToSeconds(this->_segmentList[i].second[1]);
		fout << segID << " ";
		fout << this->_uttid << " ";
		fout << std::fixed << std::setprecision(3);
		fout << segStart << " ";
		fout << segEnd << " ";
		fout << spkrLabel << "\n";
	}
	fscp << segments_wxfilename << "\n";
	fscp.close();
	fout.close();
}


void Segments::WriteIvectors(const std::string& ivector_wxfilename){
	DoubleVectorWriter ivectorWriter(ivector_wxfilename);
	for (size_t i =0; i < this->_segmentList.size(); i++){
		std::string segID = makeSegKey(this->_segmentList[i].second, this->_uttid);
		if (this->_ivectorList.empty()) {
			KALDI_ERR << "ivector list for " << this->_uttid << " Segments do not exist!"; 
		}
		ivectorWriter.Write(segID, this->_ivectorList[i]);
	}
}


BaseFloat FrameIndexToSeconds(int32 frame) {
	// Find corresponding start point (in seconds) of a given frame.
	return frame*FRAMESHIFT;
}


std::string makeSegKey(const std::vector<int32>& segmentStartEnd, 
				const std::string uttid) { 
	// Make unique key for each segment of each utterance, by concatenating uttid with segment start and end
	// Such that the key is format of "uttid_segStartFrame_segEndFrame".
	std::string segStartEndString;
	std::stringstream tmp; 
	tmp << segmentStartEnd[0];
	tmp << "_";
	tmp << segmentStartEnd[1];
	segStartEndString = tmp.str();
	std::string	segID = uttid + "_" + segStartEndString;

	return segID;       
}


std::vector<std::string>& split(const std::string& s, 
								char delim, 
								std::vector<std::string>& elems) {
	// Split string by delimiter e.g., ' ', ','. 
	// PASS output vector by reference.
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string& s, char delim) {
	// Split string by delimiter, e.g. ',' 
	// CREATE output vector.
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}


std::vector<std::string> returnNonEmptyFields(const std::vector<std::string>& fields) {
	// Return non empty elements of vector of strings.
    std::vector<std::string> nonEmptyFields; 
    for(size_t i = 0; i < fields.size(); i++){
        if(fields[i] != ""){
            nonEmptyFields.push_back(fields[i]);
        }
    }
    return nonEmptyFields;
}


void computeDistanceMatrix(const std::vector< Vector<double> >& vectorList, Matrix<BaseFloat>& distanceMatrix) {
	distanceMatrix.Resize(vectorList.size(),vectorList.size());
	// Calculate total mean and covariance:
	Vector<double> vectorMean;
	computeMean(vectorList, vectorMean);
	SpMatrix<double> vectorCovariance = computeCovariance(vectorList, vectorMean);
	for (size_t i=0; i<vectorList.size();i++){
		for (size_t j=0;j<vectorList.size();j++){
			if (i == j){
				distanceMatrix(i,j) = 0;
			}else{
				distanceMatrix(i,j) = mahalanobisDistance(vectorList[i], vectorList[j], vectorCovariance);
			}
		}
	}
}


BaseFloat mahalanobisDistance(const Vector<double>& v1, const Vector<double>& v2, const SpMatrix<double>& totalCov) {

	Vector<double> iv1(v1.Dim());
	iv1.CopyFromVec(v1);
	Vector<double> iv2(v2.Dim());
	iv2.CopyFromVec(v2);
	SpMatrix<double> Sigma(v2.Dim());
	Sigma.CopyFromSp(totalCov);
	Sigma.Invert();
	iv1.AddVec(-1.,iv2);

	// Now, calculate the quadratic term: (iv1 - iv2)^T Sigma (iv1-iv2)
	Vector<double> S_iv1(iv1.Dim());
		S_iv1.AddSpVec(1.0, Sigma, iv1, 0.0);
		return sqrt(VecVec(iv1, S_iv1));
}


BaseFloat cosineDistance(const Vector<double>& v1, const Vector<double>& v2) {
	 BaseFloat dotProduct = VecVec(v1, v2);
	 BaseFloat norm1 = VecVec(v1, v1) + FLT_EPSILON;
	 BaseFloat norm2 = VecVec(v2, v2) + FLT_EPSILON;

	 return dotProduct / (sqrt(norm1)*sqrt(norm2));  
}


}


