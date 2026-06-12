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
		std::string getBoardDescript(void) const;

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

		bool readTableDescriptor(const HbsTableDescriptor* data) override;
		bool readBufferDescriptor(const HbsBufferDescriptor* data, const HbsTableDescriptor* desc) override;
		bool readCalibsDescriptor(const HbsCalibsDescriptor* data, const HbsTableDescriptor* desc) override;

		bool readCalibMotorSets(const HbsCalibMotorSets* data, const HbsCalibsDescriptor* desc) override;
		bool readCalibOctParams(const HbsCalibOctParams* data, const HbsCalibsDescriptor* desc) override;
		bool readCalibOctSource(const HbsCalibOctSource* data, const HbsCalibsDescriptor* desc) override;
		bool readCalibOctGalvano(const HbsCalibOctGalvano* data, const HbsCalibsDescriptor* desc) override;
		bool readCalibDeviceCfg(const HbsCalibDeviceCfg* data, const HbsCalibsDescriptor* desc) override;
		bool readCalibStepMotors(const HbsCalibStepMotors* data, const HbsCalibsDescriptor* desc) override;
		bool readCalibFactorySet1(const HbsCalibFactorySet1* data, const HbsCalibsDescriptor* desc) override;
		bool readCalibFactorySet2(const HbsCalibFactorySet2* data, const HbsCalibsDescriptor* desc) override;


		bool readCalibration(const HbsCalibration* data, const HbsTableDescriptor* desc) override;
		bool readConfiguration(const HbsConfiguration* data, const HbsTableDescriptor* desc) override;

		bool readMainBoardVersion(const HbsMainBoardVersion* data, const HbsTableDescriptor* desc) override;
		bool readSystemInitStatus(const HbsSystemInitStatus* data, const HbsTableDescriptor* desc) override;
		bool readSystemConfigure(const HbsSystemConfig* data, const HbsTableDescriptor* desc) override;
		bool readGpioStatus(const HbsGpioStatus* data, const HbsTableDescriptor* desc) override;
		bool readLsoScannerParam(const HbsLsoScanner* data, const HbsTableDescriptor* desc) override;
		bool readSldStatus(const HbsSldStatus* data, const HbsTableDescriptor* desc) override;

		bool readZynqXADC(const HbsZyncXADC* data, const HbsTableDescriptor* desc) override;
		bool readGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsTableDescriptor* desc) override;
		bool readInfraredCameraStatus(const HbsInfraredCameraStatus* data, const HbsTableDescriptor* desc) override;
		bool readStepMotorStatus(const HbsStepMotorStatus* data, StepMotorType type, const HbsTableDescriptor* desc) override;
		bool readStageMotorStatus(const HbsStageMotorStatus* data, const HbsTableDescriptor* desc) override;

		bool writeCalibration(const HbsCalibration* data, const HbsTableDescriptor* desc) override;
		bool writeConfiguration(const HbsConfiguration* data, const HbsTableDescriptor* desc) override;
		bool writeSystemConfigure(const HbsSystemConfig* data, const HbsTableDescriptor* desc) override;
		bool writeGalvanoDynamicParam(const HbsGalvanoDynamicParam* data, const HbsTableDescriptor* desc) override;
		bool writeLsoScannerParam(const HbsLsoScanner* data, const HbsTableDescriptor* desc) override;

		bool writeTrajectoryParam(std::uint8_t tid, const TrajectoryProfileParam* param, const HbsTableDescriptor* desc) override;
		bool writeTrajectoryPositionsX(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsTableDescriptor* desc) override;
		bool writeTrajectoryPositionsY(std::uint8_t tid, const std::int16_t* positions, std::uint16_t count, const HbsTableDescriptor* desc) override;

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

		bool LedSldControl(LightType type, std::uint8_t onOff);
		bool LedSldPotentiometer(LightType type, std::uint8_t channel, std::uint16_t data);
		bool LedSldGetParameters(LightType type, std::uint8_t channel = 4);
		bool LedSldUpdateParameters(LightType type, std::uint8_t channel = 4);

		bool OctReferencePhaseControl(std::uint8_t mode);
		bool OctSetReferencePhase(std::int16_t phase);

		bool CalibDataBlockErase(std::uint16_t region, std::uint16_t blockNum);
		bool CalibDataBlockProgram(std::uint16_t region, std::uint16_t blockNum, std::uint32_t size);
		bool CalibDataBlockLoadFlash(std::uint16_t region, std::uint16_t blockNum, std::uint32_t size);

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

