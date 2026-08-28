#pragma once

#include "WsoDevice2.h"
#include "InfraredCamera.h"
#include <SysConfigDefines.h>




namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API RetinaCamera : public InfraredCamera
	{
	public:
		RetinaCamera();
		RetinaCamera(MainBoard* board, IrCameraType type);
		virtual ~RetinaCamera();

	public:
		virtual bool initializeRetinaCamera(void);

		bool setAnalogGain(float gain, bool control = true);
		bool setDigitalGain(float gain, bool control = true);
		bool setExposureTime(unsigned int time, bool control = true);

		float getAnalogGain(void) const;
		float getDigitalGain(void) const;
		unsigned int getExposureTime(void) const;

		void applyConfigParameters(RetinaCameraConfigParam param);
		bool applyGainSettings(void) override;

		bool handleFrameData(int index, unsigned char* frameData) override;
		bool loadCalibParamFromProfile(void) override;
		bool saveCalibParamToProfile(void) override;

	private:
		struct RetinaCameraImpl;
		std::unique_ptr<RetinaCameraImpl> d_ptr;
		RetinaCameraImpl& impl(void) const;
	};
}

