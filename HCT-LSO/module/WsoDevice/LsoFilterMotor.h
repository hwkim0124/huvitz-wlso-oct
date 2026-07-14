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
		struct LsoFilterMotorImpl;
		std::unique_ptr<LsoFilterMotorImpl> d_ptr;
		LsoFilterMotorImpl& impl(void) const;
	};
}

