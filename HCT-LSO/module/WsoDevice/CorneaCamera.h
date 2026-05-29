#pragma once

#include "WsoDevice2.h"
#include "InfraredCamera.h"




namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API CorneaCamera : public InfraredCamera
	{
	public:
		CorneaCamera();
		CorneaCamera(MainBoard* board, CameraType type, InfraredCameraId camId, std::uint8_t epid);
		virtual ~CorneaCamera();

	public:
		virtual bool initializeCorneaCamera(void);

		virtual bool setAnalogGain(float gain, bool control = true) override;
		virtual bool setDigitalGain(float gain, bool control = true) override;

		bool loadCalibParamFromProfile(void) override;
		bool saveCalibParamToProfile(void) override;

		virtual bool loadConfigFromIniFile(void) override;
		virtual bool saveConfigToIniFile(void) override;

	private:
		struct CorneaCameraImpl;
		std::unique_ptr<CorneaCameraImpl> d_ptr;
		CorneaCameraImpl& impl(void) const;
	};
}

