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


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool MoveLeft()
        {
            return moveStageLeft();
        }

        public static bool MoveRight()
        {
            return moveStageRight();
        }

        public static bool MoveUp()
        {
            return moveStageUp();
        }

        public static bool MoveDown()
        {
            return moveStageDown();
        }

        public static bool MoveForward()
        {
            return moveStageForward();
        }

        public static bool MoveBackward()
        {
            return moveStageBackward();
        }

        public static bool StopX()
        {
            return stopStageX();
        }

        public static bool StopY()
        {
            return stopStageY();
        }

        public static bool StopZ()
        {
            return stopStageZ();
        }

        public static bool StopAll()
        {
            return stopStageAll();
        }
    }
}
