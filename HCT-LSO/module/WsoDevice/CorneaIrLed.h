#pragma once
#include "WsoDevice2.h"
#include "LightLed.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API CorneaIrLed : public LightLed
	{
	public:
		CorneaIrLed();
		CorneaIrLed(MainBoard* board, LightType LedType);
		virtual ~CorneaIrLed();

		CorneaIrLed(CorneaIrLed&& rhs);
		CorneaIrLed& operator=(CorneaIrLed&& rhs);
		CorneaIrLed(const CorneaIrLed& rhs);
		CorneaIrLed& operator=(const CorneaIrLed& rhs);

	public:
		bool initializeCorneaIrLed(void);

	private:
		struct CorneaIrLedImpl;
		std::unique_ptr<CorneaIrLedImpl> d_ptr;
		CorneaIrLedImpl& impl(void) const;
	};
}



