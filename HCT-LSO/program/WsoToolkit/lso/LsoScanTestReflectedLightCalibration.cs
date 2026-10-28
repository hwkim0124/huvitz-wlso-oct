using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using WsoNativeLib;
using WsoNativeLib.Presets;
using static WsoNativeLib.WsoDevice;
using static WsoNativeLib.WsoLsoDefs;

namespace WsoToolkit
{
    public partial class LsoScanTestWindow
    {
        private volatile ReflectedLightCaptureSink? _reflectedLightCalSink;

        private sealed class ReflectedLightCaptureSink
        {
            internal readonly TaskCompletionSource<(byte[] Frame, int Flip)> Frame = new(TaskCreationOptions.RunContinuationsAsynchronously);
            internal readonly TaskCompletionSource<int> Stage = new(TaskCreationOptions.RunContinuationsAsynchronously);
            internal string Error = string.Empty;
            private readonly uint _width, _height;
            private int _offered;

            internal ReflectedLightCaptureSink(uint width, uint height) { _width = width; _height = height; }

            // Native thread: managed data only. First frame wins, including malformed frames.
            internal void Offer(byte[] data, int width, int height, int bytesPerPixel, int flip)
            {
                if (Interlocked.Exchange(ref _offered, 1) != 0) return;
                if (width != _width || height != _height || bytesPerPixel != 2
                    || data.LongLength != (long)_width * _height * 2)
                    Error = $"Calibration needs a {_width}x{_height}x2 byte frame; received {width}x{height}x{bytesPerPixel}.";
                Frame.TrySetResult((data, flip));
            }
        }

        private void myBtReflectedLightCal_Click(object sender, RoutedEventArgs e)
        {
            string? reason = reflectedLightCalibrationUnavailable_();
            if (reason != null)
            {
                MessageBox.Show(this, reason, "Reflected Light Calibration", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            new LsoReflectedLightCalibrationWindow(this) { Owner = this }.ShowDialog();
        }

        private string? reflectedLightCalibrationUnavailable_()
        {
            if (_scanTestModel.ModePresets.CaptureImageMode != LsoImageMode.Color
                || _scanTestModel.ModePresets.Capture.ColorIllumination != ColorIllumination.White)
                return "Reflected light calibration requires Color capture with White illumination.";
            if (LsoCamera.IsCapturing())
                return "A capture is in progress. Wait for it to finish before calibrating.";
            return null;
        }

        internal void ValidateReflectedLightCalibration(LsoReflectedLightRoi roi, int frames,
            double from, double to, double step)
        {
            if (frames < 1 || frames > 64)
                throw new ArgumentException("Frames per point must be from 1 to 64.");
            var camera = _scanTestModel.ModePresets.Capture.Camera;
            if (roi.X < 0 || roi.Y < 0 || roi.Width <= 0 || roi.Height <= 0
                || ((roi.X | roi.Y | roi.Width | roi.Height) & 1) != 0
                || (long)roi.X + roi.Width > camera.RoiXWidth || (long)roi.Y + roi.Height > camera.RoiYHeight)
                throw new ArgumentException("ROI X/Y/W/H must be even, positive in size, and inside the capture ROI.");
            if (from < -15 || to > 15 || from > to || !double.IsFinite(from) || !double.IsFinite(to)
                || !double.IsFinite(step) || step <= 0)
                throw new ArgumentException("Focus range must be within -15 to +15 D, start <= end, and step > 0.");
            var points = LsoReflectedLightCalibrator.PlanPoints(from, to, step, mapLsoFocusValueToPosition_);
            var positions = new HashSet<int>();
            foreach (var point in points)
            {
                if (point.Diopter > to || !positions.Add(point.Target))
                    throw new ArgumentException("Choose a step that ends within the range and gives distinct focus positions.");
            }
        }

        internal async Task<LsoReflectedLightCalibrationResult> RunReflectedLightCalibrationAsync(
            LsoReflectedLightRoi roi, int frames, double from, double to, double step,
            IProgress<LsoReflectedLightProgress> progress, CancellationToken token)
        {
            string? reason = reflectedLightCalibrationUnavailable_();
            if (reason != null) throw new InvalidOperationException(reason);
            if (DeviceMotors.IsStepMotorMoving(MotorType.LsoFocus))
                throw new InvalidOperationException("The LSO focus motor is still moving; wait for it to stop and start again.");
            stopLiveCapture_();
            if (IsRetinaPreviewing_()) StopRetinaPreview_();
            if (IsColorCameraLive()) PauseColorCameraLive();

            var copy = _scanTestModel.ModePresets.Capture.Clone(_scanTestModel.ModePresets.Capture.Name);
            copy.Motor.LsoFocusPosition = LsoModeMotor.NotStated;
            copy.PostProcess.FlatFieldEnabled = false;
            copy.PostProcess.ReflectedLightEnabled = false;
            if (!LsoFundusModeCapture.TryPrepare(LsoImageMode.Color, copy, _scanTestModel.ModePresets.Preview,
                ColorCaptureTriggerMode.HWTrigger, currentCameraParam_(), DeviceLights.GetLightIntensity,
                perFrame: true, out var prepared, out string failure) || prepared == null)
                throw new InvalidOperationException(failure);
            if (prepared.TotalFrames != 1)
                throw new InvalidOperationException("calibration needs a preset that acquires one frame per capture");
            if ((long)roi.X + roi.Width > prepared.Request.camera.roiXWidth
                || (long)roi.Y + roi.Height > prepared.Request.camera.roiYHeight)
                throw new InvalidOperationException("ROI is outside the prepared capture frame.");
            if (prepared.Request.camera.pixelFormat != (uint)ColorPixelFormat.BayerRG12Packed)
                throw new InvalidOperationException("Calibration requires BayerRG12Packed.");
            var points = LsoReflectedLightCalibrator.PlanPoints(from, to, step, mapLsoFocusValueToPosition_);
            return await Task.Run(async () =>
            {
                token.ThrowIfCancellationRequested();
                if (!LsoModeMotorApply.Apply(copy.Motor, out string motorFailure))
                    throw new InvalidOperationException(motorFailure);
                if (!prepared.InstallTrajectory(out string trajectoryFailure))
                    throw new InvalidOperationException(trajectoryFailure);
                var probe = await captureReflectedLightFrame_(prepared, token).ConfigureAwait(false);
                var conditions = LsoFundusModeCapture.ReflectedLightConditions(prepared.Request, probe.Flip);
                var engine = new LsoReflectedLightCalibrator(moveReflectedLightFocus_, async cancel =>
                {
                    var frame = await captureReflectedLightFrame_(prepared, cancel).ConfigureAwait(false);
                    if (frame.Flip != conditions.FlipMode)
                        throw new InvalidOperationException($"Calibration flip changed: {frame.Flip} != {conditions.FlipMode}.");
                    return frame.Frame;
                }, readReflectedLightFocus_);
                var logged = new ProgressForwarder(progress);
                return await engine.RunAsync(LsoConfigLocation.ReflectedLightDirectory(), points, frames,
                    roi, conditions, logged, token).ConfigureAwait(false);
            });
        }

        internal void AfterReflectedLightCalibration() => refreshLightControls_();

        private sealed class ProgressForwarder : IProgress<LsoReflectedLightProgress>
        {
            private readonly IProgress<LsoReflectedLightProgress> _ui;
            internal ProgressForwarder(IProgress<LsoReflectedLightProgress> ui) { _ui = ui; }
            public void Report(LsoReflectedLightProgress value)
            {
                Console.WriteLine($"[LSO-REFL-CAL] {value.Text}; point {value.Point}/{value.PointCount}, frame {value.Frame}/{value.FramesPerPoint}");
                _ui.Report(value);
            }
        }

        private static int readReflectedLightFocus_()
        {
            if (!DeviceMotors.FetchStepMotorStatus(MotorType.LsoFocus, out var status))
                throw new InvalidOperationException("The LSO focus position could not be read.");
            return status.currPos;
        }

        private Task<int> moveReflectedLightFocus_(int target, CancellationToken token)
        {
            token.ThrowIfCancellationRequested();
            int current = readReflectedLightFocus_();
            if (Math.Abs((long)current - target) <= LsoReflectedLightCalibrator.ReachTolerance)
                return Task.FromResult(current);
            if (DeviceMotors.IsStepMotorMoving(MotorType.LsoFocus))
                throw new InvalidOperationException("The LSO focus motor is still moving; wait for it to stop and start again.");
            // Already on the calibration worker; finish the bounded move before cancellation can restore focus.
            bool reached = DeviceMotors.MoveStepMotorPosition(MotorType.LsoFocus, target);
            int position = readReflectedLightFocus_();
            if (!reached)
                Console.WriteLine($"[LSO-REFL-CAL] focus move to {target} ended at {position}");
            return Task.FromResult(position);
        }

        private async Task<(byte[] Frame, int Flip)> captureReflectedLightFrame_(
            LsoFundusModeCapture prepared, CancellationToken token)
        {
            token.ThrowIfCancellationRequested();
            var sink = new ReflectedLightCaptureSink(prepared.Request.camera.roiXWidth, prepared.Request.camera.roiYHeight);
            _reflectedLightCalSink = sink;
            bool started = false;
            try
            {
                using var bound = new CancellationTokenSource(TimeSpan.FromSeconds(15));
                var request = prepared.Request;
                var stage = LsoCamera.StartFundusCapture(ref request, _onColorCaptureImageCaptured, _onColorCaptureCompleted);
                if (stage != ColorCaptureStage.Ok)
                    throw new InvalidOperationException($"Calibration capture start failed: {stage} - {LsoCamera.GetLastCaptureError()}");
                started = true;
                int result;
                // Cancellation does not abandon an active one-call; its teardown must finish.
                try { result = await sink.Stage.Task.WaitAsync(bound.Token).ConfigureAwait(false); }
                catch (OperationCanceledException)
                {
                    throw new TimeoutException("Calibration capture timed out after 15 s.");
                }
                // Native clears its busy flag after the completion callback returns. Join it
                // here, outside that callback, before the next start or focus restoration.
                if (!LsoCamera.StopColorCapture())
                    throw new InvalidOperationException("Calibration capture worker could not be joined.");
                started = false;
                token.ThrowIfCancellationRequested();
                if ((ColorCaptureStage)result != ColorCaptureStage.Ok)
                    throw new InvalidOperationException($"Calibration capture failed: {(ColorCaptureStage)result} - {LsoCamera.GetLastCaptureError()}");
                if (!sink.Frame.Task.IsCompletedSuccessfully)
                    throw new InvalidOperationException("Calibration capture completed without a frame.");
                if (sink.Error.Length > 0) throw new InvalidOperationException(sink.Error);
                var frame = await sink.Frame.Task.ConfigureAwait(false);
                return frame;
            }
            finally
            {
                try
                {
                    // On timeout retain the sink until native teardown has disconnected frames.
                    if (started && !LsoCamera.StopColorCapture())
                        Console.WriteLine("[LSO-REFL-CAL] capture worker could not be stopped after a capture failure.");
                }
                finally { _reflectedLightCalSink = null; }
            }
        }
    }
}
