#pragma once

#include "WsoSystem2.h"


namespace wso_system
{
	using namespace wso_domain;

	extern "C"
	{
		// Eye Alignment (EyeAlign module)
		///////////////////////////////////////////////////////////////////////////////////////////
		// mode: EyeAlignMode, eyeSide: EyeSide (used by AUTO / MANUAL; FULL forces OD).
		// Returns an EyeAlignResult code: 0 (NONE) on success, otherwise the refusal reason.
		int WSOSYSTEM_DLL_API __stdcall startEyeAlignment(int mode, int eyeSide, EyeAlignEventOccurred clbEvent);
		// Stops every axis and ends the run with ALIGN_CANCELLED(USER_CANCEL). Blocks until
		// the run's threads have finished; do not call it synchronously from the event handler.
		bool WSOSYSTEM_DLL_API __stdcall cancelEyeAlignment(void);
		bool WSOSYSTEM_DLL_API __stdcall holdEyeAlignment(void);
		bool WSOSYSTEM_DLL_API __stdcall resumeEyeAlignment(void);
		bool WSOSYSTEM_DLL_API __stdcall isEyeAlignmentHeld(void);
		bool WSOSYSTEM_DLL_API __stdcall isEyeAligning(void);
		int WSOSYSTEM_DLL_API __stdcall getEyeAlignMode(void);
		int WSOSYSTEM_DLL_API __stdcall getEyeAlignPhase(void);
		int WSOSYSTEM_DLL_API __stdcall getEyeAlignResult(void);
		bool WSOSYSTEM_DLL_API __stdcall fetchEyeAlignStatus(EyeAlignStatus* status);

		// Pupil query (polled by the UI on every preview redraw). camId: IrCameraId.
		bool WSOSYSTEM_DLL_API __stdcall isPupilDetected(int camId);
		bool WSOSYSTEM_DLL_API __stdcall getPupilPosition(int camId, float* cx, float* cy, float* radius, float* score);
		bool WSOSYSTEM_DLL_API __stdcall fetchPupilDetectResult(int camId, PupilDetectResult* result);

		// Runs the detector on an arbitrary 8-bit image (offline harness).
		bool WSOSYSTEM_DLL_API __stdcall detectPupilInImage(const unsigned char* gray, int width, int height, int camId, PupilDetectResult* result);

		// Test hook: substitutes the detector output of camId while set.
		bool WSOSYSTEM_DLL_API __stdcall setPupilDetectOverride(int camId, bool found, float cx, float cy, float radius);
		bool WSOSYSTEM_DLL_API __stdcall clearPupilDetectOverride(int camId);

		// Struct size self-check for the C# mirror.
		int WSOSYSTEM_DLL_API __stdcall getEyeAlignStatusStructSize(void);
		int WSOSYSTEM_DLL_API __stdcall getEyeAlignConfigStructSize(void);
	}
}
