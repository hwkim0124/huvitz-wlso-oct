#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API ChinRestMotor : public StepMotor
	{
	public:
		ChinRestMotor();
		ChinRestMotor(MainBoard* board);
		virtual ~ChinRestMotor();

		ChinRestMotor(ChinRestMotor&& rhs);
		ChinRestMotor& operator=(ChinRestMotor&& rhs);
		ChinRestMotor(const ChinRestMotor& rhs);
		ChinRestMotor& operator=(const ChinRestMotor& rhs);

	public:
		bool initializeChinRestMotor(void);

	private:
		struct ChinRestMotorImpl;
		std::unique_ptr<ChinRestMotorImpl> d_ptr;
		ChinRestMotorImpl& impl(void) const;
	};
}

