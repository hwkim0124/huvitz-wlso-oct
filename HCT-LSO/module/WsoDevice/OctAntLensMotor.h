#pragma once

#include "WsoDevice2.h"
#include "StepMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API OctAntLensMotor : public StepMotor
	{
	public:
		OctAntLensMotor();
		OctAntLensMotor(MainBoard* board);
		virtual ~OctAntLensMotor();

		OctAntLensMotor(OctAntLensMotor&& rhs);
		OctAntLensMotor& operator=(OctAntLensMotor&& rhs);
		OctAntLensMotor(const OctAntLensMotor& rhs);
		OctAntLensMotor& operator=(const OctAntLensMotor& rhs);

	public:
		bool initializeOctAntLensMotor(void);

	private:
		struct OctAntLensMotorImpl;
		std::unique_ptr<OctAntLensMotorImpl> d_ptr;
		OctAntLensMotorImpl& impl(void) const;
	};
}

