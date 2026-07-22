using OpenCvSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WsoToolkit.utils
{
    internal static class DrawMatUtil
    {
        public static void DottedLine(Mat img, Point pt1, Point pt2, Scalar color, int thickness = 1, int dashLength = 6, int spaceLength = 4)
        {
            // LineIterator implements IEnumerable<LineIterator.Pixel>
            using (LineIterator it = new LineIterator(img, pt1, pt2, connectivity: PixelConnectivity.Connectivity8))
            {
                // Convert to an array or list to easily use indexed loops
                LineIterator.Pixel[] pixels = it.ToArray();
                int totalPixels = pixels.Length;
                int patternLength = dashLength + spaceLength;

                for (int i = 0; i < totalPixels; i++)
                {
                    // Check if the current pixel index falls within the "dash" interval
                    if (i % patternLength < dashLength)
                    {
                        // Access the pixel position coordinate directly
                        Point currentPoint = pixels[i].Pos;

                        if (thickness <= 1)
                        {
                            // Direct pixel plotting
                            img.Set(currentPoint.Y, currentPoint.X, color);
                        }
                        else
                        {
                            // Handle thicker lines via filled circle brush
                            Cv2.Circle(img, currentPoint, thickness / 2, color, -1, LineTypes.Link8);
                        }
                    }
                }
            }
        }
    }
}
