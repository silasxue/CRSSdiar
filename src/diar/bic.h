#ifndef KALDI_BIC_H_
#define KALDI_BIC_H_

#include <vector>
#include "util/common-utils.h"
#include "matrix/matrix-lib.h"
#include "diar-utils.h"

namespace kaldi{


class Window {
public:
	Window(const int32 start, const int32 length);
	int32 Length();
	int32 Start();
	int32 End();
	void GrowWindow(int32 N);
	void ShiftWindow(int32 N);
	void CenterWindow(int32 center, int32 length);
	void ResetWindow(const int32 start, const int32 length);
private:
	int32 _start;
	int32 _end;
};


struct BICOptions {
	BICOptions();

	int32 Nmin;
	int32 Nmax;
	int32 Nsecond;
	int32 Nshift;
	int32 Nmargin;
	int32 Ngrow;
	BaseFloat lambda; // penalty factor for model complexity in BIC  
	int32 lowResolution;
	int32 highResolution;
};


class BIC {
public:
	BIC(const BICOptions& opts) {
		this->_opts = opts;
	};

	void BICSegmentation(const Segments& uttSegments, const Matrix<BaseFloat>& feats, Segments& bicSegments); 
	bool SplitSegment(const segUnit& origSegment, const Matrix<BaseFloat>& feats, Segments& bicSegments);
	BaseFloat CompareSegments(const Segments& refSegments, const Segments& bicSegments);
	std::pair<int32, BaseFloat> ComputeBIC(Window& window, const Matrix<BaseFloat>&, int32);


private:
	BICOptions _opts;
};


}

#endif