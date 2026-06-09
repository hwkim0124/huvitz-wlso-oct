#pragma once

#include "SegmProc.h"

#include <memory>
#include <vector>


namespace oct_report {
	class ProtocolSource;
	class LineDataSource;
	class CrossDataSource;
	class CircleDataSource;
	class RadialDataSource;
	class RasterDataSource;
	class CubeDataSource;
}

namespace segm_scan {
	class OcularBsegm;
}


namespace segm_proc
{
	class SEGMPROC_DLL_API SegmSession
	{
	public:
		SegmSession();
		virtual ~SegmSession();

	public:
		bool loadScanData(oct_report::ProtocolSource* data);
		bool processScanData(void);

	protected:
		virtual bool initialize(void);
		virtual bool execute(void);
		virtual bool rectify(void);
		virtual bool finalize(void);

		oct_report::ProtocolSource* getScanDataSource(void) const;

		int getPatternBsegmCount(void) const;
		void clearAllPatternBsegms(void);
		void addPatternBsegm(std::unique_ptr<segm_scan::OcularBsegm> bsegm);
		segm_scan::OcularBsegm* getPatternBsegm(int index) const;

		int getPreviewBsegmCount(void) const;
		void clearAllPreviewBsegms(void);
		void addPreviewBsegm(std::unique_ptr<segm_scan::OcularBsegm> bsegm);
		segm_scan::OcularBsegm* getPreviewBsegm(int index) const;

	private:
		struct SegmSessionImpl;
		static std::unique_ptr<SegmSessionImpl> d_ptr;
		static SegmSessionImpl& getImpl(void);
	};
}


