using System;
using System.Collections.Generic;
using System.Text;
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.NumberUtil;

    public partial class SystemConfigurationWindow
    {
        WsoConfig.InternalFixationPreset _fixationParam = new();
        WsoConfig.CorneaCameraConfigParam _corneaCamParam = new();

        // Fetch config parameters from WSO library.
        ///////////////////////////////////////////////////////////////////////////////////////////////////////
        private void ReflectConfigSettings()
        {
            ReflectCorneaCameraSettings();
            ReflectInternalFixationSettings();
        }

        private void UploadConfigSettings()
        {
            UploadCorneaCameraSettings();
            UploadInternalFixationSettings();
        }

        private void ReflectCorneaCameraSettings()
        {
            if (Configuration.ObtainCorneaCameraConfig(out var param))
            {
                editCorneaAgain1.Text = param.again_left.ToString("F1");
                editCorneaDgain1.Text = param.dgain_left.ToString("F1");
                editCorneaAgain2.Text = param.again_right.ToString("F1");
                editCorneaDgain2.Text = param.dgain_right.ToString("F1");
                editCorneaAgain3.Text = param.again_lower.ToString("F1");
                editCorneaDgain3.Text = param.dgain_lower.ToString("F1");
            }
        }

        private void ReflectInternalFixationSettings()
        {
            WsoConfig.InternalFixationPreset param = new WsoConfig.InternalFixationPreset();
            if (!Configuration.ObtainInternalFixationPreset(out param))
            {
                return;
            }

            this._fixationParam = param;
            FundusOdXEdit.Text = param.fundusOD.x.ToString();
            FundusOdYEdit.Text = param.fundusOD.y.ToString();
            FundusOsXEdit.Text = param.fundusOS.x.ToString();
            FundusOsYEdit.Text = param.fundusOS.y.ToString();

            FoveaOdXEdit.Text = param.centerOD.x.ToString();
            FoveaOdYEdit.Text = param.centerOD.y.ToString();
            FoveaOsXEdit.Text = param.centerOS.x.ToString();
            FoveaOsYEdit.Text = param.centerOS.y.ToString();

            DiscOdXEdit.Text = param.scanDiscOD.x.ToString();
            DiscOdYEdit.Text = param.scanDiscOD.y.ToString();
            DiscOsXEdit.Text = param.scanDiscOS.x.ToString();
            DiscOsYEdit.Text = param.scanDiscOS.y.ToString();

            FixBrightEdit.Text = param.brightness.ToString();
            BlinkPeriodEdit.Text = param.blinkPeriod.ToString();
            BlinkOnTimeEdit.Text = param.blinkOnTime.ToString();
            BlinkCheckBox.IsChecked = param.useBlinkMode;
            FixationTypeEdit.Text = param.fixationType.ToString();
        }

        private void UploadCorneaCameraSettings()
        {
            var param = new WsoConfig.CorneaCameraConfigParam();
            param.again_left = ToFloat(editCorneaAgain1.Text);
            param.dgain_left = ToFloat(editCorneaDgain1.Text);
            param.again_right = ToFloat(editCorneaAgain2.Text);
            param.dgain_right = ToFloat(editCorneaDgain2.Text);
            param.again_lower = ToFloat(editCorneaAgain3.Text);
            param.dgain_lower = ToFloat(editCorneaDgain3.Text);

            Configuration.SubmitCorneaCameraConfig(param);
        }

        private void UploadInternalFixationSettings()
        {
            ref var param = ref this._fixationParam;
            param.fundusOD.x = ToInt(FundusOdXEdit.Text);
            param.fundusOD.y = ToInt(FundusOdYEdit.Text);
            param.fundusOS.x = ToInt(FundusOsXEdit.Text);
            param.fundusOS.y = ToInt(FundusOsYEdit.Text);

            param.centerOD.x = ToInt(FoveaOdXEdit.Text);
            param.centerOD.y = ToInt(FoveaOdYEdit.Text);
            param.centerOS.x = ToInt(FoveaOsXEdit.Text);
            param.centerOS.y = ToInt(FoveaOsYEdit.Text);

            param.scanDiscOD.x = ToInt(DiscOdXEdit.Text);
            param.scanDiscOS.y = ToInt(DiscOdYEdit.Text);
            param.scanDiscOS.x = ToInt(DiscOsXEdit.Text);
            param.scanDiscOS.y = ToInt(DiscOsYEdit.Text);

            param.brightness = ToInt(FixBrightEdit.Text);
            param.blinkPeriod = ToInt(BlinkPeriodEdit.Text);
            param.blinkOnTime = ToInt(BlinkOnTimeEdit.Text);
            param.useBlinkMode = BlinkCheckBox.IsChecked ?? false;
            param.fixationType = ToInt(FixationTypeEdit.Text);

            Configuration.SubmitInternalFixationPreset(param);
        }
    }
}
