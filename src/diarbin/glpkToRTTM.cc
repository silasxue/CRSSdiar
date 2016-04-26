#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "diar/ilp.h"
#include "diar/diar-utils.h"


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
                segments_scpfile = po.GetArg(2),
                rttm_wspecifier = po.GetArg(3);


    Input ki(segments_scpfile);  // no binary argment: never binary.
    std::string line;
    while (std::getline(ki.Stream(), line)) {
        Segments uttSegments;
        uttSegments.Read(line);
        Segments speechSegments = uttSegments.GetSpeechSegments();
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
                allSegments[k].first = numberToString(k);
            }
            if (k!=j && nonEmptyFields[3] == "1") {
                    allSegments[k].first = numberToString(j);
            }
        }
    }
    Diarization diarObj;
    diarObj.SegmentsToRTTM("allfile",allSegments,rttm_wspecifier);
}

