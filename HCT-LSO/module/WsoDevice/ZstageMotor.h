#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API ZstageMotor : public StepMotor
	{
	public:
		ZstageMotor();
		ZstageMotor(MainBoard* board);
		virtual ~ZstageMotor();

		ZstageMotor(ZstageMotor&& rhs);
		ZstageMotor& operator=(ZstageMotor&& rhs);
		ZstageMotor(const ZstageMotor& rhs);
		ZstageMotor& operator=(const ZstageMotor& rhs);

	public:
		bool initializeZstageMotor(void);

	private:
		struct ZstageMotorImpl;
		std::unique_ptr<ZstageMotorImpl> d_ptr;
		ZstageMotorImpl& impl(void) const;
	};
}

