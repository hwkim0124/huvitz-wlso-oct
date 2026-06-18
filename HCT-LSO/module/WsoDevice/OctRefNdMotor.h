#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API OctRefNdMotor : public StepMotor
	{
	public:
		OctRefNdMotor();
		OctRefNdMotor(MainBoard* board);
		virtual ~OctRefNdMotor();

		OctRefNdMotor(OctRefNdMotor&& rhs);
		OctRefNdMotor& operator=(OctRefNdMotor&& rhs);
		OctRefNdMotor(const OctRefNdMotor& rhs);
		OctRefNdMotor& operator=(const OctRefNdMotor& rhs);

	public:
		bool initializeOctRefNdMotor(void);

	private:
		struct OctRefNdMotorImpl;
		std::unique_ptr<OctRefNdMotorImpl> d_ptr;
		OctRefNdMotorImpl& impl(void) const;
	};
}

