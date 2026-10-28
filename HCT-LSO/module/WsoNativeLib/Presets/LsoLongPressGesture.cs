using System;

namespace WsoNativeLib.Presets
{
    public enum LsoLongPressEvent
    {
        None,
        DragStart,
        DragMove,
        Cancelled,
        DragEnd,
    }

    // Press-and-hold-then-drag recogniser. Time is passed in by the caller (no clock inside), so
    // the owner drives it from its own timer. Not thread-safe: one owner thread only.
    public sealed class LsoLongPressGesture
    {
        public const int HoldMs = 500;
        public const double SlopDip = 4.0;
        public const int SendIntervalMs = 100;

        private enum State
        {
            Idle,
            Pending,
            Dragging,
            Cancelled,
        }

        private State _state = State.Idle;
        private double _pressX;
        private double _pressY;
        private long _pressTMs;
        private long? _lastSendMs;

        public void Press(double x, double y, long tMs)
        {
            _state = State.Pending;
            _pressX = x;
            _pressY = y;
            _pressTMs = tMs;
            _lastSendMs = null;
        }

        public LsoLongPressEvent Tick(long tMs)
        {
            if (_state == State.Pending && tMs - _pressTMs >= HoldMs)
            {
                _state = State.Dragging;
                return LsoLongPressEvent.DragStart;
            }

            return LsoLongPressEvent.None;
        }

        public LsoLongPressEvent Move(double x, double y, long tMs)
        {
            switch (_state)
            {
                case State.Pending:
                    double dx = x - _pressX;
                    double dy = y - _pressY;
                    if (Math.Sqrt(dx * dx + dy * dy) > SlopDip)
                    {
                        _state = State.Cancelled;
                        return LsoLongPressEvent.Cancelled;
                    }

                    return LsoLongPressEvent.None;
                case State.Dragging:
                    return LsoLongPressEvent.DragMove;
                default:
                    return LsoLongPressEvent.None;
            }
        }

        public LsoLongPressEvent Release()
        {
            bool wasDragging = _state == State.Dragging;
            _state = State.Idle;
            return wasDragging ? LsoLongPressEvent.DragEnd : LsoLongPressEvent.None;
        }

        // True when at least SendIntervalMs have passed since the last true answer; records the send.
        public bool ShouldSend(long tMs)
        {
            if (_lastSendMs is long last && tMs - last < SendIntervalMs)
            {
                return false;
            }

            _lastSendMs = tMs;
            return true;
        }
    }
}
