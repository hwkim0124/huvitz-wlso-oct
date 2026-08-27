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

    public static class StageMotors
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStageLeft();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStageRight();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStageUp();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStageDown();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStageForward();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveStageBackward();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopStageX();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopStageY();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopStageZ();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopStageAll();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStageAtLeftEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStageAtRightEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStageAtLowerEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStageAtUpperEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStageAtForwardEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isStageAtBackwardEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveSwingLeft();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveSwingRight();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopSwingMove();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isSwingAtLeftEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isSwingAtRightEnd();

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
        public static bool MoveStageLeft()
        {
            return moveStageLeft();
        }

        public static bool MoveStageRight()
        {
            return moveStageRight();
        }

        public static bool MoveStageUp()
        {
            return moveStageUp();
        }

        public static bool MoveStageDown()
        {
            return moveStageDown();
        }

        public static bool MoveStageForward()
        {
            return moveStageForward();
        }

        public static bool MoveStageBackward()
        {
            return moveStageBackward();
        }

        public static bool StopStageX()
        {
            return stopStageX();
        }

        public static bool StopStageY()
        {
            return stopStageY();
        }

        public static bool StopStageZ()
        {
            return stopStageZ();
        }

        public static bool StopStageAll()
        {
            return stopStageAll();
        }

        public static bool IsStageAtLeftEnd()
        {
            return isStageAtLeftEnd();
        }

        public static bool IsStageAtRightEnd()
        {
            return isStageAtRightEnd();
        }

        public static bool IsStageAtLowerEnd()
        {
            return isStageAtLowerEnd();
        }

        public static bool IsStageAtUpperEnd()
        {
            return isStageAtUpperEnd();
        }

        public static bool IsStageAtForwardEnd()
        {
            return isStageAtForwardEnd();
        }

        public static bool IsStageAtBackwardEnd()
        {
            return isStageAtBackwardEnd();
        }

        public static bool MoveSwingLeft()
        {
            return moveSwingLeft();
        }

        public static bool MoveSwingRight()
        {
            return moveSwingRight();
        }

        public static bool StopSwingMove()
        {
            return stopSwingMove();
        }

        public static bool IsSwingAtLeftEnd()
        {
            return isSwingAtLeftEnd();
        }

        public static bool IsSwingAtRightEnd()
        {
            return isSwingAtRightEnd();
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
