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
		CorneaCamera(MainBoard* board, IrCameraType type);
		virtual ~CorneaCamera();

	public:
		virtual bool initializeCorneaCamera(void);

		void applyConfigParameters(CorneaCameraConfigParam param);
		bool applyGainSettings(void) override;

		bool handleFrameData(int index, unsigned char* frameData) override;
		bool loadCalibParamFromProfile(void) override;
		bool saveCalibParamToProfile(void) override;

	private:
		struct CorneaCameraImpl;
		std::unique_ptr<CorneaCameraImpl> d_ptr;
		CorneaCameraImpl& impl(void) const;
	};
}
