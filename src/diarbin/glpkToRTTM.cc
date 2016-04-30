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
                rttm_scpfile = po.GetArg(3);

    GlpkILP glpkObj;
    std::vector<std::string> ilpClusterLabel = glpkObj.ReadGlpkSolution(glpk_rspecifier);

    // create empty segments to store glpk ILP generated cluster label
    Input ki(segments_scpfile);  // no binary argment: never binary.
    std::string line;

    Input ko(rttm_scpfile);
    std::string rttm_filename;

    int32 ind = 0;
    while (std::getline(ki.Stream(), line)) {
        Segments uttSegments;
        uttSegments.Read(line);
        Segments speechSegments = uttSegments.GetSpeechSegments();
        
        for (size_t i = 0; i < speechSegments.Size(); i++) {
            speechSegments.SetLabel(i, ilpClusterLabel[ind]);
            ind++;
        }

        std::getline(ko.Stream(), rttm_filename);
        speechSegments.ToRTTM(speechSegments.GetUttID(), rttm_filename);
    }
}

