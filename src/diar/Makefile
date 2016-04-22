
all:

KALDI_DIR = /home/nxs113020/kaldi-trunk
#KALDI_DIR = /home/chengzhu/work/Toolhouse/kaldi-trunk
OPENFST_CXXFLAGS = 
OPENFST_LDLIBS = 
EXTRA_CXXFLAGS = -I${KALDI_DIR}/src

include ${KALDI_DIR}/src/kaldi.mk

TESTFILES = 

OBJFILES = diar-utils.o ilp.o

LIBNAME = kaldi-diar

ADDLIBS = ${KALDI_DIR}/src/ivector/kaldi-ivector.a ${KALDI_DIR}/src/hmm/kaldi-hmm.a ${KALDI_DIR}/src/tree/kaldi-tree.a ${KALDI_DIR}/src/transform/kaldi-transform.a \
		${KALDI_DIR}/src/thread/kaldi-thread.a ${KALDI_DIR}/src/matrix/kaldi-matrix.a ${KALDI_DIR}/src/base/kaldi-base.a \
        ${KALDI_DIR}/src/util/kaldi-util.a 

include ${KALDI_DIR}/src/makefiles/default_rules.mk

