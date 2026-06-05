#pragma once

#include "WsoDevice2.h"
#include <string>

namespace wso_device
{
	class MainBoard;
	class UsbComm;

	class WSODEVICE_DLL_API FirmwareControl
	{
	public:
		FirmwareControl();
		FirmwareControl(MainBoard* board, UsbComm* usbComm);
		virtual ~FirmwareControl();

		FirmwareControl(FirmwareControl&& rhs);
		FirmwareControl& operator=(FirmwareControl&& rhs);
		FirmwareControl(const FirmwareControl& rhs) = delete;
		FirmwareControl& operator=(const FirmwareControl& rhs) = delete;

		bool writeUpgradeFirmware(unsigned char* binary, int length, unsigned int targetAddress, unsigned int binCrc);
		int waitForState(int state, int timeoutMs);
		int waitWhileState(int state, int timeoutMs, int flieSize);
		bool isState(int state);
		int getState();
		void writeFirmwareThread();

	private:
		struct FirmwareControlImpl;
		std::unique_ptr<FirmwareControlImpl> d_ptr;
		FirmwareControlImpl& impl(void) const;
	};
}


