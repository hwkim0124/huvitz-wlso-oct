using System;
using System.Collections.Generic;
using System.Text;
using WsoNativeLib;

namespace WsoToolkit
{
    using static utils.NumberUtil;

    public partial class SystemCalibrationWindow
    {
        WsoBoard.SystemCalibration _sysCalib = new();

        private bool FetchSystemCalibration()
        {
            return Calibration.FetchSystemCalibration(out _sysCalib, reload: true);
        }

        private bool WriteSystemCalibration()
        {
            return Calibration.ApplySystemCalibration(ref _sysCalib, write: true);
        }

        private bool UpdateSystemCalibration()
        {
            return Calibration.FetchSystemCalibration(out _sysCalib, reload: false);
        }

        private bool ApplySystemCalibration()
        {
            return Calibration.ApplySystemCalibration(ref _sysCalib, write: false);
        }

        private void CaptureDataFromControls()
        {
            _sysCalib.dispersionRetina.a2 = ToDouble(editRetinaDispComp1.Text);
            _sysCalib.dispersionRetina.a3 = ToDouble(editRetinaDispComp2.Text);
            _sysCalib.dispersionRetina.a4 = ToDouble(editRetinaDispComp3.Text);
            _sysCalib.dispersionCornea.a2 = ToDouble(editCorneaDispComp1.Text);
            _sysCalib.dispersionCornea.a3 = ToDouble(editCorneaDispComp2.Text);
            _sysCalib.dispersionCornea.a4 = ToDouble(editCorneaDispComp3.Text);

            _sysCalib.spectrometer.a0 = ToDouble(editSpectroCalib1.Text);
            _sysCalib.spectrometer.a1 = ToDouble(editSpectroCalib2.Text);
            _sysCalib.spectrometer.a2 = ToDouble(editSpectroCalib3.Text);
            _sysCalib.spectrometer.a3 = ToDouble(editSpectroCalib4.Text);

            _sysCalib.octGalvano.offsetX = ToFloat(editGalvanoOffsetX.Text);
            _sysCalib.octGalvano.offsetY = ToFloat(editGalvanoOffsetY.Text);
            _sysCalib.octGalvano.rangeX = ToFloat(editGalvanoRangeX.Text);
            _sysCalib.octGalvano.rangeY = ToFloat(editGalvanoRangeY.Text);

            _sysCalib.sldParam.pdCurrMax = ToUshort(editPdCurrentMax.Text);
            _sysCalib.sldParam.pdCurrMin = ToUshort(editPdCurrentMin.Text);
            _sysCalib.sldParam.sldCurrMax = ToUshort(editSldCurrentMax.Text);
            _sysCalib.sldParam.sldCurrMin = ToUshort(editSldCurrentMin.Text);
            _sysCalib.sldParam.refEpdMax = ToUshort(editRefEpdMax.Text);
            _sysCalib.sldParam.refEpdMin = ToUshort(editRefEpdMin.Text);
            _sysCalib.sldParam.rmonHighCode = ToUshort(editHighCode.Text);
            _sysCalib.sldParam.rmonLowCode1 = ToUshort(editLowCode1.Text);
            _sysCalib.sldParam.rmonLowCode2 = ToUshort(editLowCode2.Text);
            _sysCalib.sldParam.rmonRsiCode = ToUshort(editRsiCode.Text);

            _sysCalib.diopterParam.octFocusZeroPos = ToInt(editOctFocus.Text);
            _sysCalib.diopterParam.lsoFocusZeroPos = ToInt(editLsoFocus.Text);

            _sysCalib.motorParam.referRetinaOriginPos = ToInt(editReferPosRetina.Text);
            _sysCalib.motorParam.referCorneaOriginPos = ToInt(editReferPosCornea.Text);
            _sysCalib.motorParam.polarOriginPos = ToInt(editPolarPos.Text);
            _sysCalib.motorParam.returnMirror.inPos = ToShort(editRetMirrorInPos.Text);
            _sysCalib.motorParam.returnMirror.outPos = ToShort(editRetMirrorOutPos.Text);
            _sysCalib.motorParam.octAnteriorLens.inPos = ToShort(editAntLensInPos.Text);
            _sysCalib.motorParam.octAnteriorLens.outPos = ToShort(editAntLensOutPos.Text);

            _sysCalib.ledParam.anteriorIrIntensity1 = ToUshort(editLedAntIr1.Text);
            _sysCalib.ledParam.anteriorIrIntensity2 = ToUshort(editLedAntIr2.Text);
            _sysCalib.ledParam.retinaIrIntensity = ToUshort(editLedRetIr.Text);
            _sysCalib.ledParam.whiteIntensity = ToUshort(editLedWhite.Text);
            _sysCalib.ledParam.blueIntensity = ToUshort(editLedBlue.Text);
            _sysCalib.ledParam.greenIntensity = ToUshort(editLedGreen.Text);
            return;
        }

        private void UpdateDataToControls()
        {
            editRetinaDispComp1.Text = _sysCalib.dispersionRetina.a2.ToString("F4");
            editRetinaDispComp2.Text = _sysCalib.dispersionRetina.a3.ToString("F4");
            editRetinaDispComp3.Text = _sysCalib.dispersionRetina.a4.ToString("F4");

            editCorneaDispComp1.Text = _sysCalib.dispersionCornea.a2.ToString("F4");
            editCorneaDispComp2.Text = _sysCalib.dispersionCornea.a3.ToString("F4");
            editCorneaDispComp3.Text = _sysCalib.dispersionCornea.a4.ToString("F4");

            editSpectroCalib1.Text = _sysCalib.spectrometer.a0.ToString("E6");
            editSpectroCalib2.Text = _sysCalib.spectrometer.a1.ToString("E6");
            editSpectroCalib3.Text = _sysCalib.spectrometer.a2.ToString("E6");
            editSpectroCalib4.Text = _sysCalib.spectrometer.a3.ToString("E6");

            editGalvanoOffsetX.Text = _sysCalib.octGalvano.offsetX.ToString("F2");
            editGalvanoOffsetY.Text = _sysCalib.octGalvano.offsetY.ToString("F2");
            editGalvanoRangeX.Text = _sysCalib.octGalvano.rangeX.ToString("F2");
            editGalvanoRangeY.Text = _sysCalib.octGalvano.rangeY.ToString("F2");

            editPdCurrentMax.Text = _sysCalib.sldParam.pdCurrMax.ToString();
            editPdCurrentMin.Text = _sysCalib.sldParam.pdCurrMin.ToString();
            editSldCurrentMax.Text = _sysCalib.sldParam.sldCurrMax.ToString();
            editSldCurrentMin.Text = _sysCalib.sldParam.sldCurrMin.ToString();
            editRefEpdMax.Text = _sysCalib.sldParam.refEpdMax.ToString();
            editRefEpdMin.Text = _sysCalib.sldParam.refEpdMin.ToString();
            editHighCode.Text = _sysCalib.sldParam.rmonHighCode.ToString();
            editLowCode1.Text = _sysCalib.sldParam.rmonLowCode1.ToString();
            editLowCode2.Text = _sysCalib.sldParam.rmonLowCode2.ToString();
            editRsiCode.Text = _sysCalib.sldParam.rmonRsiCode.ToString();

            editOctFocus.Text = _sysCalib.diopterParam.octFocusZeroPos.ToString();
            editLsoFocus.Text = _sysCalib.diopterParam.lsoFocusZeroPos.ToString();

            editReferPosRetina.Text = _sysCalib.motorParam.referRetinaOriginPos.ToString();
            editReferPosCornea.Text = _sysCalib.motorParam.referCorneaOriginPos.ToString();
            editPolarPos.Text = _sysCalib.motorParam.polarOriginPos.ToString();
            editRetMirrorInPos.Text = _sysCalib.motorParam.returnMirror.inPos.ToString();
            editRetMirrorOutPos.Text = _sysCalib.motorParam.returnMirror.outPos.ToString();
            editAntLensInPos.Text = _sysCalib.motorParam.octAnteriorLens.inPos.ToString();
            editAntLensOutPos.Text = _sysCalib.motorParam.octAnteriorLens.outPos.ToString();

            editLedAntIr1.Text = _sysCalib.ledParam.anteriorIrIntensity1.ToString();
            editLedAntIr2.Text = _sysCalib.ledParam.anteriorIrIntensity2.ToString();
            editLedRetIr.Text = _sysCalib.ledParam.retinaIrIntensity.ToString();
            editLedWhite.Text = _sysCalib.ledParam.whiteIntensity.ToString();
            editLedBlue.Text = _sysCalib.ledParam.blueIntensity.ToString();
            editLedGreen.Text = _sysCalib.ledParam.greenIntensity.ToString();
            return;
        }
    }
}
