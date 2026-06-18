#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API YstageMotor : public StepMotor
	{
	public:
		YstageMotor();
		YstageMotor(MainBoard* board);
		virtual ~YstageMotor();

		YstageMotor(YstageMotor&& rhs);
		YstageMotor& operator=(YstageMotor&& rhs);
		YstageMotor(const YstageMotor& rhs);
		YstageMotor& operator=(const YstageMotor& rhs);

	public:
		bool initializeYstageMotor(void);

	private:
		struct YstageMotorImpl;
		std::unique_ptr<YstageMotorImpl> d_ptr;
		YstageMotorImpl& impl(void) const;
	};
}

