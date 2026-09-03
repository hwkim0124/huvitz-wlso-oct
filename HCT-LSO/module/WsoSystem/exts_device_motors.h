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


		// Stage Motors 
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall moveHeadLeft(void);
		bool WSOSYSTEM_DLL_API __stdcall moveHeadRight(void);
		bool WSOSYSTEM_DLL_API __stdcall moveHeadUp(void);
		bool WSOSYSTEM_DLL_API __stdcall moveHeadDown(void);
		bool WSOSYSTEM_DLL_API __stdcall moveHeadForward(void);
		bool WSOSYSTEM_DLL_API __stdcall moveHeadBackward(void);
		bool WSOSYSTEM_DLL_API __stdcall stopHeadX(void);
		bool WSOSYSTEM_DLL_API __stdcall stopHeadY(void);
		bool WSOSYSTEM_DLL_API __stdcall stopHeadZ(void);
		bool WSOSYSTEM_DLL_API __stdcall stopHeadXYZ(void);

		bool WSOSYSTEM_DLL_API __stdcall isHeadAtLeftEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isHeadAtRightEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isHeadAtLowerEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isHeadAtUpperEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isHeadAtForwardEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isHeadAtBackwardEnd(void);

		bool WSOSYSTEM_DLL_API __stdcall moveSwingLeft(void);
		bool WSOSYSTEM_DLL_API __stdcall moveSwingRight(void);
		bool WSOSYSTEM_DLL_API __stdcall stopSwingMove(void);

		bool WSOSYSTEM_DLL_API __stdcall isSwingAtLeftEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isSwingAtRightEnd(void);
		
		// Chinrest
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall moveChinrestUp(void);
		bool WSOSYSTEM_DLL_API __stdcall moveChinrestDown(void);
		bool WSOSYSTEM_DLL_API __stdcall stopChinrest(void);
		bool WSOSYSTEM_DLL_API __stdcall isChinrestAtUpperEnd(void);
		bool WSOSYSTEM_DLL_API __stdcall isChinrestAtLowerEnd(void);

		// PI Sensor Vaule
		///////////////////////////////////////////////////////////////////////////////////////////
		bool WSOSYSTEM_DLL_API __stdcall isOctRefNdMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctAntLensMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isRetMirrorMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isLsoFilterMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isLsoFocusMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctPolarMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctReferMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isOctFocusMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isSwingMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isZstageMotorAtPiPosition(void);
		bool WSOSYSTEM_DLL_API __stdcall isXstageMotorAtPiPosition(void);

		bool WSOSYSTEM_DLL_API __stdcall isYstageMotorAtUpperLimit(void);
		bool WSOSYSTEM_DLL_API __stdcall isYstageMotorAtLowerLimit(void);
	}

}
