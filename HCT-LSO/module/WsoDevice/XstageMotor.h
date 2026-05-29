#pragma once

#include "WsoDevice2.h"
#include "StageMotor.h"

#include <memory>
#include <string>

namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API XstageMotor : public StageMotor
	{
	public:
		XstageMotor();
		XstageMotor(MainBoard* board);
		virtual ~XstageMotor();

		XstageMotor(XstageMotor&& rhs);
		XstageMotor& operator=(XstageMotor&& rhs);
		XstageMotor(const XstageMotor& rhs);
		XstageMotor& operator=(const XstageMotor& rhs);

	public:

	private:
		struct XstageMotorImpl;
		std::unique_ptr<XstageMotorImpl> d_ptr;
		XstageMotorImpl& impl(void) const;
	};
}

