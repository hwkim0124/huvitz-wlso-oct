using System;

namespace WsoNativeLib.Presets
{
    [Flags]
    public enum LsoHeadMove
    {
        None = 0,
        Left = 1,
        Right = 2,
        Up = 4,
        Down = 8,
        Forward = 16,
        Backward = 32,
    }

    // Which head moves a press on a cornea camera preview asks for. nx/ny are the press position
    // normalised to the viewport (0..1). Each axis inside the centre band contributes nothing.
    // Side cameras: X -> Left/Right, Y -> Up/Down. The lower camera looks at Y only: Forward/Backward.
    public static class LsoCorneaClickJog
    {
        public static LsoHeadMove Classify(bool sideCamera, double nx, double ny, double deadBand = 0.08)
        {
            double low = 0.5 - deadBand;
            double high = 0.5 + deadBand;
            LsoHeadMove move = LsoHeadMove.None;

            if (sideCamera)
            {
                if (nx < low)
                {
                    move |= LsoHeadMove.Left;
                }
                else if (nx > high)
                {
                    move |= LsoHeadMove.Right;
                }

                if (ny < low)
                {
                    move |= LsoHeadMove.Up;
                }
                else if (ny > high)
                {
                    move |= LsoHeadMove.Down;
                }
            }
            else
            {
                if (ny < low)
                {
                    move |= LsoHeadMove.Forward;
                }
                else if (ny > high)
                {
                    move |= LsoHeadMove.Backward;
                }
            }

            return move;
        }
    }
}
