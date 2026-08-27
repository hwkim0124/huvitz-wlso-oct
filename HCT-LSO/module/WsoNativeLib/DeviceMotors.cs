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

    }
}
