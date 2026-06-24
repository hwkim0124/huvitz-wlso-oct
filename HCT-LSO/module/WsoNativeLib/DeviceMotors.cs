using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Animation;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoNativeLib.WsoDevice;

    public static class DeviceMotors
    {
        const string LibraryName = WsoSystemDllPath;
        private static Task? _taskToMove = null;

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
        private static extern int setStepMotorCurrentPositionAsOrigin(int type, int mode = 0);

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
        private static extern bool moveChinrestUp();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveChinrestDown();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopChinrestMove();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isChinrestAtUpperEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isChinrestAtLowerEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isSwingMotorAtHighLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isSwingMotorAtLowLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctFocusMotorAtOrigin();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctPolarMotorAtOrigin();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctReferMotorAtOrigin();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctRefNdMotorAtOrigin();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isRetMirrorMotorAtOrigin();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isLsoFocusMotorAtOrigin();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isXstageMotorAtHighLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isXstageMotorAtLowLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isYstageMotorAtHighLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isYstageMotorAtLowLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isZstageMotorAtHighLimit();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isZstageMotorAtLowLimit();

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

        public static void ReleaseStepMotorPositionChanged(MotorType type)
        {
            releaseStepMotorPositionChanged((int)type);
        }

        public static bool MoveStepMotorPosition(MotorType type, int pos)
        {
            return moveStepMotorPosition((int)type, pos);
        }

        private static bool IsTaskToMoveCompleted()
        {
            return _taskToMove == null || _taskToMove.IsCompleted;
        }

        public static void MoveStepMotorPositionAsync(MotorType type, int pos)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorPosition((int)type, pos);
                });
            }
        }

        public static bool MoveStepMotorByPositionOffset(MotorType type, int offset)
        {
            return moveStepMotorByPositionOffset((int)type, offset);
        }

        public static void MoveStepMotorByPositionOffsetAsync(MotorType type, int offset)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorByPositionOffset((int)type, offset);
                });
            }
        }

        public static bool MoveStepMotorByValue(MotorType type, float value)
        {
            return moveStepMotorByValue((int)type, value);
        }

        public static void MoveStepMotorByValueAsync(MotorType type, float value)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorByValue((int)type, value);
                });
            }
        }

        public static bool MoveStepMotorByValueOffset(MotorType type, float offset)
        {
            return moveStepMotorByValueOffset((int)type, offset);
        }

        public static void MoveStepMotorByValueOffsetAsync(MotorType type, float offset)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorByValueOffset((int)type, offset);
                });
            }
        }

        public static bool MoveStepMotorToOrigin(MotorType type, int mode = 0)
        {
            return moveStepMotorToOrigin((int)type, mode);
        }

        public static void MoveStepMotorToOriginAsync(MotorType type, int mode = 0)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorToOrigin((int)type, mode);
                });
            }
        }

        public static bool MoveStepMotorToUpperEnd(MotorType type)
        {
            return moveStepMotorToUpperEnd((int)type);
        }

        public static void MoveStepMotorToUpperEndAsync(MotorType type)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorToUpperEnd((int)type);
                });
            }
        }

        public static bool MoveStepMotorToLowerEnd(MotorType type)
        {
            return moveStepMotorToLowerEnd((int)type);
        }

        public static void MoveStepMotorToLowerEndAsync(MotorType type)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorToLowerEnd((int)type);
                });
            }
        }

        public static bool MoveStepMotorToCenter(MotorType type)
        {
            return moveStepMotorToCenter((int)type);
        }

        public static void MoveStepMotorToCenterAsync(MotorType type)
        {
            if (IsTaskToMoveCompleted())
            {
                _taskToMove = Task.Factory.StartNew(() =>
                {
                    moveStepMotorToCenter((int)type);
                });
            }
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

        public static int SetStepMotorCurrentPositionAsOrigin(MotorType type, int mode = 0)
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

        public static bool MoveChinrestUp()
        {
            return moveChinrestUp();
        }

        public static bool MoveChinrestDown()
        {
            return moveChinrestDown();
        }

        public static bool StopChinrestMove()
        {
            return stopChinrestMove();
        }

        public static bool IsChinrestAtUpperEnd()
        {
            return isChinrestAtUpperEnd();
        }

        public static bool IsChinrestAtLowerEnd()
        {
            return isChinrestAtLowerEnd();
        }

        public static bool IsSwingMotorAtHighLimit()
        {
            return isSwingMotorAtHighLimit();
        }

        public static bool IsSwingMotorAtLowLimit()
        {
            return isSwingMotorAtLowLimit();
        }

        public static bool IsOctFocusMotorAtOrigin()
        {
            return isOctFocusMotorAtOrigin();
        }

        public static bool IsOctPolarMotorAtOrigin()
        {
            return isOctPolarMotorAtOrigin();
        }

        public static bool IsOctReferMotorAtOrigin()
        {
            return isOctReferMotorAtOrigin();
        }

        public static bool IsRetMirrorMotorAtOrigin()
        {
            return isRetMirrorMotorAtOrigin();
        }

        public static bool IsLsoFocusMotorAtOrigin()
        {
            return isLsoFocusMotorAtOrigin();
        }

        public static bool IsXstageMotorAtHighLimit()
        {
            return isXstageMotorAtHighLimit();
        }

        public static bool IsXstageMotorAtLowLimit()
        {
            return isXstageMotorAtLowLimit();
        }

        public static bool IsYstageMotorAtHighLimit()
        {
            return isYstageMotorAtHighLimit();
        }

        public static bool IsYstageMotorAtLowLimit()
        {
            return isYstageMotorAtLowLimit();
        }

        public static bool IsZstageMotorAtHighLimit()
        {
            return isZstageMotorAtHighLimit();
        }

        public static bool IsZstageMotorAtLowLimit()
        {
            return isZstageMotorAtLowLimit();
        }

    }
}
