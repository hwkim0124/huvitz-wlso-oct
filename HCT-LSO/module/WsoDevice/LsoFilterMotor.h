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

		bool updatePositionToOrigin(int mode) override;
		bool setCurrentPositionAsOrigin(int mode) override;

		bool updatePositionToColor(void);
		bool updatePositionToBlueFaf(void);
		bool updatePositionToGreenFaf(void);
		bool updatePositionToICGA(void);

		void setCurrentPositionAsColor(void);
		void setCurrentPositionAsBlueFaf(void);
		void setCurrentPositionAsGreenFaf(void);
		void setCurrentPositionAsICGA(void);

		int getPositionOfColor(void);
		int getPositionOfBlueFaf(void);
		int getPositionOfGreenFaf(void);
		int getPositionOfICGA(void);

		void setPositionOfColor(int pos);
		void setPositionOfBlueFaf(int pos);
		void setPositionOfGreenFaf(int pos);
		void setPositionOfICGA(int pos);

		int getColorModeIndex(int mode);

		virtual bool loadCalibParamFromProfile(void) override;
		virtual bool saveCalibParamToProfile(void) override;

		bool isAtPiPostion(void);

	private:
		struct LsoFilterMotorImpl;
		std::unique_ptr<LsoFilterMotorImpl> d_ptr;
		LsoFilterMotorImpl& impl(void) const;
	};
}

