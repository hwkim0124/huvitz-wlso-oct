using System;

namespace WsoNativeLib.Presets
{
    // 호출자는 같은 직렬 실행기에서 Request와 게시된 작업을 실행한다.
    // 현재 쓰기가 끝날 때까지 대기 값 하나만 유지한다. post는 인라인 실행하지 않는다.
    public sealed class LsoLiveOffsetUpdates
    {
        private readonly Action<Action> _post;
        private readonly Action<ColorCameraPresetMode, int> _write;
        private (ColorCameraPresetMode mode, bool both, int row0)? _pending;
        private bool _scheduled;

        public LsoLiveOffsetUpdates(Action<Action> post, Action<ColorCameraPresetMode, int> write)
        {
            _post = post;
            _write = write;
        }

        public void Request(ColorCameraPresetMode liveMode, bool both, int row0)
        {
            _pending = (liveMode, both, row0);
            if (_scheduled)
                return;
            _scheduled = true;
            _post(Drain);
        }

        private void Drain()
        {
            var update = _pending!.Value;
            _pending = null;
            try
            {
                if (update.both)
                {
                    _write(ColorCameraPresetMode.Preview, update.row0);
                    _write(ColorCameraPresetMode.Capture, update.row0);
                }
                else
                {
                    _write(update.mode, update.row0);
                }
            }
            finally
            {
                if (_pending.HasValue)
                    _post(Drain);
                else
                    _scheduled = false;
            }
        }
    }
}
