#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API LsoFilterMotor : public StepMotor
	{
	public:
		LsoFilterMotor();
		LsoFilterMotor(MainBoard* board);
		virtual ~LsoFilterMotor();

		LsoFilterMotor(LsoFilterMotor&& rhs);
		LsoFilterMotor& operator=(LsoFilterMotor&& rhs);
		LsoFilterMotor(const LsoFilterMotor& rhs);
		LsoFilterMotor& operator=(const LsoFilterMotor& rhs);

	public:
		bool initializeLsoFilterMotor(void);

	private:
		struct LsoFilterMotorImpl;
		std::unique_ptr<LsoFilterMotorImpl> d_ptr;
		LsoFilterMotorImpl& impl(void) const;
	};
}

