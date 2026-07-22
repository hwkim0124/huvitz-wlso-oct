#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// Step Motors 
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall fetchStepMotorStatus(MotorType type, StepMotorStatus* status);
		void WSOSYSTEM_DLL_API __stdcall connectStepMotorPositionChanged(MotorType type, StepMotorPositionChanged clb);
		void WSOSYSTEM_DLL_API __stdcall releaseStepMotorPositionChanged(MotorType type);

		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorPosition(MotorType type, int pos);
		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorByPositionOffset(MotorType type, int offset);
		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorByValue(MotorType type, float value);
		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorByValueOffset(MotorType type, float offset);
		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorToOrigin(MotorType type, int mode);

		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorToUpperEnd(MotorType type);
		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorToLowerEnd(MotorType type);
		bool WSOSYSTEM_DLL_API __stdcall moveStepMotorToCenter(MotorType type);
		bool WSOSYSTEM_DLL_API __stdcall isStepMotorAtUpperEnd(MotorType type);
		bool WSOSYSTEM_DLL_API __stdcall isStepMotorAtLowerEnd(MotorType type);
		bool WSOSYSTEM_DLL_API __stdcall isStepMotorAtCenter(MotorType type);
		void WSOSYSTEM_DLL_API __stdcall stopStepMotor(MotorType type);

		bool WSOSYSTEM_DLL_API __stdcall setStepMotorCurrentPositionAsOrigin(MotorType type, int mode);

		int WSOSYSTEM_DLL_API __stdcall getStepMotorPosition(MotorType type);
		int WSOSYSTEM_DLL_API __stdcall getStepMotorPositionAtValue(MotorType type, float value);
		float WSOSYSTEM_DLL_API __stdcall getStepMotorValue(MotorType type);
		float WSOSYSTEM_DLL_API __stdcall getStepMotorValueAtPosition(MotorType type, int pos);

		bool WSOSYSTEM_DLL_API __stdcall moveStageLeft(void);
		bool WSOSYSTEM_DLL_API __stdcall moveStageRight(void);
		bool WSOSYSTEM_DLL_API __stdcall moveStageUp(void);
		bool WSOSYSTEM_DLL_API __stdcall moveStageDown(void);
		bool WSOSYSTEM_DLL_API __stdcall moveStageForward(void);
		bool WSOSYSTEM_DLL_API __stdcall moveStageBackward(void);
		bool WSOSYSTEM_DLL_API __stdcall stopStageX(void);
		bool WSOSYSTEM_DLL_API __stdcall stopStageY(void);
		bool WSOSYSTEM_DLL_API __stdcall stopStageZ(void);
		bool WSOSYSTEM_DLL_API __stdcall stopStageAll(void);

		// PI Sensor Vaule
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall isOctFocusMotorAtOrigin(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctPolarMotorAtOrigin(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctReferMotorAtOrigin(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctRefNdMotorAtOrigin(void);
		bool WSOSYSTEM_DLL_API __stdcall isLsoFocusMotorAtOrigin(void);
		bool WSOSYSTEM_DLL_API __stdcall isRetMirrorMotorAtOrigin(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctAntLensMotorAtOrigin(void);
		bool WSOSYSTEM_DLL_API __stdcall isLsoFilterMotorAtOrigin(void);

		bool WSOSYSTEM_DLL_API __stdcall isSwingMotorAtHighLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isSwingMotorAtLowLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isXstageMotorAtHighLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isXstageMotorAtLowLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isYstageMotorAtHighLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isYstageMotorAtLowLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isZstageMotorAtHighLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isZstageMotorAtLowLimit(void);

	}

}
