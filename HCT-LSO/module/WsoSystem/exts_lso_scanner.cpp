#include "pch.h"
#include "exts_lso_scanner.h"
#include "Hardware.h"

using namespace wso_system;
using namespace std;


bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainLsoScannerControlParam(int patternId, LsoScannerControlParam* param)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->obtainControlParameters(patternId, param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::submitLsoScannerControlParam(int patternId, const LsoScannerControlParam* param)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->submitControlParameters(patternId, param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::obtainLsoScannerCaptureParam(int patternId, LsoScannerCaptureParam* param)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->obtainCaptureParameters(patternId, param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::submitLsoScannerCaptureParam(int patternId, const LsoScannerCaptureParam* param)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->submitCaptureParameters(patternId, param);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::moveLsoScannerYposition(int ypos)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->controlYGalvoMove(ypos);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::controlLsoScannerCapture(int patternId, int onOff)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->controlCapture(patternId, onOff);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setLsoScannerTriggerMode(int onOff)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->controlTriggerMode(onOff);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::startLsoScannerGrabbing(int patternId)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->startGrabbing(patternId);
	}
	return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::pauseLsoScannerGrabbing(int patternId)
{
	if (auto* scan = Hardware::getInstance()->getMainBoard()->getLsoScanner(); scan) {
		return scan->pauseGrabbing(patternId);
	}
	return false;
}
