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

		virtual bool updatePositionToOrigin(int mode) override;
		virtual bool setCurrentPositionAsOrigin(int mode) override;

		bool updatePositionToMirrorIn(void);
		bool updatePositionToMirrorOut(void);
		void setCurrentPositionAsMirrorIn(void);
		void setCurrentPositionAsMirrorOut(void);

		virtual int getPositionOfMirrorIn(void) const;
		virtual int getPositionOfMirrorOut(void) const;
		virtual void setPositionOfMirrorIn(int pos);
		virtual void setPositionOfMirrorOut(int pos);

		virtual bool loadCalibParamFromProfile(void) override;
		virtual bool saveCalibParamToProfile(void) override;

	private:
		struct RetMirrorMotorImpl;
		std::unique_ptr<RetMirrorMotorImpl> d_ptr;
		RetMirrorMotorImpl& impl(void) const;
	};
}

