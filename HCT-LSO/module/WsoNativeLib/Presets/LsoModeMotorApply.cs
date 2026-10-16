using System;
using static WsoNativeLib.WsoDevice;

namespace WsoNativeLib.Presets
{
    // Puts the LSO motors where a mode preset says the mode images, immediately BEFORE the
    // acquisition one-call is started.
    //
    // This is deliberately a managed pre-step and not a field of LsoFundusCaptureRequest /
    // LsoFundusPreviewRequest: those structs are size-checked by the native callee, so
    // widening them is an ABI change, and selecting an optical path is independent of the
    // camera-arm / board-start ordering the one-call owns.
    //
    // BLOCKING. The native move waits in waitForUpdate() for up to
    // MOTOR_MOVE_WAIT_DELAY * MOTOR_MOVE_WAIT_COUNT ms, so every caller runs this off the UI
    // thread. The filter wheel is always driven, never skipped on a cached position: another
    // process (the Toolkit) can move the same wheel, and imaging through the wrong filter is
    // a worse outcome than the move's latency.
    public static class LsoModeMotorApply
    {
        // False means a stated motor did not reach its position, and `failure` names which.
        // A motor the preset does not state (-1) is left exactly as the operator left it.
        public static bool Apply(LsoModeMotor motor, out string failure)
        {
            if (motor == null)
            {
                throw new ArgumentNullException(nameof(motor));
            }

            failure = string.Empty;

            if (motor.LsoFilterMode >= 0
                && !DeviceMotors.MoveStepMotorToOrigin(MotorType.LsoFilter, motor.LsoFilterMode))
            {
                failure = $"LSO filter wheel did not reach mode {motor.LsoFilterMode} "
                    + $"({FilterModeName(motor.LsoFilterMode)}).";
                return false;
            }

            if (motor.LsoFocusPosition >= 0
                && !DeviceMotors.MoveStepMotorPosition(MotorType.LsoFocus, motor.LsoFocusPosition))
            {
                failure = $"LSO focus motor did not reach position {motor.LsoFocusPosition}.";
                return false;
            }

            return true;
        }

        // The filter wheel positions the board holds calibrated origins for. Named so a
        // failure message says which optical path was asked for.
        public static string FilterModeName(int filterMode)
        {
            return filterMode switch
            {
                0 => "Color",
                1 => "BlueFaf",
                2 => "GreenFaf",
                3 => "Icga",
                _ => "not stated",
            };
        }
    }
}
