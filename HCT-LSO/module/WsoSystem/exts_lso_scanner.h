#pragma once

#include "WsoSystem2.h"

namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		bool WSOSYSTEM_DLL_API __stdcall obtainLsoScannerControlParam(int patternId, LsoScannerControlParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitLsoScannerControlParam(int patternId, const LsoScannerControlParam* param);
		// Custom pos[] table for one scanner slot; replaces the generated linear sweep on
		// every later submit (Apply and the one-calls). count must match the profile's
		// sampleSize when submitted; count 0 / null clears. Not persisted on the board.
		bool WSOSYSTEM_DLL_API __stdcall setLsoScannerTrajectory(int patternId, const short* pos, int count);
		bool WSOSYSTEM_DLL_API __stdcall obtainLsoScannerCaptureParam(int patternId, LsoScannerCaptureParam* param);
		bool WSOSYSTEM_DLL_API __stdcall submitLsoScannerCaptureParam(int patternId, const LsoScannerCaptureParam* param);

		bool WSOSYSTEM_DLL_API __stdcall moveLsoScannerYposition(int ypos);
		// Scan sequence, board-global (the command carries no pattern id): seqMode 0 =
		// TRIG_FIRST, 1 = GALVANO_FIRST. preMove* are ignored unless GALVANO_FIRST.
		bool WSOSYSTEM_DLL_API __stdcall setLsoScanSequence(int seqMode, int preMoveOffset, int preMoveDelayUs);
		bool WSOSYSTEM_DLL_API __stdcall controlLsoScannerCapture(int patternId, int onOff);
		bool WSOSYSTEM_DLL_API __stdcall setLsoScannerTriggerMode(int onOff);
		bool WSOSYSTEM_DLL_API __stdcall startLsoScannerGrabbing(int patternId);
		bool WSOSYSTEM_DLL_API __stdcall pauseLsoScannerGrabbing(int patternId);
	}
}