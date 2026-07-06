using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Threading;
using WsoNativeLib;
using WsoToolkit.utils;

namespace WsoToolkit
{
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoDevice;

    public partial class LaserControlWindow
    {
        OctSldStatusParam _sldStatus = new();

        DispatcherTimer _timer = new DispatcherTimer();


        // Initialize window controls 
        //////////////////////////////////////////////////////////////////////////////////////////
        ///
        private void InitializeControls()
        {
            bool flag = DeviceLights.IsOctSldOn();
            btnSldOn.IsEnabled = !flag;
            btnSldOff.IsEnabled = flag;

            PopulateStatusControls();
            PopulateCalibParamControls();
        }


        // Fecth all Laser status 
        //////////////////////////////////////////////////////////////////////////////////////////
        ///
        private void RetrieveDeviceStatus()
        {
            DeviceLights.ObtainOctSldStatusParam(ref _sldStatus, true);
        }


        // Update window controls
        //////////////////////////////////////////////////////////////////////////////////////////
        ///
        private void PopulateStatusControls()
        {
            editSldSldCurrent.Text = _sldStatus.sldCurrent.ToString("N4");
            editSldIpdCurrent.Text = _sldStatus.ipdCurrent.ToString("N4");
            editSldExternPd.Text = _sldStatus.externalPd.ToString("N4");
            editSldTemperature.Text = _sldStatus.temperature.ToString("N4");
        }

        private void PopulateCalibParamControls()
        {
            ref var sldParam = ref _sldStatus.calibParam;
            editSldHighCode.Text = sldParam.highCode.ToString();
            editSldLowCode1.Text = sldParam.lowCode1.ToString();
            editSldLowCode2.Text = sldParam.lowCode2.ToString();
            editSldRsiCode.Text = sldParam.rsiCode.ToString();
        }

        private void SubmitCalibrationParams()
        {
            SubmitHighCode();
            SubmitLowCode1();
            SubmitLowCode2();
            SubmitRsiCode();
        }

        private void SubmitHighCode()
        {
            int value = NumberUtil.ToInt(editSldHighCode.Text);
            if (value >= 0 && value < 4096)
            {
                DeviceLights.SetOctSldHighCode(value);
            }
        }

        private void SubmitLowCode1()
        {
            int value = NumberUtil.ToInt(editSldLowCode1.Text);
            if (value >= 0 && value < 4096)
            {
                DeviceLights.SetOctSldLowCode1(value);
            }
        }

        private void SubmitLowCode2()
        {
            int value = NumberUtil.ToInt(editSldLowCode2.Text);
            if (value >= 0 && value < 4096)
            {
                DeviceLights.SetOctSldLowCode2(value);
            }
        }

        private void SubmitRsiCode()
        {
            int value = NumberUtil.ToInt(editSldRsiCode.Text);
            if (value >= 0 && value < 4096)
            {
                DeviceLights.SetOctSldRsiCode(value);
            }
        }

        private void StartRefreshTimer()
        {
            _timer.Interval = TimeSpan.FromSeconds(1.0);
            _timer.Tick += TimerRefreshTick;
            _timer.Start();
        }

        private void TimerRefreshTick(object? sender, EventArgs e)
        {
            RetrieveDeviceStatus();
            PopulateStatusControls();
        }

        private void CalculateOctSldRsiCode()
        {
            float current = ToFloat(editSldSldCurrent.Text);
            current += 10.0f;
            int rsiCode = current switch
            {
                >= 180.0f => 0,
                >= 171.02f => 0,
                >= 162.90f => 1,
                >= 155.51f => 2,
                >= 148.76f => 3,
                >= 142.58f => 4,
                >= 136.89f => 5,
                >= 131.63f => 6,
                >= 126.77f => 7,
                >= 122.25f => 8,
                >= 118.04f => 9,
                >= 114.11f => 10,
                >= 110.43f => 11,
                _ => 12,
            };

            editSldRsiCode.Text = rsiCode.ToString();
        }
    }
}
