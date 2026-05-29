#pragma once

#include "WsoDevice2.h"
#include "BoardComponent.h"

#include <memory>
#include <string>


namespace wso_device
{
	class MainBoard;

	class WSODEVICE_DLL_API StepMotor : public BoardComponent
	{
	public:
		StepMotor();
		StepMotor(MainBoard* board, StepMotorType type);
		virtual ~StepMotor();

		StepMotor(StepMotor&& rhs);
		StepMotor& operator=(StepMotor&& rhs);
		StepMotor(const StepMotor& rhs);
		StepMotor& operator=(const StepMotor& rhs);

	public:
		virtual bool initializeStepMotor(void);
		bool isInitiated(void) const;

		virtual bool updatePosition(int pos);
		virtual bool updatePositionUseThread(int pos);
		virtual bool updatePositionByOffset(int offset);

		virtual bool updatePositionByValue(float value);
		virtual bool updatePositionByValueOffset(float offset);
		virtual bool updatePositionToOrigin(int mode = 0);
		virtual bool setCurrentPositionAsOrigin(int mode = 0);
		virtual float getCurrentValueByPosition(void) const;

		virtual int getPositionAtValue(float value) const;
		virtual float getValueAtPosition(int pos) const;

		bool controlMove(int pos, bool async = false);
		bool controlHome(void);
		bool controlStop(void);

		virtual bool updateStopVelocity();
		bool controlMoveVelocity(int direction);
		bool controlStopVelocity();
		bool controlSetVelocity(int nAccelStop, int nMinSpeed, int nMaxSpeed);
		bool controlSetDefaultVelocity();

		bool updateStatus(void);
		void reportStatus(void);
		bool fetchStatus(StepMotorStatus* status);

		void setAsyncMode(bool flag);
		bool isAsyncMode(void) const;
		bool waitForUpdate(int posOffset = 0, int timeDelay = MOTOR_MOVE_WAIT_DELAY, int countMax = MOTOR_MOVE_WAIT_COUNT);

		void setPosition(int pos);
		int getPosition(void) const;
		int getTargetPosition(void) const;
		int getCenterPosition(void) const;

		int getRangeMax(void) const;
		int getRangeMin(void) const;
		int getRangeSize(void) const;

		void setRangeMax(int value);
		void setRangeMin(int value);

		const char* getName(void) const;
		StepMotorType getType(void) const;
		void setType(StepMotorType type);

		MotorType getMotorType(void) const;
		bool isXStageMotor(void) const;
		bool isYStageMotor(void) const;
		bool isZStageMotor(void) const;
		int getSliderStepSize(void) const;

		virtual bool isEndOfLowerPosition(void) const;
		virtual bool isEndOfUpperPosition(void) const;
		virtual bool isAtLowerSideOfPosition(void) const;
		virtual bool isAtUpperSideOfPosition(void) const;
		virtual bool isAtCenterOfPosition(void) const;

	protected:
		MainBoard* getMainBoard(void) const;
		std::uint8_t getMotorId(void) const;
		
		static std::int32_t getInitPosition(StepMotorType type);

	private:
		struct StepMotorImpl;
		std::unique_ptr<StepMotorImpl> d_ptr;
		StepMotorImpl& impl(void) const;
	};
}
