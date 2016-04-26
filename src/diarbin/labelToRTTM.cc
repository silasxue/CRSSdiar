// diarbin/labelToRTTM.cc

#include <string>
#include "util/common-utils.h"
#include "base/kaldi-common.h"
#include "diar/ilp.h"
#include "diar/diar-utils.h"


int main(int argc, char *argv[]) {
	typedef kaldi::int32 int32;
	using namespace kaldi;

	const char *usage = "Convert label (vector of speaker/nonspeech/overlap indexs) into RTTM format \n";

    kaldi::ParseOptions po(usage);
	po.Read(argc, argv);

	if (po.NumArgs() != 2) {
        po.PrintUsage();
        exit(1);
    }

    std::string label_rspecifier = po.GetArg(1),
                rttm_outputdir = po.GetArg(2);

    SequentialBaseFloatVectorReader label_reader(label_rspecifier);

    for (; !label_reader.Done(); label_reader.Next()) {
        Segments allSegments(label_reader.Value(), label_reader.Key());
        Segments speechSegments = allSegments.GetSpeechSegments();
        std::string rttm_wspecifier = rttm_outputdir + "/" + speechSegments.GetUttID() +".rttm";
        speechSegments.ToRTTM(speechSegments.GetUttID(), rttm_wspecifier);
    }
}