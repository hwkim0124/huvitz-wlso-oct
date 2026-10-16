using System;
using System.Runtime.InteropServices;

namespace WsoNativeLib
{
    // C# mirror of module/WsoDomain/EyeAlignDefines.h. Keep the enum values and the
    // struct field order identical; EyeAlign.VerifyStructSizes() asserts the sizes.
    public static class WsoEyeAlign
    {
        public enum EyeAlignMode : int
        {
            Manual = 0,
            Auto = 1,
            Full = 2,
        }

        public enum EyeAlignPhase : int
        {
            Idle = 0,
            ToReadyZ,
            ToEyeX,
            SwingToTilt,
            Searching,
            CenteringY,
            Approaching,
            CenteringX,
            CenteringZ,
            Focused,
            Monitoring,
            Held,
            Canceled,
            Failed,
        }

        public enum EyeAlignResult : int
        {
            None = 0,
            UserCancel,
            EyeNotFound,
            PupilLost,
            LowerEyeNotFound,
            AlignTimeout,
            NotInitialized,
            PreviewNotRunning,
            Busy,
            MotorNotReady,
            MotorMoveFailed,
            LimitReached,
            EyeTooClose,
            Timeout,
            InternalError,
        }

        public enum EyeAlignEvent : int
        {
            None = 0,
            FullAlignStarted,
            AutoAlignStarted,
            ManualAlignStarted,
            AlignCancelled,
            AlignFailed,
            ReadyPositionReached,
            EyeSidePositionReached,
            SwingAtTilt,
            SearchStarted,
            EyeFound,
            EyeNotFound,
            YCentered,
            ApproachStarted,
            LowerEyeFound,
            LowerEyeNotFound,
            XCentered,
            ZCentered,
            FocusAligned,
            FocusLost,
            EyeLost,
            AlignHeld,
            AlignResumed,
            EyeTooClose,
            EndOfLeft,
            EndOfRight,
            EndOfTop,
            EndOfBottom,
            EndOfForward,
            EndOfBackward,
            EndOfSwingLeft,
            EndOfSwingRight,
            MotorMoveFailed,
            PhaseTimeout,
            AlignTimeout,
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct PupilDetectParam
        {
            public int targetX;
            public int targetY;
            public int signH;
            public int signV;

            public PupilDetectParam()
            {
                targetX = -1;
                targetY = -1;
                signH = +1;
                signV = +1;
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct EyeAlignConfigParam
        {
            public int readyPosZ;
            public int swingTiltPos_OD;
            public int swingTiltPos_OS;
            public int eyePosX_OD;
            public int eyePosX_OS;
            public int searchLimitPosZ;
            public int minSafePosZ;
            public int posToleranceSteps;
            public int stepZ;

            public float toleranceYPx;
            public float approachTolZPx;
            public float toleranceXPx;
            public float toleranceZPx;
            public float startDeadbandPx;
            public float stepsPerPxX;
            public float stepsPerPxY;
            public float stepsPerPxZ;

            public int detectConsecutive;
            public int lostFramesToStop;
            public int lostFramesToCancel;
            public int settleFrames;
            public float maxJumpPx;
            public float smoothingAlpha;

            public int readyTimeoutMs;
            public int swingTimeoutMs;
            public int eyeXTimeoutMs;
            public int searchTimeoutMs;
            public int alignTimeoutMs;

            public int backOffOnEnd;
            public int irLedIntensity;

            public PupilDetectParam detLeft;
            public PupilDetectParam detRight;
            public PupilDetectParam detLower;

            // Same defaults as the in-class initializers of the C++ struct.
            public EyeAlignConfigParam()
            {
                readyPosZ = -10000;
                swingTiltPos_OD = -1500;
                swingTiltPos_OS = +1500;
                eyePosX_OD = +15000;
                eyePosX_OS = -15000;
                searchLimitPosZ = -4000;
                minSafePosZ = 0;
                posToleranceSteps = 20;
                stepZ = 300;

                toleranceYPx = 6.0f;
                approachTolZPx = 8.0f;
                toleranceXPx = 6.0f;
                toleranceZPx = 8.0f;
                startDeadbandPx = 10.0f;
                stepsPerPxX = 1.0f;
                stepsPerPxY = 1.0f;
                stepsPerPxZ = 1.0f;

                detectConsecutive = 3;
                lostFramesToStop = 5;
                lostFramesToCancel = 300;
                settleFrames = 2;
                maxJumpPx = 60.0f;
                smoothingAlpha = 0.5f;

                readyTimeoutMs = 10000;
                swingTimeoutMs = 10000;
                eyeXTimeoutMs = 10000;
                searchTimeoutMs = 20000;
                alignTimeoutMs = 30000;

                backOffOnEnd = 1;
                irLedIntensity = 3;

                detLeft = new PupilDetectParam { signV = -1 };     // side cameras, see the C++ header
                detRight = new PupilDetectParam { signV = -1 };
                detLower = new PupilDetectParam();
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct PupilDetectResult
        {
            public int camId;
            public int found;
            public float cx;
            public float cy;
            public float radius;
            public float score;
            public int frameSeq;

            public bool IsFound => found != 0;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct EyeAlignStatus
        {
            public int mode;
            public int phase;
            public int result;
            public int eyeSide;
            public int sideCamId;
            public int posX;
            public int posY;
            public int posZ;
            public int posSwing;
            public PupilDetectResult side;
            public PupilDetectResult lower;
            public float errX;
            public float errY;
            public float errZ;
            public int lostFrames;
            public int elapsedMs;
        }
    }
}
