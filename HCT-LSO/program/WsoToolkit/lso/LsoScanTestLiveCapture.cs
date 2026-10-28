using System.Windows;
using WsoNativeLib;
using static WsoNativeLib.WsoLsoDefs;

namespace WsoToolkit
{
    public partial class LsoScanTestWindow
    {
        private bool _liveCapture;
        private int _liveCaptureGeneration;

        private void myBtLiveCapture_Click(object sender, RoutedEventArgs e)
        {
            if (_liveCapture)
            {
                stopLiveCapture_();
                return;
            }
            if (LsoCamera.IsCapturing())
                return;
            if (_alignActive)
                myCheckAlignMode.IsChecked = false;
            if (IsRetinaPreviewing_())
                StopRetinaPreview_();
            if (IsColorCameraLive())
                PauseColorCameraLive();

            myCbAlignTargetMode.SelectedIndex = 1;
            _liveCapture = true;
            myBtLiveCapture.Content = "Stop Live";
            startLiveCaptureFrame_();
        }

        private void stopLiveCapture_()
        {
            _liveCaptureGeneration++;
            _liveCapture = false;
            myBtLiveCapture.Content = "Live Capture";
        }

        private void startLiveCaptureFrame_()
        {
            if (!_liveCapture || !IsLoaded)
                return;
            readyToCapture_();
            if (!StartColorCameraOriginal())
                stopLiveCapture_();
        }

        private void continueLiveCapture_(ColorCaptureStage stage)
        {
            if (!_liveCapture)
                return;
            if (stage != ColorCaptureStage.Ok)
            {
                stopLiveCapture_();
                return;
            }
            // UI dispatcher, never the native callback: Stop joins the completed worker.
            // Native clears s_capturing AFTER notifying completion, so a direct restart races it.
            if (!LsoCamera.StopColorCapture())
            {
                stopLiveCapture_();
                MessageBox.Show("Live Capture stopped: the previous capture worker could not be joined.");
                return;
            }
            int generation = _liveCaptureGeneration;
            Dispatcher.BeginInvoke(new System.Action(() =>
            {
                if (generation == _liveCaptureGeneration)
                    startLiveCaptureFrame_();
            }));
        }
    }
}
