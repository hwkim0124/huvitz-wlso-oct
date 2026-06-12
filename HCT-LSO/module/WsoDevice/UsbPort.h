#pragma once

#include "WsoDevice2.h"
#include "FTD3XX.h"

#include <memory>
#include <string>


namespace wso_device
{
	class WSODEVICE_DLL_API UsbPort
	{
	public:
		UsbPort();
		virtual ~UsbPort();

		UsbPort(UsbPort&& rhs);
		UsbPort& operator=(UsbPort&& rhs);
		// Prevent copy construction and assignment. 
		UsbPort(const UsbPort& rhs) = delete;
		UsbPort& operator=(const UsbPort& rhs) = delete;

	public:
		bool openPort(const std::string& desc = USB_MAIN_BOARD_DESC, bool useNotificationCallback = false, unsigned long timeout = USB_TIMEOUT,
			unsigned char readPid = USB_READ_PID, unsigned char writePid = USB_WRITE_PID,
			unsigned char readIr1 = USB_IR1_PID, unsigned char readIr2 = USB_IR2_PID);
		void closePort(void);
		void clearPort(void);

		bool isOpened(void) const;
		bool isAvailable(unsigned char pid) const;
		void setPipeError(unsigned char pid, bool flag);
		bool isPipeError(unsigned char pid) const;

		PVOID getHandle(void) const;
		void setTimeout(unsigned long timeout);
		void setReadPipeID(unsigned char pid);
		void setWritePipeID(unsigned char pid);

		void abortPipe(unsigned char pid);

		bool readGPIO(std::uint32_t* status);
		bool receive(unsigned char* buff, unsigned long size, bool check = true);
		bool receiveFromIr1(unsigned char* buff, unsigned long size, bool check = true);
		bool receiveFromIr2(unsigned char* buff, unsigned long size, bool check = true);
		bool receiveFromSlo(unsigned char* buff, unsigned long size, bool check = true);

		bool receive(unsigned char epid, unsigned char* buff, unsigned long size, bool check = true);
		bool receive2(unsigned char epid, unsigned char* buff, unsigned long size, bool check = true);
		bool send(unsigned char* buff, unsigned long size, bool check = true);

	protected:
		int getIndexOfPipe(unsigned char pid) const;
		static VOID NotificationCallback(PVOID pvCallbackContext, E_FT_NOTIFICATION_CALLBACK_TYPE eCallbackType, PVOID pvCallbackInfo);

	private:
		struct UsbPortImpl;
		std::unique_ptr<UsbPortImpl> d_ptr;
		UsbPortImpl& impl(void) const;
	};

}

