#pragma once

#include "WsoDevice2.h"
#include "InfraredCamera.h"




namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API RetinaCamera : public InfraredCamera
	{
	public:
		RetinaCamera();
		RetinaCamera(MainBoard* board, CameraType type, std::uint8_t epid);
		virtual ~RetinaCamera();

	public:
		virtual bool initializeRetinaCamera(void);

		virtual bool setAnalogGain(float gain, bool control = true) override;
		virtual bool setDigitalGain(float gain, bool control = true) override;

		bool loadCalibParamFromProfile(void) override;
		bool saveCalibParamToProfile(void) override;

		virtual bool loadConfigFromIniFile(void) override;
		virtual bool saveConfigToIniFile(void) override;

	private:
		struct RetinaCameraImpl;
		std::unique_ptr<RetinaCameraImpl> d_ptr;
		RetinaCameraImpl& impl(void) const;
	};
}

