using OpenCvSharp;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WsoToolkit.controls
{
    public partial class LsoScanImagePreview
    {
        public enum PreviewDisplayMode
        {
            NONE = 0,
            LIVE = 1,
            REVIEW = 2,
            REVIEW_SLICE = 3,
            REVIEW_ROI = 4,
            LIVE_SEQ_ROI = 5,
            LIVE_OFFSET_ROI = 6,
            REVIEW_OFFSET_ROI = 7,
            LIVE_ROLLING_SW_TRIGGER_OVERLAP = 8,
            REVIEW_ROLLING_SW_TRIGGER_OVERLAP = 9,
            REVIEW_ROLLING_SW_TRIGGER_MANUAL = 10
        }

        private struct MaskCache
        {
            public Mat Mask { get; set; }
            public Mat InvertedMask { get; set; }
            public int Radius { get; set; }
            public OpenCvSharp.Size ImageSize { get; set; }

            public bool IsValid => Mask != null && InvertedMask != null;

            public void Dispose()
            {
                Mask?.Dispose();
                InvertedMask?.Dispose();
                Mask = null;
                InvertedMask = null;
            }

            public void Invalidate()
            {
                Dispose();
                Radius = -1;
                ImageSize = new OpenCvSharp.Size(0, 0);
            }

            public bool NeedsUpdate(OpenCvSharp.Size imageSize, int radius)
            {
                return Mask == null ||
                       Radius != radius ||
                       ImageSize != imageSize;
            }
        }

        public class FrameRateCalculator
        {
            private const int DefaultSampleSize = 10;
            private readonly int _sampleSize;
            private readonly Queue<long> _frameTimes = new();
            private readonly System.Diagnostics.Stopwatch _stopwatch = new();
            private long _lastTick;
            public double FrameRate { get; private set; }

            public FrameRateCalculator(int sampleSize = DefaultSampleSize)
            {
                _sampleSize = sampleSize;
            }

            public void Start()
            {
                if (!_stopwatch.IsRunning)
                    _stopwatch.Start();
            }

            public void Restart()
            {
                _frameTimes.Clear();
                _lastTick = 0;
                FrameRate = 0;
                _stopwatch.Restart();
            }

            public void Stop() => _stopwatch.Stop();

            /// <summary>
            /// 프레임 도착 시 호출. FrameRate 프로퍼티가 갱신됩니다.
            /// </summary>
            public void Tick()
            {
                long now = _stopwatch.ElapsedMilliseconds;
                long delta = now - _lastTick;
                _lastTick = now;

                if (delta <= 0) return;

                _frameTimes.Enqueue(delta);
                if (_frameTimes.Count > _sampleSize)
                    _frameTimes.Dequeue();

                FrameRate = 1000.0 / _frameTimes.Average();
            }
        }
    }
}
