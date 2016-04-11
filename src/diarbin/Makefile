
all:

EXTRA_CXXFLAGS = -Wno-sign-compare -I${KALDI_DIR}/src
include ${KALDI_DIR}/src/kaldi.mk

BINFILES = ivector-extractor-conv-init ivector-extractor-conv-acc-stats \
		   ivector-extractor-conv-sum-acc ivector-extractor-conv-est \
		   ivector-extract-conv \
		   copy-tmat-conv 

OBJFILES =

TESTFILES =

ADDLIBS = ../ivector/kaldi-ivector-conv.a 

include ${KALDI_DIR}/src/makefiles/default_rules.mk
