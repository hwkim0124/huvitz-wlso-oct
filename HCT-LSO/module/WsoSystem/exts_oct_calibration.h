#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		int WSOSYSTEM_DLL_API _stdcall getOctFbgClassListSize(void);
		bool WSOSYSTEM_DLL_API _stdcall getOctFbgClassName(int index, LPTSTR name);
		int WSOSYSTEM_DLL_API _stdcall getOctFbgClassValues(int index, double* values, int buff_size);

		int WSOSYSTEM_DLL_API _stdcall findOctSpectrumDataPeaks(unsigned short* data, int data_size, 
					int threshold, int threshold2, int* values, int* indices);
		int WSOSYSTEM_DLL_API _stdcall computeOctSpectrometerCoefficients(int* indice, double* values, int data_size,
					int poly_order, double* coeffs);
		bool WSOSYSTEM_DLL_API _stdcall applyOctSpectrometerCoefficients(double* values, int data_size);
	}
}