#pragma once

namespace wso_domain
{
    struct DispersionCalibration
    {
        double a2;
        double a3;
        double a4;
    };

    struct SpectrometerCalibration
    {
        double a0;
        double a1;
        double a2;
        double a3;
    };

    struct OctGalvanometerCalibration
    {
        float offsetX;
        float offsetY;
        float rangeX;
        float rangeY;
    };

    struct SldCalibration
    {
        unsigned short pdCurrMax;
        unsigned short pdCurrMin;
        unsigned short sldCurrMax;
        unsigned short sldCurrMin;
        unsigned short refEpdMax;
        unsigned short refEpdMin;

        unsigned short rmonHighCode;
        unsigned short rmonLowCode1;
        unsigned short rmonLowCode2;
        unsigned short rmonRsiCode;
    };

    struct LedCalibration
    {
        unsigned short whiteIntensity;
        unsigned short retinaIrIntensity;
        unsigned short anteriorIrIntensity1;
        unsigned short anteriorIrIntensity2;
        unsigned short blueIntensity;
        unsigned short greenIntensity;
    };

    struct FilterCalibration
    {
        int inPos;
        int outPos;
    };

    struct LsoFilterCalibration
    {
        int colorModePos;
        int blueFafPos;
        int greenFafPos;
        int icgaPos;
        int reserved;
    };

    struct MotorCalibration
    {
        int referRetinaOriginPos;
        int referCorneaOriginPos;
        int polarOriginPos;
        int lsoFocusZeroDioptPos;
        int octFocusZeroDioptPos;
        FilterCalibration returnMirror;
        FilterCalibration octAnteriorLens;
        LsoFilterCalibration lsoFilter;
    };

    struct SystemCalibration
    {
        DispersionCalibration dispersionRetina;
        DispersionCalibration dispersionCornea;
        SpectrometerCalibration spectrometer;
        OctGalvanometerCalibration octGalvano;
        SldCalibration sldParam;
        LedCalibration ledParam;
        MotorCalibration motorParam;
    };
}