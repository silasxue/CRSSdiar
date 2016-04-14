
all:

KALDI_DIR = /home/chengzhu/work/Toolhouse/kaldi-trunk
EXTRA_CXXFLAGS = -Wno-sign-compare -I${KALDI_DIR}/src
include ${KALDI_DIR}/src/kaldi.mk

BINFILES = changeDetectBIC

OBJFILES =

TESTFILES =

ADDLIBS = ../diar/kaldi-diar.a ${KALDI_DIR}/src/matrix/kaldi-matrix.a ${KALDI_DIR}/src/util/kaldi-util.a  ${KALDI_DIR}/src/base/kaldi-base.a


include ${KALDI_DIR}/src/makefiles/default_rules.mk
