#pragma once

#include "EyeAlign2.h"


namespace wso_device
{
	class StepMotor;
}


namespace eye_align
{
	// The only code in the module that touches a motor. Wraps one stage motor and
	// drives it through StepMotor::moveInVelocityMode() / stopInVelocityMode() (jog)
	// or StepMotor::updatePosition() / updatePositionByOffset() (blocking position
	// moves); position and limit feedback via updateStatus().
	class EYEALIGN_DLL_API AxisController
	{
	public:
		AxisController();
		AxisController(wso_device::StepMotor* motor, MotorType type, const char* name);
		virtual ~AxisController();

	public:
		bool isValid(void) const;					// has a motor
		bool isReady(void) const;					// motor initiated
		MotorType getMotorType(void) const;
		const char* getName(void) const;

		// Starts a velocity-mode jog in direction (0/1). No-op when already jogging that way;
		// a reversal stops first. Returns false when the board rejected the command.
		bool jog(int direction);
		// Stops a running jog. Idempotent, always safe to call.
		bool stop(void);
		// Refreshes position and end-switch flags from the board (one USB transaction).
		bool poll(void);

		// Jogs toward target and returns true once |target - position| <= tolerance
		// (the jog is stopped when it returns true). Relies on poll() being called
		// between iterations and on POSITIVE_STEP_DIRECTION matching the encoder convention.
		bool jogToward(int target, int tolerance);

		// Position move: stops any jog, then calls StepMotor::updatePosition(target),
		// which BLOCKS the calling thread until the motor reports the target, a stop
		// arrives (abortMove() from another thread) or the motor's own wait times out.
		// Returns the motor's verdict; position is refreshed on return. A rejected
		// command is reported through hasCommandFailed() as well.
		bool moveTo(int target);
		// Relative position move: stops any jog, then calls
		// StepMotor::updatePositionByOffset(offset), which BLOCKS like moveTo().
		// Returns the motor's verdict; position is refreshed on return.
		bool moveBy(int offset);
		// Sends a stop to a position move in flight so a blocked moveTo() / moveBy()
		// returns. Safe to call from any thread.
		bool abortMove(void);

		bool isJogging(void) const;
		int getDirection(void) const;				// -1 when not jogging
		int getPosition(void) const;				// steps, as of the last poll()
		bool isAtMinimumEnd(void) const;			// end switch on the direction-0 side
		bool isAtMaximumEnd(void) const;			// end switch on the direction-1 side
		bool isJoggingIntoEndSwitch(void) const;	// jogging into an active end switch
		bool hasCommandFailed(void) const;			// last jog/stop/move command was rejected

	private:
		wso_device::StepMotor* motor_;
		MotorType type_;
		std::string name_;
		bool jogging_;
		int direction_;
		int position_;
		bool atMinimumEnd_;
		bool atMaximumEnd_;
		bool commandFailed_;
	};
}
