#pragma once

#include "SegmProc.h"
#include "SegmSession.h"

namespace cv {
	class Mat;
}

namespace segm_proc
{
	class SEGMPROC_DLL_API OpticDiscSession : public SegmSession
	{
	public:
		OpticDiscSession();
		virtual ~OpticDiscSession();

	public:
		virtual bool initialize(void) override;
		virtual bool rectify(void) override;

	protected:
		void determineDiscRange(void);
		bool makeBRMImage(OctScanPattern pattern);
		bool elaborateDiscRange(OctScanPattern pattern);
		bool preprocessingEnface(cv::Mat & enfaceImg);
		bool findSmoothContour(cv::Mat& enfaceImg);
		bool modifyDiscPoint(cv::Mat& resizeDisc, int rows, int cols, bool horizontal);
		bool filterOpticDiscSet(void);
		bool adjustOpticDiscSet(void);
		bool isSegmentVersion2(void);

	private:
		struct OpticDiscSessionImpl;
		static std::unique_ptr<OpticDiscSessionImpl> d_ptr;
		static OpticDiscSessionImpl& getImpl(void);
	};
}
