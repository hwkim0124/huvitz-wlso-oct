using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace WsoNativeLib
{
    using static LibraryConfig;
    using static WsoLsoDefs;

    // Stateless display adjust running in native. There are no callbacks here, so this API
    // carries none of the delegate-lifetime hazards the capture APIs do.
    public static class LsoImageAdjust
    {
        const string LibraryName = WsoSystemDllPath;

        // Pinned on both sides: the C++ header carries the matching static_assert, so a
        // field added to one mirror and not the other fails one of the two checks.
        internal const int ExpectedParamSize = 24;

        static LsoImageAdjust()
        {
            Debug.Assert(Marshal.SizeOf<LsoImageAdjustParam>() == ExpectedParamSize,
                $"LsoImageAdjustParam must be {ExpectedParamSize} bytes to match native LsoImageAdjust.");
        }

        // P/Invoke declarations
        /////////////////////////////////////////////////////////////////////////////////////////////
        [DllImport(LibraryName)]
        private static extern int adjustLsoImage(IntPtr src, IntPtr dst, int width, int height,
            int srcStride, int dstStride, int channels, int depthBytes, int channelOrder,
            ref LsoImageAdjustParam param);

        [DllImport(LibraryName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool isLsoImageAdjustNeutral(ref LsoImageAdjustParam param);

        [DllImport(LibraryName)]
        private static extern int getLastImageAdjustError(StringBuilder buf, int len);


        // Public methods
        /////////////////////////////////////////////////////////////////////////////////////////////
        // Applies tone (brightness/contrast/gamma fused into a single lookup) -> saturation
        // -> sharpness -> channel view. src and dst must have identical geometry; passing the
        // same pointer for both adjusts in place. Callers hand over mat.Data and
        // (int)mat.Step().
        // channels: 1 or 3. depthBytes: 1 (8-bit) or 2 (16-bit).
        // 'order' must describe what the buffer actually holds - it decides which plane a
        // channel view extracts and which luma weights saturation uses.
        public static ImageAdjustResult Adjust(IntPtr src, IntPtr dst, int width, int height,
            int srcStride, int dstStride, int channels, int depthBytes,
            LsoChannelOrder order, ref LsoImageAdjustParam param)
        {
            return (ImageAdjustResult)adjustLsoImage(src, dst, width, height, srcStride, dstStride,
                channels, depthBytes, (int)order, ref param);
        }

        // True when every field is neutral, so the caller can skip Adjust altogether.
        public static bool IsNeutral(ref LsoImageAdjustParam param)
        {
            return isLsoImageAdjustNeutral(ref param);
        }

        // Message from the calling thread's own last failed Adjust.
        public static string GetLastError()
        {
            var buf = new StringBuilder(256);
            getLastImageAdjustError(buf, buf.Capacity);
            return buf.ToString();
        }
    }
}
