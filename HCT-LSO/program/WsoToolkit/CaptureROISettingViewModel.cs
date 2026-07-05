using System.Collections.Generic;
using System.Windows;
using static WsoNativeLib.LsoCamera;
using static WsoNativeLib.WsoLsoScan;

namespace WsoToolkit
{
    public partial class CaptureROISettingWindow
    {
        public class FrameInfo
        {
            public int FrameNumber { get; set; }

            public int StartY { get; set; }

            public int EndY { get; set; }
        }

        private LsoColorCameraSettingParam _colorCameraParam = new LsoColorCameraSettingParam();

        private List<FrameRoiPosition> _presetRois = new List<FrameRoiPosition>();

        private void initControls_()
        {
            myDataGrid.ItemsSource = FrameItems;
        }

        private void updateControls_()
        {
            myLbFrameSize.Content = $"Frame Size : {_colorCameraParam.roiXWidth} x {_colorCameraParam.roiYHeight} ( W x H )";
        }

        private void loadCameraParam_()
        {
            GetCameraParameters(out _colorCameraParam);
        }

        private void loadFrameParam_()
        {
            _presetRois = new List<FrameRoiPosition>(_scanTestModel.ColorCamera.CaptureFrameRois);
        }

        private void updateDataGrid_()
        {
            FrameItems.Clear();

            int nFrameCount = _presetRois.Count;

            if (FrameCount <= nFrameCount)
            {
                for (int i = 0; i < FrameCount; ++i)
                {
                    FrameItems.Add(new FrameInfo
                    {
                        FrameNumber = i,
                        StartY = _presetRois[i].StartY,
                        EndY = _presetRois[i].EndY
                    });
                }
            }
            else
            {
                for (int i = 0; i < nFrameCount; ++i)
                {
                    FrameItems.Add(new FrameInfo
                    {
                        FrameNumber = i,
                        StartY = _presetRois[i].StartY,
                        EndY = _presetRois[i].EndY
                    });
                }

                for (int i = nFrameCount; i < FrameCount; ++i)
                {
                    FrameItems.Add(new FrameInfo
                    {
                        FrameNumber = i,
                        StartY = 0,
                        EndY = (int)_colorCameraParam.roiYHeight
                    });
                }
            }
        }

        private void uploadCaptureFrameROISettings_()
        {
            var rois = new List<FrameRoiPosition>(FrameCount);

            for (int nFrameIndex = 0; nFrameIndex < FrameCount; ++nFrameIndex)
            {
                rois.Add(new FrameRoiPosition
                {
                    StartY = FrameItems[nFrameIndex].StartY,
                    EndY = FrameItems[nFrameIndex].EndY,
                });
            }

            _scanTestModel.ColorCamera.SetCaptureFrameRois(rois);
        }
    }
}
