#pragma once

#include "EyeAlign2.h"


namespace wso_device
{
	class StepMotor;
}


namespace eye_align
{
	// The only code in the module that touches a motor. Wraps one stage motor and
	// drives it exclusively through StepMotor::moveInVelocityMode() /
	// stopInVelocityMode(); position and limit feedback via updateStatus().
	class EYEALIGN_DLL_API AxisController
	{
	public:
		AxisController();
		AxisController(wso_device::StepMotor* motor, MotorType type, const char* name);
		virtual ~AxisController();

	public:
		bool isValid(void) const;					// has a motor
		bool isReady(void) const;					// motor initiated
		MotorType getType(void) const;
		const char* getName(void) const;

		// Starts a velocity-mode jog in dir (0/1). No-op when already jogging that way;
		// a reversal stops first. Returns false when the board rejected the command.
		bool jog(int dir);
		// Stops a running jog. Idempotent, always safe to call.
		bool stop(void);
		// Refreshes position and end-switch flags from the board (one USB transaction).
		bool poll(void);

		// Jogs toward target and returns true once |target - position| <= tolerance
		// (the jog is stopped when it returns true). Relies on poll() being called
		// between iterations and on POSITIVE_STEP_DIR matching the encoder convention.
		bool jogToward(int target, int tolerance);

		bool isJogging(void) const;
		int getDirection(void) const;				// -1 when not jogging
		int getPosition(void) const;				// steps, as of the last poll()
		bool isAtMin(void) const;					// end switch on the dir-0 side
		bool isAtMax(void) const;					// end switch on the dir-1 side
		bool isAtLimitInDirection(void) const;		// jogging into an active end switch
		bool hasCommandFailed(void) const;			// last jog/stop was rejected

	private:
		wso_device::StepMotor* motor_;
		MotorType type_;
		std::string name_;
		bool jogging_;
		int direction_;
		int position_;
		bool atMin_;
		bool atMax_;
		bool commandFailed_;
	};
}
