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

    public static class AlignStage
    {
        const string LibraryName = WsoSystemDllPath;

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveHeadLeft();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveHeadRight();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveHeadUp();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveHeadDown();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveHeadForward();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool moveHeadBackward();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopHeadX();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopHeadY();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopHeadZ();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool stopHeadXYZ();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isHeadAtLeftEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isHeadAtRightEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isHeadAtLowerEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isHeadAtUpperEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isHeadAtForwardEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isHeadAtBackwardEnd();

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
        private static extern bool stopChinrest();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isChinrestAtUpperEnd();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isChinrestAtLowerEnd();




        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool MoveHeadLeft()
        {
            return moveHeadLeft();
        }

        public static bool MoveHeadRight()
        {
            return moveHeadRight();
        }

        public static bool MoveHeadUp()
        {
            return moveHeadUp();
        }

        public static bool MoveHeadDown()
        {
            return moveHeadDown();
        }

        public static bool MoveHeadForward()
        {
            return moveHeadForward();
        }

        public static bool MoveHeadBackward()
        {
            return moveHeadBackward();
        }

        public static bool StopHeadX()
        {
            return stopHeadX();
        }

        public static bool StopHeadY()
        {
            return stopHeadY();
        }

        public static bool StopHeadZ()
        {
            return stopHeadZ();
        }

        public static bool StopHeadXYZ()
        {
            return stopHeadXYZ();
        }

        public static bool IsHeadAtLeftEnd()
        {
            return isHeadAtLeftEnd();
        }

        public static bool IsHeadAtRightEnd()
        {
            return isHeadAtRightEnd();
        }

        public static bool IsHeadAtLowerEnd()
        {
            return isHeadAtLowerEnd();
        }

        public static bool IsHeadAtUpperEnd()
        {
            return isHeadAtUpperEnd();
        }

        public static bool IsHeadAtForwardEnd()
        {
            return isHeadAtForwardEnd();
        }

        public static bool IsHeadAtBackwardEnd()
        {
            return isHeadAtBackwardEnd();
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

        public static bool StopChinrest()
        {
            return stopChinrest();
        }

        public static bool IsChinrestAtUpperEnd()
        {
            return isChinrestAtUpperEnd();
        }

        public static bool IsChinrestAtLowerEnd()
        {
            return isChinrestAtLowerEnd();
        }

        
    }
}
