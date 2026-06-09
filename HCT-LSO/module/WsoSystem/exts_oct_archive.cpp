#include "pch.h"
#include "exts_oct_archive.h"
#include "OctArchive.h"


bool WSOSYSTEM_DLL_API _stdcall wso_system::isOctProtocolResultValid(void)
{
	if (auto p = OctArchive::getInstance(); p) {
		return p->isProtocolResultValid();
	}
	return false;
}

void WSOSYSTEM_DLL_API _stdcall wso_system::clearOctProtocolResult(void)
{
	if (auto p = OctArchive::getInstance(); p) {
		p->clearProtocolResult();
	}
	return;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctResultPatternSectionCount(void)
{
	if (auto p = OctArchive::getInstance(); p) {
		return p->getMeasureSectionCount();
	}
	return 0;
}

int WSOSYSTEM_DLL_API _stdcall wso_system::getOctResultPreviewSectionCount(void)
{
	if (auto p = OctArchive::getInstance(); p) {
		return p->getPreviewSectionCount();
	}
	return 0;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctResultPatternImage(int section_idx, int image_idx, OctScanImageDescript* desc)
{
	if (auto p = OctArchive::getInstance(); p) {
		return p->getMeasureSectionImage(section_idx, image_idx, *desc);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctResultPreviewImage(int section_idx, int image_idx, OctScanImageDescript* desc)
{
	if (auto p = OctArchive::getInstance(); p) {
		return p->getPreviewSectionImage(section_idx, image_idx, *desc);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctResultEnfaceImage(OctEnfaceImageDescript* desc)
{
	if (auto p = OctArchive::getInstance(); p) {
		return p->getMeasureEnfaceImage(*desc);
	}
	return false;
}

bool WSOSYSTEM_DLL_API _stdcall wso_system::getOctResultCorneaImage(OctCorneaImageDescript* desc)
{
	if (auto p = OctArchive::getInstance(); p) {
		return p->getCorneaCameraImage(*desc);
	}
	return false;
}


