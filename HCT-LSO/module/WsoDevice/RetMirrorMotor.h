#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API RetMirrorMotor : public StepMotor
	{
	public:
		RetMirrorMotor();
		RetMirrorMotor(MainBoard* board);
		virtual ~RetMirrorMotor();

		RetMirrorMotor(RetMirrorMotor&& rhs);
		RetMirrorMotor& operator=(RetMirrorMotor&& rhs);
		RetMirrorMotor(const RetMirrorMotor& rhs);
		RetMirrorMotor& operator=(const RetMirrorMotor& rhs);

	public:
		bool initializeRetMirrorMotor(void);

	private:
		struct RetMirrorMotorImpl;
		std::unique_ptr<RetMirrorMotorImpl> d_ptr;
		RetMirrorMotorImpl& impl(void) const;
	};
}

