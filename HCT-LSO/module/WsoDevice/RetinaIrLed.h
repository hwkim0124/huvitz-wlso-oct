#pragma once
#include "WsoDevice2.h"
#include "LightLed.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API RetinaIrLed : public LightLed
	{
	public:
		RetinaIrLed();
		RetinaIrLed(MainBoard* board);
		virtual ~RetinaIrLed();

		RetinaIrLed(RetinaIrLed&& rhs);
		RetinaIrLed& operator=(RetinaIrLed&& rhs);
		RetinaIrLed(const RetinaIrLed& rhs);
		RetinaIrLed& operator=(const RetinaIrLed& rhs);

	public:
		bool initializeRetinaIrLed(void);

	private:
		struct RetinaIrLedImpl;
		std::unique_ptr<RetinaIrLedImpl> d_ptr;
		RetinaIrLedImpl& impl(void) const;
	};
}



