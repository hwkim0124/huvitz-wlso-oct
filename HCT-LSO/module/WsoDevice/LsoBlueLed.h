#pragma once
#include "WsoDevice2.h"
#include "LightLed.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API LsoBlueLed : public LightLed
	{
	public:
		LsoBlueLed();
		LsoBlueLed(MainBoard* board);
		virtual ~LsoBlueLed();

		LsoBlueLed(LsoBlueLed&& rhs);
		LsoBlueLed& operator=(LsoBlueLed&& rhs);
		LsoBlueLed(const LsoBlueLed& rhs);
		LsoBlueLed& operator=(const LsoBlueLed& rhs);

	public:
		bool initializeLsoBlueLed(void);

		bool loadCalibParamFromProfile(void) override;
		bool saveCalibParamToProfile(void) override;

	private:
		struct LsoBlueLedImpl;
		std::unique_ptr<LsoBlueLedImpl> d_ptr;
		LsoBlueLedImpl& impl(void) const;
	};
}



