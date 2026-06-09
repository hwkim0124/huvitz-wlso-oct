#pragma once

#include "OctScan2.h"

#include <memory>


namespace wso_device
{
	class MainBoard;
}


namespace oct_scan
{
	class OCTSCAN_DLL_API ScanOptimizer
	{
	public:
		ScanOptimizer();
		virtual ~ScanOptimizer();

		ScanOptimizer(const ScanOptimizer& rhs) = delete;
		ScanOptimizer& operator=(const ScanOptimizer& rhs) = delete;

		static ScanOptimizer* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool initializeScanOptimizer(wso_device::MainBoard * board);
		bool isInitialized(void);
		
		bool initiateAutoScanOptimize(bool on_cornea);
		bool initiateAutoPosition(void);

		bool initiateAutoDiopterFocus(OctAutoFocusOptimized callback = nullptr);
		bool initiateAutoPolarization(OctAutoPolarOptimized callback = nullptr);
		bool initiateAutoReference(OctAutoReferOptimized callback = nullptr, bool on_cornea = false);

		void ceaseAutoReference(bool wait);
		void ceaseAutoDiopterFocus(bool wait);
		void ceaseAutoPolarization(bool wait);
		void ceaseOptimizing(void);

		bool isAutoDiopterFocusing(void);
		bool isAutoPolarizing(void);
		bool isAutoReferencing(void);

		bool isOptimizing(void);
		bool isCancelling(void);
		bool isCompleted(void);


	protected:
		wso_device::MainBoard* getMainBoard(void);

		void threadAutoOptimizeFunction(void);
		void threadAutoPositionFunction(void);

	private:
		struct ScanOptimizerImpl;
		std::unique_ptr<ScanOptimizerImpl> d_ptr;
		ScanOptimizerImpl& impl(void);

	};
}
