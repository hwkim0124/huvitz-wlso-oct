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

    struct DiopterCalibration
    {
        int octFocusZeroPos;
        int lsoFocusZeroPos;
    };

    struct FilterCalibration
    {
        short inPos;
        short outPos;
    };

    struct MotorCalibration
    {
        int referRetinaOriginPos;
        int referCorneaOriginPos;
        int polarOriginPos;
        FilterCalibration returnMirror;
        FilterCalibration octAnteriorLens;
    };

    struct SystemCalibration
    {
        DispersionCalibration dispersionRetina;
        DispersionCalibration dispersionCornea;
        SpectrometerCalibration spectrometer;
        OctGalvanometerCalibration octGalvano;
        SldCalibration sldParam;
        LedCalibration ledParam;
        DiopterCalibration diopterParam;
        MotorCalibration motorParam;
    };
}