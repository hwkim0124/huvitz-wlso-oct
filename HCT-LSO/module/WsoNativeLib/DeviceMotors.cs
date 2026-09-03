using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Media.Animation;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoDevice;

    public static class DeviceMotors
    {
        const string LibraryName = WsoSystemDllPath;

        // At most one move may be in flight per motor. Moves on different motors
        // may overlap: UsbComm serializes each control transaction internally.
        private static readonly Dictionary<MotorType, Task> _moveTasks = new();
        private static readonly object _moveLock = new();

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool fetchStepMotorStatus(int type, out StepMotorStatus status);

        [DllImport(LibraryName)]
        private static extern void connectStepMotorPositionChanged(int type, StepMotorPositionChanged clb);

        [DllImport(LibraryName)]
        private static extern void releaseStepMotorPositionChanged(int type);


        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorPosition(int type, int pos);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorByPositionOffset(int type, int offset);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorByValue(int type, float value);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorByValueOffset(int type, float offset);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorToOrigin(int type, int mode = 0);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorToUpperEnd(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorToLowerEnd(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStepMotorToCenter(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStepMotorAtUpperEnd(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStepMotorAtLowerEnd(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStepMotorAtCenter(int type);

        [DllImport(LibraryName)]
        private static extern void stopStepMotor(int type);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool setStepMotorCurrentPositionAsOrigin(int type, int mode = 0);

        [DllImport(LibraryName)]
        private static extern int getStepMotorPosition(int type);

        [DllImport(LibraryName)]
        private static extern float getStepMotorValue(int type);

        [DllImport(LibraryName)]
        private static extern int getStepMotorPositionAtValue(int type, float value);

        [DllImport(LibraryName)]
        private static extern float getStepMotorValueAtPosition(int type, int pos);



        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctRefNdMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctAntLensMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isRetMirrorMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isLsoFilterMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isLsoFocusMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctPolarMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctReferMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctFocusMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isSwingMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isXstageMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isZstageMotorAtPiPosition();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isYstageMotorAtUpperLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isYstageMotorAtLowerLimit();


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool FetchStepMotorStatus(MotorType type, out StepMotorStatus status)
        {
            return fetchStepMotorStatus((int)type, out status);
        }

        public static void ConnectStepMotorPositionChanged(MotorType type, StepMotorPositionChanged clb)
        {
            connectStepMotorPositionChanged((int)type, clb);
        }

        // Safe to call with a move still in flight: CallbackRegistry guards the set
        // and the invoke with the same mutex, so once this returns the entry is null
        // and no further callback can fire.
        public static void ReleaseStepMotorPositionChanged(MotorType type)
        {
            releaseStepMotorPositionChanged((int)type);
        }

        public static bool MoveStepMotorPosition(MotorType type, int pos)
        {
            return moveStepMotorPosition((int)type, pos);
        }
        
        private static bool TryStartMove(MotorType type, Action move)
        {
            lock (_moveLock)
            {
                if (_moveTasks.TryGetValue(type, out var running) && !running.IsCompleted)
                {
                    return false;
                }

                // LongRunning: the native call blocks in waitForUpdate() for up to
                // MOTOR_MOVE_WAIT_DELAY * MOTOR_MOVE_WAIT_COUNT ms, and every motor
                // may be moving at once, so these must not run on pooled threads.
                _moveTasks[type] = Task.Factory.StartNew(
                    move, CancellationToken.None,
                    TaskCreationOptions.LongRunning, TaskScheduler.Default);
                return true;
            }
        }

        public static bool IsStepMotorMoving(MotorType type)
        {
            lock (_moveLock)
            {
                return _moveTasks.TryGetValue(type, out var running) && !running.IsCompleted;
            }
        }

        // The Async variants return false without starting anything when a move
        // on the same motor is still in flight.
        public static bool MoveStepMotorPositionAsync(MotorType type, int pos)
        {
            return TryStartMove(type, () => moveStepMotorPosition((int)type, pos));
        }

        public static bool MoveStepMotorByPositionOffset(MotorType type, int offset)
        {
            return moveStepMotorByPositionOffset((int)type, offset);
        }

        public static bool MoveStepMotorByPositionOffsetAsync(MotorType type, int offset)
        {
            return TryStartMove(type, () => moveStepMotorByPositionOffset((int)type, offset));
        }

        public static bool MoveStepMotorByValue(MotorType type, float value)
        {
            return moveStepMotorByValue((int)type, value);
        }

        public static bool MoveStepMotorByValueAsync(MotorType type, float value)
        {
            return TryStartMove(type, () => moveStepMotorByValue((int)type, value));
        }

        public static bool MoveStepMotorByValueOffset(MotorType type, float offset)
        {
            return moveStepMotorByValueOffset((int)type, offset);
        }

        public static bool MoveStepMotorByValueOffsetAsync(MotorType type, float offset)
        {
            return TryStartMove(type, () => moveStepMotorByValueOffset((int)type, offset));
        }

        public static bool MoveStepMotorToOrigin(MotorType type, int mode = 0)
        {
            return moveStepMotorToOrigin((int)type, mode);
        }

        public static bool MoveStepMotorToOriginAsync(MotorType type, int mode = 0)
        {
            return TryStartMove(type, () => moveStepMotorToOrigin((int)type, mode));
        }

        public static bool MoveStepMotorToUpperEnd(MotorType type)
        {
            return moveStepMotorToUpperEnd((int)type);
        }

        public static bool MoveStepMotorToUpperEndAsync(MotorType type)
        {
            return TryStartMove(type, () => moveStepMotorToUpperEnd((int)type));
        }

        public static bool MoveStepMotorToLowerEnd(MotorType type)
        {
            return moveStepMotorToLowerEnd((int)type);
        }

        public static bool MoveStepMotorToLowerEndAsync(MotorType type)
        {
            return TryStartMove(type, () => moveStepMotorToLowerEnd((int)type));
        }

        public static bool MoveStepMotorToCenter(MotorType type)
        {
            return moveStepMotorToCenter((int)type);
        }

        public static bool MoveStepMotorToCenterAsync(MotorType type)
        {
            return TryStartMove(type, () => moveStepMotorToCenter((int)type));
        }

        public static bool IsStepMotorAtUpperEnd(MotorType type)
        {
            return isStepMotorAtUpperEnd((int)type);
        }
        public static bool IsStepMotorAtLowerEnd(MotorType type)
        {
            return isStepMotorAtLowerEnd((int)type);
        }

        public static bool IsStepMotorAtCenter(MotorType type)
        {
            return isStepMotorAtCenter((int)type);
        }

        public static void StopStepMotor(MotorType type)
        {
            stopStepMotor((int)type);
        }

        public static bool SetStepMotorCurrentPositionAsOrigin(MotorType type, int mode = 0)
        {
            return setStepMotorCurrentPositionAsOrigin((int)type, mode);
        }

        public static int GetStepMotorPosition(MotorType type)
        {
            return getStepMotorPosition((int)type);
        }

        public static int GetStepMotorPositionAtValue(MotorType type, float value)
        {
            return getStepMotorPositionAtValue((int)type, value);
        }

        public static float GetStepMotorValue(MotorType type)
        {
            return getStepMotorValue((int)type);
        }

        public static float GetStepMotorValueAtPosition(MotorType type, int pos)
        {
            return getStepMotorValueAtPosition((int)type, pos);
        }

        public static bool IsOctRefNdMotorAtPiPosition()
        {
            return isOctRefNdMotorAtPiPosition();
        }

        public static bool IsOctAntLensMotorAtPiPosition()
        {
            return isOctAntLensMotorAtPiPosition();
        }

        public static bool IsRetMirrorMotorAtPiPosition()
        {
            return isRetMirrorMotorAtPiPosition();
        }

        public static bool IsLsoFilterMotorAtPiPosition()
        {
            return isLsoFilterMotorAtPiPosition();
        }

        public static bool IsLsoFocusMotorAtPiPosition()
        {
            return isLsoFocusMotorAtPiPosition();
        }

        public static bool IsOctPolarMotorAtPiPosition()
        {
            return isOctPolarMotorAtPiPosition();
        }

        public static bool IsOctReferMotorAtPiPosition()
        {
            return isOctReferMotorAtPiPosition();
        }

        public static bool IsOctFocusMotorAtPiPosition()
        {
            return isOctFocusMotorAtPiPosition();
        }

        public static bool IsSwingMotorAtPiPosition()
        {
            return isSwingMotorAtPiPosition();
        }

        public static bool IsXstageMotorAtPiPosition()
        {
            return isXstageMotorAtPiPosition();
        }

        public static bool IsZstageMotorAtPiPosition()
        {
            return isZstageMotorAtPiPosition();
        }

        public static bool IsYstageMotorAtUpperLimit()
        {
            return isYstageMotorAtUpperLimit();
        }

        public static bool IsYstageMotorAtLowerLimit()
        {
            return isYstageMotorAtLowerLimit();
        }
    }
}
