using System;
using static WsoNativeLib.WsoDevice;

namespace WsoNativeLib.Presets
{
    public enum LsoJogSize
    {
        Fine,
        Med,
        Coarse,
    }

    // Relative step rules for the stage jog pad and wheel. No native calls: the caller supplies
    // the axis range it read from the board and acts on the answers.
    public static class LsoStageJogRules
    {
        // Compile-time travel used when the board range is unavailable (X/Z -30..+30, Y 0..48000).
        public const int FallbackTravelX = 60;
        public const int FallbackTravelY = 48000;
        public const int FallbackTravelZ = 60;

        public static readonly MotorType[] StopAllMotors = { MotorType.StageX, MotorType.StageY, MotorType.StageZ, MotorType.Swing };

        public static int Travel(MotorType axis, int rangeMin, int rangeMax)
        {
            long travel = (long)rangeMax - rangeMin;
            if (travel > 0 && travel <= int.MaxValue)
            {
                return (int)travel;
            }

            return axis switch
            {
                MotorType.StageY => FallbackTravelY,
                MotorType.StageZ => FallbackTravelZ,
                _ => FallbackTravelX,
            };
        }

        public static int StepFor(LsoJogSize size, int travel)
        {
            (double fraction, int cap) = size switch
            {
                LsoJogSize.Fine => (0.02, 10),
                LsoJogSize.Med => (0.08, 50),
                _ => (0.20, 200),
            };
            double step = Math.Min(cap, Math.Round(fraction * travel, MidpointRounding.AwayFromZero));
            return (int)Math.Clamp(step, 1, cap);
        }

        // At most one signedStep: inside [rangeMin, rangeMax] the old boundary clamp applies, outside it only a step towards the range counts (never past the far edge) and a step away is 0.
        public static int ClampStep(int currentPos, int signedStep, int rangeMin, int rangeMax)
        {
            if (currentPos >= rangeMin && currentPos <= rangeMax)
            {
                long target = Math.Clamp((long)currentPos + signedStep, rangeMin, rangeMax);
                return (int)(target - currentPos);
            }

            if (signedStep == 0)
            {
                return 0;
            }

            bool towardRange = currentPos < rangeMin ? signedStep > 0 : signedStep < 0;
            if (!towardRange)
            {
                return 0;
            }

            long proposed = (long)currentPos + signedStep;
            if (currentPos < rangeMin && proposed > rangeMax)
            {
                return (int)((long)rangeMax - currentPos);
            }

            if (currentPos > rangeMax && proposed < rangeMin)
            {
                return (int)((long)rangeMin - currentPos);
            }

            return signedStep;
        }

        public static string? Refusal(bool capturing, bool axisBusy, bool holdActive, int clampedStep)
        {
            if (capturing)
            {
                return "capture running";
            }

            if (axisBusy)
            {
                return "busy";
            }

            if (holdActive)
            {
                return "busy (hold)";
            }

            if (clampedStep == 0)
            {
                return "limit";
            }

            return null;
        }
    }
}
