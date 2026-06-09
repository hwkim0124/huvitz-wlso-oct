#pragma once

#include "WsoSystem2.h"

#include <memory>
#include <mutex>


namespace wso_system
{
	using namespace wso_domain;

	
	class WSOSYSTEM_DLL_API Bootstrapper
	{
	public:
		Bootstrapper();
		virtual ~Bootstrapper();

		Bootstrapper(const Bootstrapper& rhs) = delete;
		Bootstrapper& operator=(const Bootstrapper& rhs) = delete;

		static Bootstrapper* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool initializeWsoSystem(WsoLogMsgCallback clb, bool trace_mode);
		bool isWsoSystemInitialized(void) const;
		void releaseWsoSystem(void);

		bool initSystemLogger(void);
		bool initLogMsgCallback(WsoLogMsgCallback clb, bool trace_mode);

		void connectWsoLogMsgCallback(WsoLogMsgCallback clb);
		void releaseWsoLogMsgCallback(void);

		bool initializeHardware(void);
		bool initOctScanning(void);

		bool implementSystemCalibration(void);
		bool implementSystemConfiguration(void);

	protected:
		void establishSystemBootMode(void);
		void parseCommandLineArguments(void);

	private:
		struct BootstrapperImpl;
		std::unique_ptr<BootstrapperImpl> d_ptr;
		BootstrapperImpl& impl(void) const;
	};
}

