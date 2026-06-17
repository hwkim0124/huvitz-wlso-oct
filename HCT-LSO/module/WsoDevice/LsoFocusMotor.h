#pragma once

#include "WsoDevice2.h"
#include "OctFocusMotor.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API LsoFocusMotor : public OctFocusMotor
	{
	public:
		LsoFocusMotor();
		LsoFocusMotor(MainBoard* board);
		virtual ~LsoFocusMotor();

		LsoFocusMotor(LsoFocusMotor&& rhs);
		LsoFocusMotor& operator=(LsoFocusMotor&& rhs);
		LsoFocusMotor(const LsoFocusMotor& rhs);
		LsoFocusMotor& operator=(const LsoFocusMotor& rhs);

	public:
		virtual bool initializeLsoFocusMotor(bool ready = true);

		virtual bool updatePosition(int pos) override;

		virtual int getZeroDiopterPosition(void) const override;
		virtual void setZeroDiopterPosition(int pos) override;

		virtual bool loadCalibParamFromProfile(void) override;
		virtual bool saveCalibParamToProfile(void) override;

		virtual bool loadConfigFromIniFile(void) override;
		virtual bool saveConfigToIniFile(void) override;

	private:
		struct LsoFocusMotorImpl;
		std::unique_ptr<LsoFocusMotorImpl> d_ptr;
		LsoFocusMotorImpl& impl(void) const;
	};
}

