using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Threading;
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.NumberUtil;

    public partial class XadcMonitorWindow
    {
        WsoDevice.AdcSensorStatus _adcSensor = new();

        DispatcherTimer _timer = new DispatcherTimer();


        // Fecth all Laser status 
        //////////////////////////////////////////////////////////////////////////////////////////
        ///
        private void FetchAllDeviceStatus()
        {
            DeviceLights.ObtainAdcSensorStatus(ref _adcSensor, true);
        }

        // Update window controls
        //////////////////////////////////////////////////////////////////////////////////////////
        ///
        private void UpdateCurrentStatus()
        {
            editZyncCpuTemp.Text = _adcSensor.cpuTempature.ToString("N4");
            editZyncExternPd.Text = _adcSensor.externalMonitorPd.ToString();
            editZyncHallSensor1.Text = _adcSensor.lensHallSensor1.ToString();
            editZyncHallSensor2.Text = _adcSensor.lensHallSensor2.ToString();
            editZyncOdos.Text = _adcSensor.odOs.ToString();
        }

        private void StartRefreshTimer()
        {
            _timer.Interval = TimeSpan.FromSeconds(1.0);
            _timer.Tick += TimerRefreshTick;
            _timer.Start();
        }

        private void TimerRefreshTick(object? sender, EventArgs e)
        {
            FetchAllDeviceStatus();
            UpdateCurrentStatus();
        }
    }
}
