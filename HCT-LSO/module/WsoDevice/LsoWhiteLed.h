#pragma once
#include "WsoDevice2.h"
#include "LightLed.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API LsoWhiteLed : public LightLed
	{
	public:
		LsoWhiteLed();
		LsoWhiteLed(MainBoard* board);
		virtual ~LsoWhiteLed();

		LsoWhiteLed(LsoWhiteLed&& rhs);
		LsoWhiteLed& operator=(LsoWhiteLed&& rhs);
		LsoWhiteLed(const LsoWhiteLed& rhs);
		LsoWhiteLed& operator=(const LsoWhiteLed& rhs);

	public:
		bool initializeLsoWhiteLed(void);

	private:
		struct LsoWhiteLedImpl;
		std::unique_ptr<LsoWhiteLedImpl> d_ptr;
		LsoWhiteLedImpl& impl(void) const;
	};
}



