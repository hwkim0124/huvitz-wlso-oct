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
            ReflectRetinaCameraSettings();
            ReflectInternalFixationSettings();
            ReflectEyeAlignSettings();
        }

        private void UploadConfigSettings()
        {
            UploadCorneaCameraSettings();
            UploadRetinaCameraSettings();
            UploadInternalFixationSettings();
            UploadEyeAlignSettings();
        }

        private void ReflectEyeAlignSettings()
        {
            if (!Configuration.ObtainEyeAlignConfig(out var p))
            {
                return;
            }

            editAlignReadyZ.Text = p.readyPosZ.ToString();
            editAlignSearchZ.Text = p.searchLimitPosZ.ToString();
            editAlignMinSafeZ.Text = p.minSafePosZ.ToString();
            editAlignEyeXOD.Text = p.eyePosX_OD.ToString();
            editAlignEyeXOS.Text = p.eyePosX_OS.ToString();
            editAlignPosTol.Text = p.posToleranceSteps.ToString();
            editAlignStepZ.Text = p.stepZ.ToString();
            editAlignSwingOD.Text = p.swingTiltPos_OD.ToString();
            editAlignSwingOS.Text = p.swingTiltPos_OS.ToString();
            checkAlignBackOff.IsChecked = p.backOffOnEnd != 0;
            editAlignIrLed.Text = p.irLedIntensity.ToString();

            editAlignTolY.Text = p.toleranceYPx.ToString("F1");
            editAlignTolZApp.Text = p.approachTolZPx.ToString("F1");
            editAlignTolX.Text = p.toleranceXPx.ToString("F1");
            editAlignTolZ.Text = p.toleranceZPx.ToString("F1");
            editAlignStartDb.Text = p.startDeadbandPx.ToString("F1");
            editAlignStepsPxX.Text = p.stepsPerPxX.ToString("F2");
            editAlignStepsPxY.Text = p.stepsPerPxY.ToString("F2");
            editAlignStepsPxZ.Text = p.stepsPerPxZ.ToString("F2");

            editAlignDetectN.Text = p.detectConsecutive.ToString();
            editAlignSettleN.Text = p.settleFrames.ToString();
            editAlignLostStop.Text = p.lostFramesToStop.ToString();
            editAlignLostCancel.Text = p.lostFramesToCancel.ToString();
            editAlignMaxJump.Text = p.maxJumpPx.ToString("F1");
            editAlignSmooth.Text = p.smoothingAlpha.ToString("F2");

            editAlignToReady.Text = p.readyTimeoutMs.ToString();
            editAlignToSwing.Text = p.swingTimeoutMs.ToString();
            editAlignToEyeX.Text = p.eyeXTimeoutMs.ToString();
            editAlignToSearch.Text = p.searchTimeoutMs.ToString();
            editAlignToAlign.Text = p.alignTimeoutMs.ToString();

            ReflectPupilTarget(p.detLeft, editAlignLeftTX, editAlignLeftTY, editAlignLeftSH, editAlignLeftSV);
            ReflectPupilTarget(p.detRight, editAlignRightTX, editAlignRightTY, editAlignRightSH, editAlignRightSV);
            ReflectPupilTarget(p.detLower, editAlignLowerTX, editAlignLowerTY, editAlignLowerSH, editAlignLowerSV);
        }

        private static void ReflectPupilTarget(WsoEyeAlign.PupilDetectParam det,
            System.Windows.Controls.TextBox tx, System.Windows.Controls.TextBox ty,
            System.Windows.Controls.TextBox sh, System.Windows.Controls.TextBox sv)
        {
            tx.Text = det.targetX.ToString();
            ty.Text = det.targetY.ToString();
            sh.Text = det.signH.ToString();
            sv.Text = det.signV.ToString();
        }

        private static WsoEyeAlign.PupilDetectParam UploadPupilTarget(
            System.Windows.Controls.TextBox tx, System.Windows.Controls.TextBox ty,
            System.Windows.Controls.TextBox sh, System.Windows.Controls.TextBox sv)
        {
            var det = new WsoEyeAlign.PupilDetectParam();
            det.targetX = ToInt(tx.Text);
            det.targetY = ToInt(ty.Text);
            det.signH = ToInt(sh.Text) < 0 ? -1 : +1;
            det.signV = ToInt(sv.Text) < 0 ? -1 : +1;
            return det;
        }

        private void UploadEyeAlignSettings()
        {
            var p = new WsoEyeAlign.EyeAlignConfigParam();
            p.readyPosZ = ToInt(editAlignReadyZ.Text);
            p.searchLimitPosZ = ToInt(editAlignSearchZ.Text);
            p.minSafePosZ = ToInt(editAlignMinSafeZ.Text);
            p.eyePosX_OD = ToInt(editAlignEyeXOD.Text);
            p.eyePosX_OS = ToInt(editAlignEyeXOS.Text);
            p.posToleranceSteps = ToInt(editAlignPosTol.Text);
            p.stepZ = ToInt(editAlignStepZ.Text);
            p.swingTiltPos_OD = ToInt(editAlignSwingOD.Text);
            p.swingTiltPos_OS = ToInt(editAlignSwingOS.Text);
            p.backOffOnEnd = (checkAlignBackOff.IsChecked ?? false) ? 1 : 0;
            p.irLedIntensity = ToInt(editAlignIrLed.Text);

            p.toleranceYPx = ToFloat(editAlignTolY.Text);
            p.approachTolZPx = ToFloat(editAlignTolZApp.Text);
            p.toleranceXPx = ToFloat(editAlignTolX.Text);
            p.toleranceZPx = ToFloat(editAlignTolZ.Text);
            p.startDeadbandPx = ToFloat(editAlignStartDb.Text);
            p.stepsPerPxX = ToFloat(editAlignStepsPxX.Text);
            p.stepsPerPxY = ToFloat(editAlignStepsPxY.Text);
            p.stepsPerPxZ = ToFloat(editAlignStepsPxZ.Text);

            p.detectConsecutive = ToInt(editAlignDetectN.Text);
            p.settleFrames = ToInt(editAlignSettleN.Text);
            p.lostFramesToStop = ToInt(editAlignLostStop.Text);
            p.lostFramesToCancel = ToInt(editAlignLostCancel.Text);
            p.maxJumpPx = ToFloat(editAlignMaxJump.Text);
            p.smoothingAlpha = ToFloat(editAlignSmooth.Text);

            p.readyTimeoutMs = ToInt(editAlignToReady.Text);
            p.swingTimeoutMs = ToInt(editAlignToSwing.Text);
            p.eyeXTimeoutMs = ToInt(editAlignToEyeX.Text);
            p.searchTimeoutMs = ToInt(editAlignToSearch.Text);
            p.alignTimeoutMs = ToInt(editAlignToAlign.Text);

            p.detLeft = UploadPupilTarget(editAlignLeftTX, editAlignLeftTY, editAlignLeftSH, editAlignLeftSV);
            p.detRight = UploadPupilTarget(editAlignRightTX, editAlignRightTY, editAlignRightSH, editAlignRightSV);
            p.detLower = UploadPupilTarget(editAlignLowerTX, editAlignLowerTY, editAlignLowerSH, editAlignLowerSV);

            Configuration.SubmitEyeAlignConfig(in p);
        }

        private void ReflectCorneaCameraSettings()
        {
            if (Configuration.ObtainCorneaCameraConfig(out var param))
            {
                editCorneaAgain1.Text = param.again_left.ToString("F0");
                editCorneaDgain1.Text = param.dgain_left.ToString("F0");
                editCorneaAgain2.Text = param.again_right.ToString("F0");
                editCorneaDgain2.Text = param.dgain_right.ToString("F0");
                editCorneaAgain3.Text = param.again_lower.ToString("F0");
                editCorneaDgain3.Text = param.dgain_lower.ToString("F0");
                editCorneaExpos1.Text = param.exposTime_left.ToString("F0");
                editCorneaExpos2.Text = param.exposTime_right.ToString("F0");
                editCorneaExpos3.Text = param.exposTime_lower.ToString("F0");
            }
        }

        private void ReflectRetinaCameraSettings()
        {
            if (Configuration.ObtainRetinaCameraConfig(out var param))
            {
                editRetinaAgain.Text = param.again.ToString("F0");
                editRetinaDgain.Text = param.dgain.ToString("F0");
                editRetinaExpos.Text = param.exposTime.ToString("F0");
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
            param.exposTime_left = ToFloat(editCorneaExpos1.Text);
            param.exposTime_right = ToFloat(editCorneaExpos2.Text);
            param.exposTime_lower = ToFloat(editCorneaExpos3.Text);

            Configuration.SubmitCorneaCameraConfig(param);
        }

        private void UploadRetinaCameraSettings()
        {
            var param = new WsoConfig.RetinaCameraConfigParam();
            param.again = ToFloat(editRetinaAgain.Text);
            param.dgain = ToFloat(editRetinaDgain.Text);
            param.exposTime = ToFloat(editRetinaExpos.Text);
            Configuration.SubmitRetinaCameraConfig(param);
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
