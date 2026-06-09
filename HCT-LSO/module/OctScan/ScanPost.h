#pragma once

#include "OctScan2.h"

#include <memory>

namespace sig_chain {
	class OctProcess;
}

namespace oct_device {
	class MainBoard;
}


namespace oct_scan
{
	class OCTSCAN_DLL_API ScanPost
	{
	public:
		ScanPost();
		virtual ~ScanPost();
		
		ScanPost(const ScanPost& rhs) = delete;
		ScanPost& operator=(const ScanPost& rhs) = delete;

		static ScanPost* getInstance(void);
		static std::mutex singleMutex_;

	public:
		void initiateScanPost(wso_device::MainBoard* board, sig_chain::OctProcess* process);
		void releaseScanPost(void);
		bool isInitiated(void) const;
		bool isPosting(void) const;
		bool isProcessingCompleted(void) const;

		bool postMeasure(bool result);

	protected:
		bool startScanPost(void);
		bool closeScanPost(void);

		void threadPostFunction(void);
		bool waitForMeasureProcessed(bool init);
		bool checkIfMeasureProcessed(void);
		bool completeMeasureResult(void);

		sig_chain::OctProcess* getProcess(void);

	private:
		struct ScanPostImpl;
		std::unique_ptr<ScanPostImpl> d_ptr;
		ScanPostImpl& getImpl(void) const;
	};
}

