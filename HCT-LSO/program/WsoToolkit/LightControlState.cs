using System;
using System.Collections.Generic;
using WsoNativeLib;
using static WsoNativeLib.WsoDevice;

namespace WsoToolkit
{
    /// <summary>
    /// One process-wide copy of the LED values the toolkit screens drive. The DevKit keeps the
    /// same state in lso_device (HBS_LED_Info + LED_ContinuousMode) and lets its LED form and
    /// pattern form mirror each other; this is the toolkit equivalent. A control publishes a
    /// value it has just committed and every other control mirrors it, so the scan-test
    /// alignment block and the LightControlPanel cannot show different numbers.
    /// Notifications only update editors - they never write the board - and the publisher
    /// suppresses the programmatic control update with its own sync flag.
    /// </summary>
    public static class LightControlState
    {
        public const int IntensityMax = 255;

        public sealed class ChangedEventArgs : EventArgs
        {
            public ChangedEventArgs(LightType type, int intensity, bool continuous,
                bool intensityChanged, bool continuousChanged)
            {
                Type = type;
                Intensity = intensity;
                Continuous = continuous;
                IntensityChanged = intensityChanged;
                ContinuousChanged = continuousChanged;
            }

            public LightType Type { get; }
            public int Intensity { get; }
            public bool Continuous { get; }
            public bool IntensityChanged { get; }
            public bool ContinuousChanged { get; }
        }

        private sealed class ChannelState
        {
            public int Intensity;
            public bool Continuous;
            public bool HasIntensity;
            public bool HasContinuous;
        }

        private static readonly object Gate = new();
        private static readonly Dictionary<LightType, ChannelState> Channels = new();
        private static bool _publishing;

        /// <summary>Raised after a control published an intensity and/or mode change.</summary>
        public static event EventHandler<ChangedEventArgs>? Changed;

        public static int ClampIntensity(int value)
        {
            return Math.Clamp(value, 0, IntensityMax);
        }

        public static void PublishIntensity(LightType type, int intensity)
        {
            Publish(type, ClampIntensity(intensity), null);
        }

        public static void PublishContinuous(LightType type, bool continuous)
        {
            Publish(type, null, continuous);
        }

        /// <summary>Last value published for the channel, if both fields are known.</summary>
        public static bool TryGet(LightType type, out int intensity, out bool continuous)
        {
            lock (Gate)
            {
                if (Channels.TryGetValue(type, out ChannelState? state)
                    && state.HasIntensity && state.HasContinuous)
                {
                    intensity = state.Intensity;
                    continuous = state.Continuous;
                    return true;
                }
            }

            intensity = 0;
            continuous = false;
            return false;
        }

        private static void Publish(LightType type, int? intensity, bool? continuous)
        {
            ChangedEventArgs args;

            lock (Gate)
            {
                if (!Channels.TryGetValue(type, out ChannelState? state))
                {
                    state = new ChannelState();
                    Channels[type] = state;
                }

                bool intensityChanged = intensity.HasValue
                    && (!state.HasIntensity || state.Intensity != intensity.Value);
                bool continuousChanged = continuous.HasValue
                    && (!state.HasContinuous || state.Continuous != continuous.Value);

                if (!intensityChanged && !continuousChanged)
                {
                    return;
                }

                if (intensityChanged)
                {
                    state.Intensity = intensity!.Value;
                    state.HasIntensity = true;
                }
                if (continuousChanged)
                {
                    state.Continuous = continuous!.Value;
                    state.HasContinuous = true;
                }

                // Echo guard: a subscriber that publishes back into the state while handling
                // a notification would bounce through the other controls. Drop that nested
                // notification; the value is stored, but nobody re-enters.
                if (_publishing)
                {
                    return;
                }
                _publishing = true;

                args = new ChangedEventArgs(type, state.Intensity, state.Continuous,
                    intensityChanged, continuousChanged);
            }

            try
            {
                Changed?.Invoke(null, args);
            }
            finally
            {
                lock (Gate)
                {
                    _publishing = false;
                }
            }
        }
    }
}
