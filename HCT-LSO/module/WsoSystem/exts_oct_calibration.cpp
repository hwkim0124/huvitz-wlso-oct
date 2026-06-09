#include "pch.h"
#include "exts_oct_calibration.h"
#include "SpectroCalibrator.h"


int WSOSYSTEM_DLL_API _stdcall wso_system::getOctFbgClassListSize(void)
{
	int size = NUMBER_OF_FBG_CLASSES;
	return size;
}


bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctFbgClassName(int index, LPTSTR name)
{
	SpectroCalibrator calib;
	auto wstr = atow(string(calib.getFbgClassName(index)));
	wcsncpy_s((wchar_t*)name, 256, wstr.c_str(), wstr.length());
	return true;
}


int WSOSYSTEM_DLL_API _stdcall wso_system::getOctFbgClassValues(int index, double* values, int buff_size)
{
	SpectroCalibrator calib;
	auto data = calib.getFbgClass(index);
	auto size = (int)data.size();

	if (size <= buff_size && values)
	{
		for (int i = 0; i < size; i++)
		{
			values[i] = data[i];
		};
		return size;
	}
	return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::findOctSpectrumDataPeaks(unsigned short* data, int data_size, int threshold1, int threshold2, int* values, int* indice)
{
	if (data_size <= 0 || !data || !values || !indice)
	{
		return 0;
	}

	SpectroCalibrator calib;
	calib.setThresholdOfPeak(threshold1);
	calib.setThresholdOfSubs(threshold2);
	calib.findPeaksOnSpectrum(data, data_size);

	int found = calib.getNumberOfPeaks();
	auto v1 = calib.getPeakValues();
	auto v2 = calib.getPeakIndexs();

	if (found == v1.size() && found == v2.size()) {
		for (int i = 0; i < v1.size(); i++)
		{
			values[i] = v1[i];
			indice[i] = v2[i];
		}
		return found;
	}
	return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::computeOctSpectrometerCoefficients(int* indice, double* values, int data_size, int poly_order, double* coeffs)
{
	if (data_size <= 0 || !indice || !values || !coeffs)
	{
		return 0;
	}

	SpectroCalibrator calib;
	auto& pis = calib.getPeakIndexs();
	auto& wls = calib.getWavelengths();

	pis.clear();
	wls.clear();
	for (int i = 0; i < data_size; i++)
	{
		pis.push_back(indice[i]);
		wls.push_back(values[i]);
	}

	if (calib.calculateParameters(poly_order))
	{
		auto c = calib.getParameters();
		for (int i = 0; i < c.size(); i++)
		{
			coeffs[i] = c[i];
		}
		return (int)c.size();
	}
	return 0;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::applyOctSpectrometerCoefficients(double* values, int data_size)
{
	if (data_size != WAVELENGTH_FUNCTION_COEFFS_SIZE || !values)
	{
		return false;
	}
	ChainSetup::setSpectrometerParameters(values);
	return true;
}
