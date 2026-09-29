#include "pch.h"
#include "exts_eye_align.h"
#include "EyeAlignment.h"


using namespace wso_system;


namespace
{
	bool isValidCamera(int camId)
	{
		return camId >= 0 && camId < static_cast<int>(IrCameraId::NUMBER_OF_ITEMS);
	}
}


int WSOSYSTEM_DLL_API __stdcall wso_system::startEyeAlignment(int mode, int eyeSide, EyeAlignEventOccurred clbEvent)
{
	try {
		if (mode < static_cast<int>(EyeAlignMode::MANUAL) || mode > static_cast<int>(EyeAlignMode::FULL)) {
			return static_cast<int>(EyeAlignResult::INTERNAL_ERROR);
		}
		auto result = EyeAlignment::getInstance()->startAlignment(
			static_cast<EyeAlignMode>(mode), static_cast<EyeSide>(eyeSide), clbEvent);
		return static_cast<int>(result);
	}
	catch (...) {
		return static_cast<int>(EyeAlignResult::INTERNAL_ERROR);
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::cancelEyeAlignment(void)
{
	try {
		return EyeAlignment::getInstance()->cancelAlignment();
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::holdEyeAlignment(void)
{
	try {
		return EyeAlignment::getInstance()->holdAlignment();
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::resumeEyeAlignment(void)
{
	try {
		return EyeAlignment::getInstance()->resumeAlignment();
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isEyeAlignmentHeld(void)
{
	try {
		return EyeAlignment::getInstance()->isHeld();
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isEyeAligning(void)
{
	try {
		return EyeAlignment::getInstance()->isAligning();
	}
	catch (...) {
		return false;
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getEyeAlignMode(void)
{
	try {
		return static_cast<int>(EyeAlignment::getInstance()->getMode());
	}
	catch (...) {
		return static_cast<int>(EyeAlignMode::MANUAL);
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getEyeAlignPhase(void)
{
	try {
		return static_cast<int>(EyeAlignment::getInstance()->getPhase());
	}
	catch (...) {
		return static_cast<int>(EyeAlignPhase::IDLE);
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getEyeAlignResult(void)
{
	try {
		return static_cast<int>(EyeAlignment::getInstance()->getResult());
	}
	catch (...) {
		return static_cast<int>(EyeAlignResult::INTERNAL_ERROR);
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::fetchEyeAlignStatus(EyeAlignStatus* status)
{
	try {
		if (status == nullptr) {
			return false;
		}
		return EyeAlignment::getInstance()->fetchStatus(status);
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::isPupilDetected(int camId)
{
	try {
		if (!isValidCamera(camId)) {
			return false;
		}
		return EyeAlignment::getInstance()->isPupilDetected(static_cast<IrCameraId>(camId));
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::getPupilPosition(int camId, float* cx, float* cy, float* radius, float* score)
{
	try {
		if (!isValidCamera(camId)) {
			return false;
		}
		PupilDetectResult result;
		if (!EyeAlignment::getInstance()->getPupilResult(static_cast<IrCameraId>(camId), &result) || !result.found) {
			return false;
		}
		if (cx) *cx = result.cx;
		if (cy) *cy = result.cy;
		if (radius) *radius = result.radius;
		if (score) *score = result.score;
		return true;
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::fetchPupilDetectResult(int camId, PupilDetectResult* result)
{
	try {
		if (!isValidCamera(camId) || result == nullptr) {
			return false;
		}
		return EyeAlignment::getInstance()->getPupilResult(static_cast<IrCameraId>(camId), result);
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::detectPupilInImage(const unsigned char* gray, int width, int height, int camId, PupilDetectResult* result)
{
	try {
		if (!isValidCamera(camId) || result == nullptr || gray == nullptr) {
			return false;
		}
		return EyeAlignment::getInstance()->detectPupilInImage(gray, width, height, static_cast<IrCameraId>(camId), result);
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setPupilDetectOverride(int camId, bool found, float cx, float cy, float radius)
{
	try {
		if (!isValidCamera(camId)) {
			return false;
		}
		PupilDetectResult result;
		result.camId = camId;
		result.found = found ? 1 : 0;
		result.cx = cx;
		result.cy = cy;
		result.radius = radius;
		result.score = found ? 1.0f : 0.0f;
		return EyeAlignment::getInstance()->setPupilOverride(static_cast<IrCameraId>(camId), result);
	}
	catch (...) {
		return false;
	}
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::clearPupilDetectOverride(int camId)
{
	try {
		if (!isValidCamera(camId)) {
			return false;
		}
		return EyeAlignment::getInstance()->clearPupilOverride(static_cast<IrCameraId>(camId));
	}
	catch (...) {
		return false;
	}
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getEyeAlignStatusStructSize(void)
{
	return static_cast<int>(sizeof(EyeAlignStatus));
}

int WSOSYSTEM_DLL_API __stdcall wso_system::getEyeAlignConfigStructSize(void)
{
	return static_cast<int>(sizeof(EyeAlignConfigParam));
}
