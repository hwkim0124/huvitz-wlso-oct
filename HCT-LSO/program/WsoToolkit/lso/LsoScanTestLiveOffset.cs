using System;
using System.Windows.Threading;
using WsoNativeLib;
using WsoNativeLib.Presets;

namespace WsoToolkit
{
    public partial class LsoScanTestWindow
    {
        private LsoLiveOffsetUpdates? _liveOffsetUpdates;
        private ColorCameraPresetMode? _liveOffsetSourceMode;

        private ColorCameraPresetMode? liveOffsetMode_()
        {
            if (_liveCapture)
                return ColorCameraPresetMode.Capture;
            if (!_alignActive && IsRetinaPreviewing_())
                return ColorCameraPresetMode.Preview;
            return null;
        }

        private void queueLiveOffsetUpdate_()
        {
            if (_syncUpdating || !IsLoaded)
                return;
            ColorCameraPresetMode? mode = liveOffsetMode_();
            if (!mode.HasValue)
                return;

            _liveOffsetUpdates ??= new LsoLiveOffsetUpdates(
                action => Dispatcher.BeginInvoke(DispatcherPriority.Background, action),
                writeLiveOffset_);
            _liveOffsetSourceMode = mode;
            _liveOffsetUpdates.Request(mode.Value, SyncBothModes,
                Math.Max(0, WsoToolkit.utils.NumberUtil.ToInt(myTbSyncBlank.Text)));
        }

        // 창의 기존 네이티브 호출과 같은 Dispatcher에서만 실행한다.
        // 완료 콜백에서는 호출하지 않는다. DevKit처럼 스캐너/카메라 정지 없이 RAM만 쓴다.
        private void writeLiveOffset_(ColorCameraPresetMode mode, int row0)
        {
            if (!IsLoaded || liveOffsetMode_() != _liveOffsetSourceMode)
                return;
            var preset = _scanTestModel.ModePresets.Active(mode);
            mirrorCameraExposureIntoScan_(preset);
            var scan = preset.Scan;
            scan.SensorBlankingRows = row0;
            double delay = Math.Max(0, preset.Camera.TriggerDelayUs);
            double exposure = preset.Camera.ExposureTime > 0
                ? preset.Camera.ExposureTime : scan.ExposureTimeUs;
            if (!LsoScanTimingCalculator.TryCalculate(scan, delay, exposure,
                    out var timing, out string failure, desiredPreMoveDelayUs: scan.TimingPreMoveDelayUs))
            {
                myLbSyncStatus.Content = $"Live Row0 update failed: {failure}";
                return;
            }
            LsoScanTimingCalculator.ApplyTo(scan, timing);
            int slot = LsoModePreset.ScanPatternId(mode);
            if (!LsoModeTrajectory.TryLoad(scan, out short[]? table, out failure, out _)
                || !LsoModeTrajectory.Install(slot, table, out failure))
            {
                myLbSyncStatus.Content = $"Live Row0 update failed: {failure}";
                return;
            }
            var profile = scan.ToControlParam();
            // Preview는 연속, LiveCapture는 기존 유한 Capture를 반복한다.
            if (mode == ColorCameraPresetMode.Preview)
                profile.acquisitionMode = 0;
            if (!LsoScanner.SubmitLsoScannerControlParam(slot, profile))
            {
                myLbSyncStatus.Content = $"Live Row0 update failed: slot {slot} RAM write refused.";
                return;
            }
            Console.WriteLine($"[LSO-TRAJ] live update slot={slot} row0={row0} lead={scan.LeadInSamples}");
            reflectScanTiming_();
        }
    }
}
