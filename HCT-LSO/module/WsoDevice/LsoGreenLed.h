#pragma once
#include "WsoDevice2.h"
#include "LightLed.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API LsoGreenLed : public LightLed
	{
	public:
		LsoGreenLed();
		LsoGreenLed(MainBoard* board);
		virtual ~LsoGreenLed();

		LsoGreenLed(LsoGreenLed&& rhs);
		LsoGreenLed& operator=(LsoGreenLed&& rhs);
		LsoGreenLed(const LsoGreenLed& rhs);
		LsoGreenLed& operator=(const LsoGreenLed& rhs);

	public:
		bool initializeLsoGreenLed(void);

		bool loadCalibParamFromProfile(void) override;
		bool saveCalibParamToProfile(void) override;

	private:
		struct LsoGreenLedImpl;
		std::unique_ptr<LsoGreenLedImpl> d_ptr;
		LsoGreenLedImpl& impl(void) const;
	};
}



