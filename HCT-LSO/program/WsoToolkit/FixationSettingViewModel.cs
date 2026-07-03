using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.NumberUtil;
    using static WsoNativeLib.WsoConfig;

    public partial class FixationSettingWindow
    {
        InternalFixationPreset _fixationParam = new();

        private void ReflectInternalFixationSettings()
        {
            if (!Configuration.ObtainInternalFixationPreset(out InternalFixationPreset param))
            {
                return;
            }

            this._fixationParam = param;
            myTbFixationODFundusRow.Text = param.fundusOD.x.ToString();
            myTbFixationODFundusCol.Text = param.fundusOD.y.ToString();
            myTbFixationOSFundusRow.Text = param.fundusOS.x.ToString();
            myTbFixationOSFundusCol.Text = param.fundusOS.y.ToString();

            myTbFixationODCenterRow.Text = param.centerOD.x.ToString();
            myTbFixationODCenterCol.Text = param.centerOD.y.ToString();
            myTbFixationOSCenterRow.Text = param.centerOS.x.ToString();
            myTbFixationOSCenterCol.Text = param.centerOS.y.ToString();

            myTbFixationODDiscRow.Text = param.scanDiscOD.x.ToString();
            myTbFixationODDiscCol.Text = param.scanDiscOD.y.ToString();
            myTbFixationOSDiscRow.Text = param.scanDiscOS.x.ToString();
            myTbFixationOSDiscCol.Text = param.scanDiscOS.y.ToString();

            myTbFixationBrightness.Text = param.brightness.ToString();
            myTbFixationPeriod.Text = param.blinkPeriod.ToString();
            myTbFixationOnTime.Text = param.blinkOnTime.ToString();
            myCheckFixationBlinkMode.IsChecked = param.useBlinkMode;
            myTbFixationType.Text = param.fixationType.ToString();
        }

        private void UploadConfigSettings()
        {
            UploadInternalFixationSettings();
        }

        private void UploadInternalFixationSettings()
        {
            ref var param = ref this._fixationParam;
            param.fundusOD.x = ToInt(myTbFixationODFundusRow.Text);
            param.fundusOD.y = ToInt(myTbFixationODFundusCol.Text);
            param.fundusOS.x = ToInt(myTbFixationOSFundusRow.Text);
            param.fundusOS.y = ToInt(myTbFixationOSFundusCol.Text);

            param.centerOD.x = ToInt(myTbFixationODCenterRow.Text);
            param.centerOD.y = ToInt(myTbFixationODCenterCol.Text);
            param.centerOS.x = ToInt(myTbFixationOSCenterRow.Text);
            param.centerOS.y = ToInt(myTbFixationOSCenterCol.Text);

            param.scanDiscOD.x = ToInt(myTbFixationODDiscRow.Text);
            param.scanDiscOD.y = ToInt(myTbFixationODDiscCol.Text);
            param.scanDiscOS.x = ToInt(myTbFixationOSDiscRow.Text);
            param.scanDiscOS.y = ToInt(myTbFixationOSDiscCol.Text);

            param.brightness = ToInt(myTbFixationBrightness.Text);
            param.blinkPeriod = ToInt(myTbFixationPeriod.Text);
            param.blinkOnTime = ToInt(myTbFixationOnTime.Text);
            param.useBlinkMode = myCheckFixationBlinkMode.IsChecked ?? false;
            param.fixationType = ToInt(myTbFixationType.Text);

            Configuration.SubmitInternalFixationPreset(param);
        }
    }
}
