using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WsoNativeLib;
using WsoNativeLib.Presets;
using static WsoNativeLib.WsoDevice;

namespace WsoToolkit.utils
{
    public enum JogAxis
    {
        X,
        Y,
        Z,
    }

    // Owns the per-axis busy registry (velocity holds plus DeviceMotors.IsStepMotorMoving); every caller
    // that starts a stage move (jog pad, wheel, cornea click, chinrest, Advanced tab) must go through
    // TryStep / IsAxisFree / StartVelocityHold first. Every rule decision goes through LsoStageJogRules.
    // Thread contract: callers are the UI thread and the hold watchdog timer thread; all hold and
    // range bookkeeping is guarded by _lock. No UI code here: event subscribers marshal themselves.
    public enum HoldStartResult
    {
        Started,
        Busy,
        Rejected,
        RacedStopped,
        RacedStopFailed,
    }

    internal static class StageJog
    {
        public const int HoldWatchdogMs = 10000;
        private const int StopConfirmPollMs = 25;
        private const int StopConfirmTotalMs = 100;

        private sealed class Hold
        {
            public required string Owner;
            public required Action Stop;
            public required JogAxis[] Axes;
            public Timer? Watchdog;
        }

        private static readonly object _lock = new();
        private static readonly Dictionary<string, Hold> _holds = new();
        private static readonly Dictionary<JogAxis, (int Min, int Max)> _ranges = new();

        // Raised on a thread-pool thread with the motors that still report a move in flight.
        public static event Action<string>? StopUnconfirmed;

        // Raised on the watchdog timer thread with the owner whose hold ran past HoldWatchdogMs.
        public static event Action<string>? HoldTimedOut;

        // Raised on the thread that ran a hold's stop action (UI or watchdog) when that action threw:
        // owner and message. The device may still be moving.
        public static event Action<string, string>? HoldStopFailed;

        public static MotorType ToMotor(JogAxis axis) => axis switch
        {
            JogAxis.X => MotorType.StageX,
            JogAxis.Y => MotorType.StageY,
            JogAxis.Z => MotorType.StageZ,
            _ => throw new ArgumentOutOfRangeException(nameof(axis)),
        };

        // Step in motor positions for this axis and size; uses the fallback travel until the board
        // range has been read once successfully (a failed read is retried on the next call).
        public static int StepFor(JogAxis axis, LsoJogSize size)
        {
            int travel = TryGetRange(axis, out int min, out int max)
                ? LsoStageJogRules.Travel(ToMotor(axis), min, max)
                : LsoStageJogRules.Travel(ToMotor(axis), 0, 0);
            return LsoStageJogRules.StepFor(size, travel);
        }

        // sent is the clamped signed step actually sent (0 when refused).
        public static bool TryStep(JogAxis axis, LsoJogSize size, int direction, out int sent, out string refusal)
        {
            if (direction == 0)
            {
                throw new ArgumentOutOfRangeException(nameof(direction));
            }

            MotorType type = ToMotor(axis);
            int requested = Math.Sign(direction) * StepFor(axis, size);
            string? reason;
            int clamped = 0;

            if (!DeviceMotors.FetchStepMotorStatus(type, out StepMotorStatus status))
            {
                reason = "no status";
            }
            else if (!TryGetRange(axis, out int min, out int max))
            {
                reason = "no range";
            }
            else
            {
                clamped = LsoStageJogRules.ClampStep(status.currPos, requested, min, max);
                lock (_lock)
                {
                    reason = LsoStageJogRules.Refusal(
                        LsoCamera.IsCapturing(), DeviceMotors.IsStepMotorMoving(type), IsHeldLocked(axis), clamped);
                    if (reason == null && !DeviceMotors.MoveStepMotorByPositionOffsetAsync(type, clamped))
                    {
                        reason = "rejected";
                    }
                }
            }

            if (reason != null)
            {
                Console.WriteLine($"[LSO-JOG] step {axis} {requested:+0;-0} refused: {reason}");
                sent = 0;
                refusal = reason;
                return false;
            }

            Console.WriteLine($"[LSO-JOG] step {axis} {clamped:+0;-0} ok");
            sent = clamped;
            refusal = string.Empty;
            return true;
        }

        // Checked before a caller starts a move on this axis (cornea click, Advanced tab) and by the badges.
        public static bool IsAxisFree(JogAxis axis, out string refusal)
        {
            if (LsoCamera.IsCapturing())
            {
                refusal = "capture running";
                return false;
            }

            lock (_lock)
            {
                if (DeviceMotors.IsStepMotorMoving(ToMotor(axis)) || IsHeldLocked(axis))
                {
                    refusal = "busy";
                    return false;
                }
            }

            refusal = string.Empty;
            return true;
        }

        public static void StopAxis(JogAxis axis)
        {
            DeviceMotors.StopStepMotor(ToMotor(axis));
            Console.WriteLine($"[LSO-JOG] stop {axis}");
        }

        public static bool IsAxisHeld(JogAxis axis)
        {
            lock (_lock)
            {
                return IsHeldLocked(axis);
            }
        }

        // Returns the names of the stop calls that returned false (empty when all were accepted).
        public static IReadOnlyList<string> StopAll()
        {
            foreach (MotorType motor in LsoStageJogRules.StopAllMotors)
            {
                DeviceMotors.StopStepMotor(motor);
            }

            var failed = new List<string>();
            if (!AlignStage.StopHeadXYZ())
            {
                failed.Add("StopHeadXYZ");
            }
            if (!AlignStage.StopSwingMove())
            {
                failed.Add("StopSwingMove");
            }
            if (!AlignStage.StopChinrest())
            {
                failed.Add("StopChinrest");
            }

            List<Hold> cleared;
            lock (_lock)
            {
                cleared = _holds.Values.ToList();
                _holds.Clear();
            }

            Console.WriteLine(failed.Count == 0
                ? "[LSO-JOG] stop all"
                : $"[LSO-JOG] stop all (returned false: {string.Join(", ", failed)})");

            // A hold removed here is owned by nobody else any more, so its stop runs exactly once.
            foreach (Hold hold in cleared)
            {
                hold.Watchdog?.Dispose();
                RunHoldStop(hold, "cleared");
            }

            Task.Run(ConfirmStoppedAsync).ContinueWith(
                t => Console.WriteLine($"[LSO-JOG] stop all confirm failed: {t.Exception}"),
                TaskContinuationOptions.OnlyOnFaulted);
            return failed;
        }

        // Registers a velocity move owned by the caller, then runs start. Busy when the owner already
        // holds or any axis is held by another owner (start does not run). Rejected when start returns
        // false (the hold is ended, which runs stop). A StopAll or watchdog on another thread can remove
        // the hold before start sent its move, so its stop may have preceded the move: the hold is then
        // gone and stopAgain runs (RacedStopped / RacedStopFailed). UI thread only.
        // stop must be non-blocking (device Stop* calls, BeginInvoke, Interlocked; never Dispatcher.Invoke):
        // it can run on the watchdog timer thread.
        public static HoldStartResult StartVelocityHold(string owner, Action stop, Func<bool> start, Func<bool> stopAgain, params JogAxis[] axes)
        {
            Hold hold;
            lock (_lock)
            {
                if (_holds.ContainsKey(owner) || axes.Any(IsHeldLocked))
                {
                    Console.WriteLine($"[LSO-JOG] hold begin {owner} refused: busy");
                    return HoldStartResult.Busy;
                }

                hold = new Hold { Owner = owner, Stop = stop, Axes = axes };
                _holds.Add(owner, hold);
                // Assigned under _lock: OnHoldTimeout takes _lock first, so it cannot run before Watchdog is set.
                hold.Watchdog = new Timer(OnHoldTimeout, hold, HoldWatchdogMs, Timeout.Infinite);
            }

            Console.WriteLine($"[LSO-JOG] hold begin {owner}");
            bool started;
            try
            {
                started = start();
            }
            catch
            {
                EndVelocityHold(owner);
                throw;
            }

            bool raced;
            lock (_lock)
            {
                raced = !(_holds.TryGetValue(owner, out Hold? current) && ReferenceEquals(current, hold));
            }

            if (raced)
            {
                bool stopped = stopAgain();
                Console.WriteLine($"[LSO-JOG] hold {owner} raced a stop: stopped again {(stopped ? "ok" : "returned false")}");
                return stopped ? HoldStartResult.RacedStopped : HoldStartResult.RacedStopFailed;
            }

            if (!started)
            {
                EndVelocityHold(owner);
                return HoldStartResult.Rejected;
            }

            return HoldStartResult.Started;
        }

        // Idempotent: only the caller that removes the hold from the registry (this, the watchdog or
        // StopAll) runs its stop, so a second End or an End after a timeout does nothing.
        public static void EndVelocityHold(string owner)
        {
            Hold? hold;
            lock (_lock)
            {
                if (!_holds.Remove(owner, out hold))
                {
                    return;
                }
            }

            hold.Watchdog?.Dispose();
            RunHoldStop(hold, "end");
        }

        private static void OnHoldTimeout(object? state)
        {
            var hold = (Hold)state!;
            lock (_lock)
            {
                if (!_holds.TryGetValue(hold.Owner, out Hold? current) || !ReferenceEquals(current, hold))
                {
                    return;
                }
                _holds.Remove(hold.Owner);
            }

            hold.Watchdog?.Dispose();
            RunHoldStop(hold, "timeout");
            try
            {
                StopAll();
            }
            catch (Exception ex)
            {
                // An escaping exception on the watchdog timer thread would end the process.
                Console.WriteLine($"[LSO-JOG] hold timeout {hold.Owner}: StopAll threw {ex.GetType().Name}: {ex.Message}");
            }

            try
            {
                HoldTimedOut?.Invoke(hold.Owner);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[LSO-JOG] hold timeout {hold.Owner}: HoldTimedOut threw {ex.GetType().Name}: {ex.Message}");
            }
        }

        private static void RunHoldStop(Hold hold, string kind)
        {
            Console.WriteLine($"[LSO-JOG] hold {kind} {hold.Owner}");
            try
            {
                hold.Stop();
            }
            catch (Exception ex)
            {
                // Not rethrown: on the watchdog thread a throw would skip StopAll and end the process.
                // Surfaced through HoldStopFailed instead.
                Console.WriteLine($"[LSO-JOG] hold {kind} {hold.Owner} stop failed: {ex}");
                try
                {
                    HoldStopFailed?.Invoke(hold.Owner, ex.Message);
                }
                catch (Exception notifyEx)
                {
                    Console.WriteLine($"[LSO-JOG] hold {kind} {hold.Owner}: HoldStopFailed threw {notifyEx.GetType().Name}: {notifyEx.Message}");
                }
            }
        }

        private static async Task ConfirmStoppedAsync()
        {
            List<MotorType> moving = new();
            for (int waited = 0; waited < StopConfirmTotalMs; waited += StopConfirmPollMs)
            {
                await Task.Delay(StopConfirmPollMs).ConfigureAwait(false);
                moving = LsoStageJogRules.StopAllMotors.Where(DeviceMotors.IsStepMotorMoving).ToList();
                if (moving.Count == 0)
                {
                    return;
                }
            }

            // A step task can send its move after the StopAll reached the device (the native move clears the
            // stop), so the motors still moving are stopped once more before reporting.
            foreach (MotorType motor in moving)
            {
                DeviceMotors.StopStepMotor(motor);
            }

            string motors = string.Join(", ", moving);
            Console.WriteLine($"[LSO-JOG] stop all unconfirmed: {motors} (stop sent again)");
            StopUnconfirmed?.Invoke(motors);
        }

        private static bool IsHeldLocked(JogAxis axis)
        {
            foreach (Hold hold in _holds.Values)
            {
                if (Array.IndexOf(hold.Axes, axis) >= 0)
                {
                    return true;
                }
            }
            return false;
        }

        // Caches the board range of an axis after the first valid read; failures are not cached.
        private static bool TryGetRange(JogAxis axis, out int min, out int max)
        {
            lock (_lock)
            {
                if (_ranges.TryGetValue(axis, out var cached))
                {
                    (min, max) = cached;
                    return true;
                }
            }

            if (DeviceMotors.FetchStepMotorStatus(ToMotor(axis), out StepMotorStatus status)
                && status.rangeMax > status.rangeMin)
            {
                lock (_lock)
                {
                    _ranges[axis] = (status.rangeMin, status.rangeMax);
                }
                min = status.rangeMin;
                max = status.rangeMax;
                return true;
            }

            min = 0;
            max = 0;
            return false;
        }
    }
}
