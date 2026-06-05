#pragma once

#include "WsoDevice2.h"
#include "UsbPacket.h"


#include <memory>
#include <string>


namespace wso_board
{
	class HbsDataComm;
}

namespace wso_device
{
	using namespace wso_board;

	class UsbPort;

	class WSODEVICE_DLL_API UsbComm : public HbsDataComm
	{
	public:
		UsbComm();
		virtual ~UsbComm();

		UsbComm(UsbComm&& rhs);
		UsbComm& operator=(UsbComm&& rhs);

		// Prevent copy construction and assignment. 
		UsbComm(const UsbComm& rhs) = delete;
		UsbComm& operator=(const UsbComm& rhs) = delete;

	public:
		void setBoardDescript(string str);
		bool openChannel(bool setNotiCallback);
		void releaseChannel(void);
		bool isInitiated(void) const;
		bool isRestoring(void) const;
		void setCommandAsyncMode(bool flag);
		bool isCommandAsyncMode(void) const;
		UsbPort* getUsbPort(void) const;

		bool sendMsgCmd(MsgCommand* msgCmd, bool reply = true);
		bool recvMsgCmdRes(void);
		bool readAddress(std::uint32_t addr, std::uint8_t* buff, std::uint32_t size);
		bool writeAddress(std::uint32_t addr, std::uint8_t* buff, std::uint32_t size, bool reply = true);

		bool readIrCameraFrame(CameraType type, std::uint8_t* buff, std::uint32_t size);
		bool readSloScanFrame(std::uint8_t* buff, std::uint32_t size);
		bool checkSystemReadyGPIO(void);
		bool checkEyeSideGPIO(EyeSide& side);

		bool readDescriptor(const HbsDescriptor* data) override;
		bool readBulkBuffer(const HbsBulkBuffer* data, const HbsDescriptor* desc) override;
		bool readCalibration(const HbsCalibration* data, const HbsDescriptor* desc) override;
		bool readConfiguration(const HbsConfiguration* data, const HbsDescriptor* desc) override;

		bool readMainBoardVersion(const HbsMainBoardVersion* data, const HbsDescriptor* desc) override;
		bool readSystemInitStatus(const HbsSystemInitStatus* data, const HbsDescriptor* desc) override;
		bool readSystemConfigure(const HbsSystemConfigure* data, const HbsDescriptor* desc) override;
		bool readGpioStatus(const HbsGpioStatus* data, const HbsDescriptor* desc) override;
		bool readLsoScannerParam(const HbsLsoScanner* data, const HbsDescriptor* desc) override;
		bool readSldStatus(const HbsSldStatus* data, const HbsDescriptor* desc) override;

		bool readZynqXADC(const HbsZyncXADC* data, const HbsDescriptor* desc) override;
		bool readGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsDescriptor* desc) override;
		bool readInfraredCameraStatus(const HbsInfraredCameraStatus* data, const HbsDescriptor* desc) override;
		bool readStepMotorStatus(const HbsStepMotorStatus* data, StepMotorType type, const HbsDescriptor* desc) override;
		bool readStageMotorStatus(const HbsStageMotorStatus* data, const HbsDescriptor* desc) override;

		bool writeCalibration(const HbsCalibration* data, const HbsDescriptor* desc) override;
		bool writeConfiguration(const HbsConfiguration* data, const HbsDescriptor* desc) override;
		bool writeSystemConfigure(const HbsSystemConfigure* data, const HbsDescriptor* desc) override;
		bool writeGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsDescriptor* desc) override;
		bool writeLsoScannerParam(const HbsLsoScanner* data, const HbsDescriptor* desc) override;

		bool writeTrajectoryParam(std::uint8_t tid, const TrajectoryProfileParam* param, const HbsDescriptor* desc) override;
		bool writeTrajectoryPositionsX(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsDescriptor* desc) override;
		bool writeTrajectoryPositionsY(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsDescriptor* desc) override;

		bool SysCalibLoad(std::uint16_t offset, std::uint16_t size);
		bool SysCalibSave(std::uint16_t offset, std::uint16_t size);
		bool SysCalibLoad2(std::uint16_t offset, std::uint16_t size);
		bool SysCalibSave2(std::uint16_t offset, std::uint16_t size);

		bool MotorJogg(std::uint8_t mid, int delta);
		bool MotorMove(std::uint8_t mid, int pos);
		bool MotorHome(std::uint8_t mid);
		bool MotorStop(std::uint8_t mid);

		//motor velocity move
		bool MotorMoveVelocity(std::uint8_t mid, std::uint8_t dir);
		bool MotorStopVelocity(std::uint8_t mid);
		bool WriteMotorSpeedVelocity(std::uint8_t mid, std::uint32_t accelStep, std::uint32_t minSpeed, std::uint32_t maxSpeed);
		bool MotorSpeedDefaultVelocity(std::uint8_t mid);

		bool StageMove(StageMotorType type, std::uint8_t dir, std::uint8_t duty);
		bool StageStop(StageMotorType type);
		bool ChinrestMove(std::uint8_t dir);
		bool ChinrestStop(void);

		bool IrCameraControl(std::uint8_t cid, std::uint8_t onoff);
		bool IrCameraDigitalGain(std::uint8_t cid, std::uint8_t gain);
		bool IrCameraAnalogGain(std::uint8_t cid, std::uint8_t gain);
		bool IrCameraExposureTime(std::uint8_t cid, std::uint16_t ints);

		bool TestCommand(std::uint8_t cmd);

		bool LsoScannerControl(std::uint8_t patternId, std::uint8_t onOff);
		bool LsoGalvanoMoveY(std::int16_t ypos);
		bool LsoScannerTriggerControl(std::uint8_t onOff);

		bool LedSetIntensity(LightType type, std::uint8_t value);
		bool LedSetMode(LightType type, std::uint8_t value);

		bool GalvanoMoveX(short x);
		bool GalvanoSlewXY(short x, short y);
		bool GalvanoDynamicOffsetWrite(std::int16_t offsetX, std::int16_t offsetY);
		bool GalvanoDynamicOffsetRead(std::int16_t& offsetX, std::int16_t& offsetY);

		bool GalvanoScanXY(std::uint16_t sid, std::uint16_t eid);
		bool GalvanoRasterX(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs);
		bool GalvanoRasterY(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs);
		bool GalvanoRasterXY(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs);
		bool GalvanoRasterFastX(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs);
		bool GalvanoRasterFastY(std::uint16_t tid, std::uint16_t lines, std::int16_t xoffs, std::int16_t yoffs);

		bool LcdFixationControl(std::uint8_t row, std::uint8_t col);
		bool LcdFixationParameters(std::uint8_t bright, std::uint8_t blink, std::uint16_t period, std::uint16_t ontime, std::uint8_t type);

		bool FpgaWrite(std::uint32_t buff_addr, std::uint32_t flash_addr, std::uint32_t size, std::uint32_t calc_crc);

	protected:
		MsgCommand* getMsgCommand(CommandType ctrl, std::uint8_t msg_len);
		ReadRequest* getReadRequest(std::uint32_t addr, std::uint16_t rd_size);
		ReadResponse* getReadResponse(std::uint16_t rd_size);
		WriteRequest* getWriteRequest(std::uint32_t addr, std::uint8_t* data, std::uint16_t size, bool reply = true);
		WriteResponse* getWriteResponse(void);

		void attachCRC(std::uint8_t* addr);
		void attachCRC(std::uint8_t* addr, std::uint32_t offset);
		unsigned char getNextPacketID(void);
		unsigned int getWordChecksum(std::uint32_t* baseAddr, std::uint32_t wordCount);

	private:
		struct UsbCommImpl;
		std::unique_ptr<UsbCommImpl> d_ptr;
		UsbCommImpl& impl(void) const;
	};
}

