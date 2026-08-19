#include "pch.h"
#include "UsbPort.h"

#include "FTD3XX.h"

#include <chrono>
#include <mutex>
#include <thread>

using namespace wso_device;
using namespace std;


// FT_STATUS is an unvalued enum, so a raw value in a log tells nobody anything.
// The values worth naming here are the ones the stream pipes actually produce:
// 19 when the camera is not sending, 20 when a pending read was cancelled.
static const char* getFtStatusName(FT_STATUS status)
{
	switch (status) {
	case FT_OK:						return "FT_OK";
	case FT_INVALID_HANDLE:			return "FT_INVALID_HANDLE";
	case FT_DEVICE_NOT_FOUND:		return "FT_DEVICE_NOT_FOUND";
	case FT_DEVICE_NOT_OPENED:		return "FT_DEVICE_NOT_OPENED";
	case FT_IO_ERROR:				return "FT_IO_ERROR";
	case FT_INSUFFICIENT_RESOURCES:	return "FT_INSUFFICIENT_RESOURCES";
	case FT_INVALID_PARAMETER:		return "FT_INVALID_PARAMETER";
	case FT_INVALID_ARGS:			return "FT_INVALID_ARGS";
	case FT_NOT_SUPPORTED:			return "FT_NOT_SUPPORTED";
	case FT_TIMEOUT:				return "FT_TIMEOUT";
	case FT_OPERATION_ABORTED:		return "FT_OPERATION_ABORTED";
	case FT_RESERVED_PIPE:			return "FT_RESERVED_PIPE";
	case FT_IO_PENDING:				return "FT_IO_PENDING";
	case FT_IO_INCOMPLETE:			return "FT_IO_INCOMPLETE";
	case FT_BUSY:					return "FT_BUSY";
	case FT_NO_SYSTEM_RESOURCES:	return "FT_NO_SYSTEM_RESOURCES";
	case FT_DEVICE_NOT_CONNECTED:	return "FT_DEVICE_NOT_CONNECTED";
	case FT_OTHER_ERROR:			return "FT_OTHER_ERROR";
	default:						return "FT_UNKNOWN";
	}
}


// FT_HANDLE SloMain_FT601_Handle = nullptr;

#define NOTIFICATION_RX_BUFFER_SIZE 1024
#define NOTIFICATION_MSG_PKT_SIZE 12
#define NOTIFICATION_MSG_HEADER 0xD0
#define NOTIFICAITON_INFO_FIELD_SIZE 4

#define JOYSTICK_BUTTON_PRESSED 0
#define OPTIMIZE_BUTTON_PRESSED 1
#define MAINBOARD_SW_INTERRUPT 2


typedef struct _NOTIFICATION_CONTEXT
{
	FT_HANDLE ftHandle;
	UCHAR ucRecvBuffer[NOTIFICATION_RX_BUFFER_SIZE];
	DWORD32 ulCurrentRecvData;
	FT_STATUS ftStatus;
	UCHAR ucEndpointNo;

} NOTIFICATION_CONTEXT;

typedef NOTIFICATION_CONTEXT* PNOTIFICATION_CONTEXT;


struct UsbPort::UsbPortImpl
{
	FT_HANDLE handle;
	NOTIFICATION_CONTEXT notificationContext = { 0 };

	string strDesc;
	unsigned char pidReadCmd;
	unsigned char pidWriteCmd;
	unsigned char pidReadIr1;
	unsigned char pidReadIr2;
	unsigned char pidReadSlo;
	unsigned char pidNotification;
	unsigned long timeout;

	bool isErrorPipes[4];
	mutex mutexPort;

	UsbPortImpl() : handle(nullptr), timeout(USB_TIMEOUT), isErrorPipes{ false } {
	}
};


UsbPort::UsbPort() :
	d_ptr(make_unique<UsbPortImpl>())
{
}


UsbPort::~UsbPort()
{
}


// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::UsbPort::UsbPort(UsbPort&& rhs) = default;
UsbPort& wso_device::UsbPort::operator=(UsbPort&& rhs) = default;


UsbPort::UsbPortImpl& wso_device::UsbPort::impl(void) const
{
	return *d_ptr;
}



bool wso_device::UsbPort::openPort(const std::string& desc, bool useNotificationCallback, unsigned long timeout, unsigned char readPid,
	unsigned char writePid, unsigned char readIr1, unsigned char readIr2)
{
	closePort();

	lock_guard<mutex> lock(impl().mutexPort);

	impl().strDesc = desc;
	impl().timeout = timeout;
	impl().pidReadCmd = readPid;
	impl().pidWriteCmd = writePid;
	impl().pidReadIr1 = readIr1;
	impl().pidReadIr2 = readIr2;
	impl().pidReadSlo = readIr2;
	// Was never assigned, yet closePort() passes it to FT_FlushPipe/setPipeError.
	// getIndexOfPipe() maps an unknown pid to index 0, so an uninitialized value
	// there silently clears the command-read error flag.
	impl().pidNotification = USB_NOTI_PID;

	char argv[64] = { 0 };
	memcpy(argv, desc.c_str(), desc.size());

	try {
		FT_STATUS ret = FT_Create(argv, FT_OPEN_BY_DESCRIPTION, (FT_HANDLE*)&impl().handle);
		if (FT_FAILED(ret)) {
			clearPort();
		}
		else {
			FT_SetPipeTimeout(impl().handle, impl().pidReadCmd, impl().timeout);
			FT_SetPipeTimeout(impl().handle, impl().pidWriteCmd, impl().timeout);
			// The camera stream pipes are deliberately left at the driver default
			// timeout. A 12s timeout there means an acquisition worker blocked on a
			// non-streaming camera cannot be stopped without cancelling its read,
			// and cancelling is what produces FT_OPERATION_ABORTED.

			FT_FlushPipe(impl().handle, impl().pidReadCmd);
			FT_FlushPipe(impl().handle, impl().pidWriteCmd);
			FT_FlushPipe(impl().handle, impl().pidReadIr1);
			FT_FlushPipe(impl().handle, impl().pidReadIr2);

			// SloMain_FT601_Handle = impl().handle;

			//Set bit0,bit1 GPIO as Input.
			ret = FT_EnableGPIO(impl().handle, 0x3, 0x0);
			if (FT_FAILED(ret)) {
				LogDebug() << "GPIO port not enabled!";
				throw exception();
			}

			//0: pull down,1: Hi-z,2:pull up, 3: Hi-z
			ret = FT_SetGPIOPull(impl().handle, 0x3, 0x2);
			if (FT_FAILED(ret)) {
				LogDebug() << "GPIO port not pulled!";
				throw exception();
			}

			if (useNotificationCallback)
			{
				impl().notificationContext.ftHandle = impl().handle;
				impl().notificationContext.ucEndpointNo = 0x85;
				ret = FT_SetNotificationCallback(impl().handle, NotificationCallback, &impl().notificationContext);
				if (FT_FAILED(ret)) {
					LogDebug() << "Notification callback not initialized!";
					throw exception();
				}
			}
			return true;
		}
	}
	catch (...) {
		// throw runtime_error("FT_Cread() error!");
	}
	LogDebug() << "UsbPort::open() failed!";
	return false;
}


void wso_device::UsbPort::closePort(void)
{
	lock_guard<mutex> lock(impl().mutexPort);

	if (isOpened()) {
		try {
			FT_FlushPipe(impl().handle, impl().pidReadCmd);
			FT_FlushPipe(impl().handle, impl().pidWriteCmd);
			FT_FlushPipe(impl().handle, impl().pidReadIr1);
			FT_FlushPipe(impl().handle, impl().pidReadIr2);
			FT_FlushPipe(impl().handle, impl().pidNotification);

			FT_ClearNotificationCallback(impl().handle);
			/*
			FT_ClearStreamPipe(impl().handle, FALSE, TRUE, impl().pidReadIr1);
			FT_ClearStreamPipe(impl().handle, FALSE, TRUE, impl().pidReadIr2);
			*/

			//FT_AbortPipe(impl().handle, impl().pidReadCmd);
			//FT_AbortPipe(impl().handle, impl().pidWriteCmd);
			//FT_AbortPipe(impl().handle, impl().pidReadIr1);
			//FT_AbortPipe(impl().handle, impl().pidReadIr2);

			FT_Close(impl().handle);
			impl().handle = nullptr;

			setPipeError(impl().pidReadCmd, false);
			setPipeError(impl().pidWriteCmd, false);
			setPipeError(impl().pidReadIr1, false);
			setPipeError(impl().pidReadIr2, false);
			setPipeError(impl().pidNotification, false);
		}
		catch (...) {
			throw runtime_error("FT_Close() error!");
		}
	}
	return;
}


void wso_device::UsbPort::clearPort(void)
{
	closePort();
	return;
}


void wso_device::UsbPort::setPipeError(unsigned char pid, bool flag)
{
	int idx = getIndexOfPipe(pid);
	impl().isErrorPipes[idx] = flag;
	return;
}


bool wso_device::UsbPort::isPipeError(unsigned char pid) const
{
	int idx = getIndexOfPipe(pid);
	return impl().isErrorPipes[idx];
}


bool wso_device::UsbPort::isAvailable(unsigned char pid) const
{
	return (isOpened() && !isPipeError(pid));
}


bool wso_device::UsbPort::isOpened(void) const
{
	return (impl().handle != nullptr);
}


PVOID wso_device::UsbPort::getHandle(void) const
{
	return impl().handle;
}


void wso_device::UsbPort::setTimeout(unsigned long timeout)
{
	impl().timeout = timeout;
	return;
}


void wso_device::UsbPort::setReadPipeID(unsigned char pid)
{
	impl().pidReadCmd = pid;
	return;
}


void wso_device::UsbPort::setWritePipeID(unsigned char pid)
{
	impl().pidWriteCmd = pid;
	return;
}


void wso_device::UsbPort::abortPipe(unsigned char pid)
{
	if (isOpened()) {
		FT_AbortPipe(impl().handle, pid);
	}
	return;
}


bool wso_device::UsbPort::readGPIO(std::uint32_t* status)
{
	if (!isOpened()) {
		return false;
	}

	FT_STATUS ret;
	ret = FT_ReadGPIO(impl().handle, status);
	if (ret != FT_OK) {
		return false;
	}
	return true;
}


bool wso_device::UsbPort::receive(unsigned char* buff, unsigned long size, bool check)
{
	uint8_t epid = impl().pidReadCmd;
	bool ret = receive2(epid, buff, size, check);
	if (!ret) {
		setPipeError(impl().pidWriteCmd, true);
		setPipeError(impl().pidReadCmd, true);
	}
	return ret;
}


bool wso_device::UsbPort::receiveFromIr1(unsigned char* buff, unsigned long size, bool check)
{
	uint8_t epid = impl().pidReadIr1;
	bool ret = receiveStream(epid, buff, size);
	return ret;
}


bool wso_device::UsbPort::receiveFromIr2(unsigned char* buff, unsigned long size, bool check)
{
	uint8_t epid = impl().pidReadIr2;
	bool ret = receiveStream(epid, buff, size);
	return ret;
}


bool wso_device::UsbPort::receiveFromSlo(unsigned char* buff, unsigned long size, bool check)
{
	uint8_t epid = impl().pidReadSlo;
	bool ret = receiveStream(epid, buff, size);
	return ret;
}


// Bulk-IN read for the camera stream pipes.
//
// Two properties matter here and both differ from receive()/receive2():
//
//  - A frame is read in USB_STREAM_CHUNK_SIZE pieces. A zero-length chunk in the
//    middle of a frame is not an error, it just means the sender has not caught up
//    yet: yield and keep reading, because dropping a half-read frame would leave
//    the pipe offset by the unread remainder and every later frame would fail the
//    line-header check.
//  - A failure leaves the pipe untouched: no FT_AbortPipe, no pipe-error latch.
//    Aborting cancels any read still pending on that pipe (FT_OPERATION_ABORTED),
//    and the latch is only ever cleared by closePort(), so a single stalled read
//    used to disable the camera for the rest of the session. Callers recover by
//    simply reading again.
bool wso_device::UsbPort::receiveStream(unsigned char epid, unsigned char* buff, unsigned long size)
{
	if (!isAvailable(epid)) {
		return false;
	}

	unsigned long n_total = 0;
	while (n_total < size) {
		unsigned long n_left = size - n_total;
		unsigned long n_want = (n_left < USB_STREAM_CHUNK_SIZE ? n_left : USB_STREAM_CHUNK_SIZE);
		unsigned long n_read = 0;

		FT_STATUS ret = FT_ReadPipe(impl().handle, epid, buff + n_total, n_want, &n_read, NULL);
		if (FT_FAILED(ret)) {
			LogDebug() << "FT_ReadPipe() failed, epid: " << static_cast<int>(epid)
				<< ", status: " << getFtStatusName(ret) << " (" << ret << ")";
			return false;
		}

		if (n_read == 0) {
			if (n_total == 0) {
				// Nothing at all: the camera is not streaming yet. Not an error.
				return false;
			}
			this_thread::sleep_for(chrono::milliseconds(1));
			continue;
		}
		n_total += n_read;
	}
	return true;
}


bool wso_device::UsbPort::receive(unsigned char epid, unsigned char* buff, unsigned long size, bool check)
{
	if (!isAvailable(epid)) {
		return false;
	}

	// lock_guard<mutex> lock(impl().mutexPort);

	unsigned char* p = buff;
	unsigned long n_read = 0;
	unsigned long n_data = size;

	try
	{
		while (n_data > 0) {
			FT_STATUS ret = FT_ReadPipe(impl().handle, epid, p, n_data, &n_read, NULL);
			if (FT_SUCCESS(ret)) {
				if (check == false) {
					return true;
				}
				else {
					if (n_read <= 0) {
						// Timeout??
						// return false ;
						LogDebug() << "FT_ReadPipe() transferred data with zero length";
						break;
					}
					else {
						n_data -= n_read;
						if (n_data <= 0) {
							return true;
						}
						else {
							p += n_read;
						}
					}
				}
			}
			else {
				// If the timeout (default 5 seconds) occurred, FT_ReadPipe returns with
				// an error code FT_TIMEOUT.
				LogDebug() << "FT_ReadPipe() returned with error: " << getFtStatusName(ret) << " (" << ret << ")";
				break;
			}
		}
	}
	catch (...) {
		throw runtime_error("FT_ReadPipe() error!");
	}
	LogDebug() << "UsbPort::receive() failed!, epid: " << static_cast<int>(epid);

	FT_AbortPipe(impl().handle, epid);
	setPipeError(epid, true);
	return false;
}


bool wso_device::UsbPort::receive2(unsigned char epid, unsigned char* buff, unsigned long size, bool check)
{
	if (!isAvailable(epid)) {
		return false;
	}

	// lock_guard<mutex> lock(impl().mutexPort);

	unsigned char* p = buff;
	unsigned long n_read = 0;
	unsigned long n_data = size;

	OVERLAPPED overlapped = { 0 };
	FT_STATUS ftStatus = FT_InitializeOverlapped(impl().handle, &overlapped);

	try
	{
		while (n_data > 0) {
			ftStatus = FT_ReadPipe(impl().handle, epid, p, n_data, &n_read, &overlapped);

			if (FT_IO_PENDING == ftStatus) {
				ftStatus = FT_GetOverlappedResult(impl().handle, &overlapped, &n_read, TRUE);

				if (FT_SUCCESS(ftStatus)) {
					if (check == false) {
						return true;
					}
					else {
						if (n_read <= 0) {
							// Timeout??
							// return false ;
							LogDebug() << "FT_GetOverlappedResult() transferred data with zero length";
							break;
						}
						else {
							n_data -= n_read;
							if (n_data <= 0) {
								return true;
							}
							else {
								p += n_read;
							}
						}
					}
				}
				else {
					LogDebug() << "FT_GetOverlappedResult() returned with error: " << getFtStatusName(ftStatus) << " (" << ftStatus << ")";
					break;
				}
			}
			else {
				// If the timeout (default 5 seconds) occurred, FT_ReadPipe returns with
				// an error code FT_TIMEOUT.
				LogDebug() << "FT_ReadPipe() returned with error: " << getFtStatusName(ftStatus) << " (" << ftStatus << ")";
				break;
			}
		}
	}
	catch (...) {
		throw runtime_error("FT_ReadPipe() error!");
	}

	FT_ReleaseOverlapped(impl().handle, &overlapped);

	LogDebug() << "UsbPort::receive2() failed!, epid=%x" << epid;

	FT_AbortPipe(impl().handle, epid);
	setPipeError(epid, true);
	return false;
}


bool wso_device::UsbPort::send(unsigned char* buff, unsigned long size, bool check)
{
	unsigned char epid = impl().pidWriteCmd;
	if (!isAvailable(epid)) {
		return false;
	}

	// lock_guard<mutex> lock(impl().mutexPort);

	unsigned char* p = buff;
	unsigned long n_sent = 0;
	unsigned long n_data = size;

	try
	{
		while (n_data > 0) {
			FT_STATUS ret = FT_WritePipe(impl().handle, epid, p, n_data, &n_sent, NULL);
			if (FT_SUCCESS(ret)) {
				if (check == false) {
					return true;
				}
				else {
					if (n_sent <= 0) {
						// Timeout??
						// return false;
						LogDebug() << "FT_WritePipe() transferred data with zero length";
						break;
					}
					else {
						n_data -= n_sent;
						if (n_data <= 0) {
							return true;
						}
						else {
							p += n_sent;
						}
					}
				}
			}
			else {
				LogDebug() << "FT_WritePipe() returned with error: " << getFtStatusName(ret) << " (" << ret << ")";
				break;
			}
		}
	}
	catch (...) {
		throw runtime_error("FT_WritePipe() error!");
	}
	LogDebug() << "UsbPort::send() failed!, epid: " << static_cast<int>(epid);

	FT_AbortPipe(impl().handle, epid);
	setPipeError(epid, true);
	return false;
}


int wso_device::UsbPort::getIndexOfPipe(unsigned char pid) const
{
	int idx = 0;
	if (pid == impl().pidReadCmd) {
		idx = 0;
	}
	else if (pid == impl().pidWriteCmd) {
		idx = 1;
	}
	else if (pid == impl().pidReadIr1) {
		idx = 2;
	}
	else if (pid == impl().pidReadIr2) {
		idx = 3;
	}
	return idx;
}


VOID wso_device::UsbPort::NotificationCallback(PVOID pvCallbackContext, E_FT_NOTIFICATION_CALLBACK_TYPE eCallbackType, PVOID pvCallbackInfo)
{
	ULONG ulBytesTransferred = 0;
	FT_STATUS ftStatus;
	PNOTIFICATION_CONTEXT pNotificationContext = (PNOTIFICATION_CONTEXT)pvCallbackContext;
	FT_NOTIFICATION_CALLBACK_INFO_DATA* pInfo = (FT_NOTIFICATION_CALLBACK_INFO_DATA*)pvCallbackInfo;

	switch (eCallbackType) {
		case E_FT_NOTIFICATION_CALLBACK_TYPE_DATA: {
			if (pInfo)
			{
				if (pInfo->ulRecvNotificationLength != NOTIFICATION_MSG_PKT_SIZE) {
					printf(" invalid packet size=%d\n", pInfo->ulRecvNotificationLength);
					//	break;
				}

				ftStatus = FT_ReadPipe(pNotificationContext->ftHandle, pInfo->ucEndpointNo, &pNotificationContext->ucRecvBuffer[0],
					pInfo->ulRecvNotificationLength, &ulBytesTransferred, NULL);

				if (FT_FAILED(ftStatus)) {
					printf("NotificationCallback FT_ReadPipe failed 0x%x\n", ftStatus);
				}
				else {
					if (ulBytesTransferred != NOTIFICATION_MSG_PKT_SIZE) {
						printf("invalid Notification message FT_READ size=%d\n", ulBytesTransferred);
					}

					printf(" Noti. Msg packet  %d bytes read done!\n", ulBytesTransferred);
					UCHAR* data = &pNotificationContext->ucRecvBuffer[0];
					if (data[0] != NOTIFICATION_MSG_HEADER) {
					}
					else {
						if (data[2] != NOTIFICAITON_INFO_FIELD_SIZE) {
						}
						else {
							switch (data[3]) {
							case JOYSTICK_BUTTON_PRESSED:
								LogD() << "Joystick button pressed";
								// wso_domain::CallbackRegistry::getInstance()->runJoystickButtonPressed();
								break;

							case OPTIMIZE_BUTTON_PRESSED:
								LogD() << "Optimize button pressed";
								// wso_domain::CallbackRegistry::getInstance()->runOptimizeButtonPressed();
								break;

							case MAINBOARD_SW_INTERRUPT:
								// printf("  :MB software interrupt,MSG info=%d\n", data[4]);//Data[4] Contains cause of sw interrupt...
								LogD() << "Software interrupt";
								unsigned short msg_type;
								unsigned short msg_data;
								memcpy(&msg_data, &data[4], sizeof(unsigned short));
								memcpy(&msg_type, &data[6], sizeof(unsigned short));
								if (msg_type == 0) {
									LogD() << "Adapter lens attached, type=" << msg_data;
									// wso_domain::CallbackRegistry::getInstance()->runAdapterLensAttached(msg_data);
								}
								break;
							default:
								break;
							}
						}
					}
					pNotificationContext->ulCurrentRecvData = ulBytesTransferred;
				}
				pNotificationContext->ftStatus = ftStatus;
			}
			break;
		}
		default: {
			break;
		}
	}
	return;
}
