#include "exts_fixation.h"
#include "Fixation.h"


bool WSOSYSTEM_DLL_API __stdcall wso_system::turnOnInternalFixation(int row, int col)
{
    if (auto* p = Fixation::getInstance(); p) {
		return p->turnOnInternalLed(row, col);
    }
    return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::turnOnInternalFixationWithTarget(EyeSide side, FixationTarget target)
{
    if (auto* p = Fixation::getInstance(); p) {
        return p->turnOnInternalLed(side, target);
    }
    return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::turnOffInternalFixation(void)
{
    if (auto* p = Fixation::getInstance(); p) {
        return p->turnOffInternalLed();
    }
    return false;
}


bool WSOSYSTEM_DLL_API __stdcall wso_system::getCurrentInternalFixation(int* row, int* col)
{
    if (auto* p = Fixation::getInstance(); p) {
        return p->getCurrentInternalPosition(*row, *col);
    }
    return false;
}

bool WSOSYSTEM_DLL_API __stdcall wso_system::setupInternalFixation(InternalFixationParam param)
{
    if (auto* p = Fixation::getInstance(); p) {
        return p->setupInternalFixation(param);
    }
    return false;
}
