#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "diar/ilp.h"
#include "diar/diar-utils.h"

std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::vector<std::string> returnNonEmptyFields(const std::vector<std::string>& fields){
    std::vector<std::string> nonEmptyFields; 
    for(size_t i = 0; i < fields.size(); i++){
        if(fields[i] != ""){
            nonEmptyFields.push_back(fields[i]);
        }
    }
    return nonEmptyFields;
}

std::vector<int32> varNameToIndex(std::string& varName){
    std::vector<std::string> fields = split(varName, '_');
    std::vector<int32> indexes;
    indexes.push_back(std::atoi(fields[1].c_str()));
    indexes.push_back(std::atoi(fields[2].c_str()));
    return indexes;
}

int main(int argc, char *argv[]) {
    typedef kaldi::int32 int32;
    using namespace kaldi;

    const char *usage = "Convert gplk output into RTTM format for compute DER \n";

    kaldi::ParseOptions po(usage);
    po.Read(argc, argv);

    if (po.NumArgs() != 3) {
        po.PrintUsage();
        exit(1);
    }

    std::string glpk_rspecifier = po.GetArg(1),
                label_rspecifier = po.GetArg(2),
                rttm_wspecifier = po.GetArg(3);

    SequentialBaseFloatVectorReader label_reader(label_rspecifier);    

    segType allSegments;
    for (; !label_reader.Done(); label_reader.Next()) {
        Diarization diarObj;
        segType segments;
        segType speechSegments;
        diarObj.LabelsToSegments(label_reader.Value(), segments);
        diarObj.getSpeechSegments(segments, speechSegments);
        allSegments.insert(allSegments.end(),speechSegments.begin(),speechSegments.end());
    }

    std::ifstream fin;
    fin.open(glpk_rspecifier.c_str());
    std::string line;
    while (std::getline(fin, line)){
        if (line.find("*") != std::string::npos){
            std::vector<std::string> fields = split(line, ' ');
            std::vector<string> nonEmptyFields = returnNonEmptyFields(fields);
            std::vector<int32> varIndex = varNameToIndex(nonEmptyFields[1]);
            int32 k = varIndex[0];
            int32 j = varIndex[1];

            if (k==j  && nonEmptyFields[3] == "1") {

                allSegments[k].first = numberToStr(k);
            }
            if (k!=j && nonEmptyFields[3] == "1") {
                    allSegments[k].first = numberToStr(j);
            }
        }
    }
    Diarization diarObj;
    diarObj.SegmentsToRTTM("allfile",allSegments,rttm_wspecifier);
}

