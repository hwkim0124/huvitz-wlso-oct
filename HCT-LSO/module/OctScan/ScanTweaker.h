#pragma once

#include "OctScan2.h"

#include <memory>
#include <string>



namespace oct_scan
{
	class OCTSCAN_DLL_API ScanTweaker
	{
	public:
		ScanTweaker();
		virtual ~ScanTweaker();

	public:

	protected:
		static bool obtainQualityIndexFromPreview(float& qidx, float& sig_ratio, bool next = true);
		static bool obtainReferencePointFromPreview(int& refPoint, bool next = true);

		static bool isQualityToSignal(float qidx);
		static bool isQualityToComplete(float qidx);
		static bool isQualityToTarget(float qidx, bool isLensBack = false);
		static bool isQualityToConfirm(float qidx);

		static void onScanOptimizingStarted(void);
		static void onScanOptimizingClosed(void);

	private:
		struct ScanTweakerImpl;
		static std::unique_ptr<ScanTweakerImpl> d_ptr;
		static ScanTweakerImpl& getImpl(void);
	};
}

