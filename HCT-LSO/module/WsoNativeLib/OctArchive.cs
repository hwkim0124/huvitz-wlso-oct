using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace WsoNativeLib
{
    using static WsoNativeLib.LibraryConfig;
    using static WsoOctScan;
    public static class OctArchive
    {
        const string LibraryName = WsoSystemDllPath;


        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isOctProtocolResultValid();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool clearOctProtocolResult();

        [DllImport(LibraryName)]
        private static extern int getOctResultPatternSectionCount();

        [DllImport(LibraryName)]
        private static extern int getOctResultPreviewSectionCount();

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctResultPatternImage(int section_idx, int image_idx, ref OctScanImageDescript image);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctResultPreviewImage(int section_idx, int image_idx, ref OctScanImageDescript image);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctResultEnfaceImage(ref OctEnfaceImageDescript image);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool getOctResultCorneaImage(ref OctCorneaImageDescript image);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        public static bool IsProtocolResultValid()
        {
            return isOctProtocolResultValid();
        }

        public static bool ClearProtocolResult()
        {
            return clearOctProtocolResult();
        }
        public static int GetPatternSectionCount()
        {
            return getOctResultPatternSectionCount();
        }
        public static int GetPreviewSectionCount()
        {
            return getOctResultPreviewSectionCount();
        }

        public static bool GetPatternImage(int section_idx, ref OctScanImageDescript image)
        {
            int image_idx = 0;
            return getOctResultPatternImage(section_idx, image_idx, ref image);
        }

        public static bool GetPreviewImage(int section_idx, ref OctScanImageDescript image)
        {
            int image_idx = 0;
            return getOctResultPreviewImage(section_idx, image_idx, ref image);
        }

        public static bool GetEnfaceImage(ref OctEnfaceImageDescript image)
        {
            return getOctResultEnfaceImage(ref image);
        }

        public static bool GetCorneaImage(ref OctCorneaImageDescript image)
        {
            return getOctResultCorneaImage(ref image);
        }
    }
}
