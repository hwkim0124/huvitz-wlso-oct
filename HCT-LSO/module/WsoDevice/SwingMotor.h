#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API SwingMotor : public StepMotor
	{
	public:
		SwingMotor();
		SwingMotor(MainBoard* board);
		virtual ~SwingMotor();

		SwingMotor(SwingMotor&& rhs);
		SwingMotor& operator=(SwingMotor&& rhs);
		SwingMotor(const SwingMotor& rhs);
		SwingMotor& operator=(const SwingMotor& rhs);

	public:
		bool initializeSwingMotor(void);

	private:
		struct SwingMotorImpl;
		std::unique_ptr<SwingMotorImpl> d_ptr;
		SwingMotorImpl& impl(void) const;
	};
}

