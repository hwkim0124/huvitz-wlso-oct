#include "pch.h"
#include "LsoScanner.h"
#include "MainBoard.h"
#include "UsbComm.h"

using namespace wso_device;
using namespace wso_board;

struct LsoScanner::LsoScannerImpl
{
	MainBoard* board;
	bool initiated = false;
	bool turnedOn = false;

	unsigned int ctrlStatus = 0;
	unsigned int acqCnt = 0;
	lso_scan_profile_st scanProfile[8] = {};

	LsoScannerImpl() : board(nullptr), initiated(false) {
	}
};

LsoScanner::LsoScanner() :
	d_ptr(make_unique<LsoScannerImpl>())
{
}

wso_device::LsoScanner::LsoScanner(MainBoard* board) :
	d_ptr(make_unique<LsoScannerImpl>())
{
	impl().board = board;

	return;
}

LsoScanner::~LsoScanner()
{
}

// Copy constructor and assignment by const reference are not allowed here 
// becuase the innert implementation struct has unique_ptr member for exclusive-ownership.
// It needs additional defintion of copy constructor and assignment for the struct to resolve 
// the situation. 
wso_device::LsoScanner::LsoScanner(LsoScanner&& rhs) = default;
LsoScanner& wso_device::LsoScanner::operator=(LsoScanner&& rhs) = default;

bool wso_device::LsoScanner::initializeLsoScanner(void)
{
	/*
	if (getMainBoard()->isNoLeds()) {
	return true;
	}
	*/
	if (impl().board == nullptr) {
		return false;
	}

	updateScannerStatus();

	
	// controlSwitch(0);
	//loadConfigFromIniFile();

	//if (!storeControlParameters())
	//{
	//	LogDebug() << "Lso scanner control parameters failed to change!";
	//	return false;
	//}
	
	impl().initiated = true;
	return true;
}


bool wso_device::LsoScanner::updateScannerStatus(void)
{
	UsbComm& usbComm = getMainBoard()->getUsbComm();

	auto* hbs = getMainBoard()->getHbsDataProfile();
	auto* scanner = hbs->getHbsLsoScanner();
	auto* desc = hbs->getHbsTableDescriptor();
	if (!usbComm.readLsoScannerParam(scanner, desc)) {
		LogDebug() << "Reading slo scanner status failed!";
		return false;
	}

	auto& capture_status = scanner->lso_capture_status;
	auto& scan_traj_profile = scanner->traj_profile;

	impl().ctrlStatus = capture_status.ctrl_status;
	impl().acqCnt = capture_status.acq_cnt;

	impl().scanProfile[0] = scan_traj_profile[0];
	impl().scanProfile[1] = scan_traj_profile[1];
	return true;
}


bool wso_device::LsoScanner::obtainControlParameters(int patternId, LsoScannerControlParam* param)
{
	if (patternId < 0 || patternId >= LSO_SCANNER_PATTERN_NUM_MAX || param == nullptr) {
		return false;
	}

	if (auto* data = impl().board->getHbsDataProfile(); data) {
		if (data->loadLsoScannerParam()) {
			if (auto* scan = data->getHbsLsoScanner(); scan) {
				auto& traj = scan->traj_profile[patternId];
				param->timeStepUs = traj.time_step_us;
				param->exposureTimeUs = traj.exposure_time_us;
				param->triggerSource = traj.TrgSrc;
				param->acquisitionMode = traj.AcqMode;
				param->acqFrameSize = traj.AcqFrameSize;
				param->sampleSize = traj.sample_size;
				param->subFrameSize = traj.subframe_size;
				param->prescanPosRewindOffset = traj.prescan_pos_rewind_offset;
				param->ledOffPosIndex = traj.led_off_pos_index;
				param->ledOnPosIndex = traj.led_on_pos_index;

				param->yGalvoStartPos = 0;
				param->yGalvoEndPos = 0;

				auto size = param->sampleSize;
				if (size > 0 && size <= LSO_SCANNER_SAMPLE_SIZE_MAX) {
					param->yGalvoStartPos = traj.pos[0];
					param->yGalvoEndPos = traj.pos[size - 1];
				}
				return true;
			}
		}
	}
	return false;
}

bool wso_device::LsoScanner::submitControlParameters(int patternId, const LsoScannerControlParam* param)
{
	if (patternId < 0 || patternId >= LSO_SCANNER_PATTERN_NUM_MAX || param == nullptr) {
		return false;
	}

	if (auto* data = impl().board->getHbsDataProfile(); data) {
		if (auto* scan = const_cast<HbsLsoScanner*>(data->getHbsLsoScanner()); scan) {
			auto& traj = scan->traj_profile[patternId];
			traj.time_step_us = param->timeStepUs;
			traj.exposure_time_us = param->exposureTimeUs;
			traj.TrgSrc = param->triggerSource;
			traj.AcqMode = param->acquisitionMode;
			traj.AcqFrameSize = param->acqFrameSize;
			traj.sample_size = param->sampleSize;
			traj.subframe_size = param->subFrameSize;
			traj.prescan_pos_rewind_offset = param->prescanPosRewindOffset;
			traj.led_off_pos_index = param->ledOffPosIndex;
			traj.led_on_pos_index = param->ledOnPosIndex;

			auto startPos = param->yGalvoStartPos;;
			auto endPos = param->yGalvoEndPos;
			auto sampleSize = param->sampleSize;
			generateGalvanoPositions(startPos, endPos, sampleSize, traj.pos);

			if (data->saveLsoScannerParam()) {
				return true;
			}
		}
	}
	return false;
}

bool wso_device::LsoScanner::obtainCaptureParameters(int patternId, LsoScannerCaptureParam* param)
{
	if (patternId < 0 || patternId >= LSO_SCANNER_PATTERN_NUM_MAX || param == nullptr) {
		return false;
	}

	if (auto* data = impl().board->getHbsDataProfile(); data) {
		if (data->loadLsoScannerParam()) {
			if (auto* scan = data->getHbsLsoScanner(); scan) {
				param->controlStatus = scan->lso_capture_status.ctrl_status; 
				param->acquisitionCount = scan->lso_capture_status.acq_cnt;
				return true;
			}
		}
	}
	return false;
}

bool wso_device::LsoScanner::submitCaptureParameters(int patternId, const LsoScannerCaptureParam* param)
{
	if (patternId < 0 || patternId >= LSO_SCANNER_PATTERN_NUM_MAX || param == nullptr) {
		return false;
	}

	if (auto* data = impl().board->getHbsDataProfile(); data) {
		if (auto* scan = const_cast<HbsLsoScanner*>(data->getHbsLsoScanner()); scan) {
			scan->lso_capture_status.ctrl_status = param->controlStatus;
			scan->lso_capture_status.acq_cnt = param->acquisitionCount;

			if (data->saveLsoScannerParam()) {
				return true;
			}
		}
	}
	return false;
}

//bool wso_device::LsoScanner::storeControlParameters(void)
//{
//	auto* hbs = getMainBoard()->getHbsDataProfile();
//	auto* scanner = const_cast<HbsLsoScanner*>(hbs->getHbsLsoScanner());
//
//	auto& capture_status = scanner->lso_capture_status;
//	auto& scan_traj_profile = scanner->traj_profile;
//
//	capture_status.ctrl_status = impl().ctrlStatus;
//	capture_status.acq_cnt = impl().acqCnt;
//
//	scan_traj_profile[0] = impl().scanProfile[0];
//	scan_traj_profile[1] = impl().scanProfile[1];
//
//	if (!hbs->saveLsoScannerParam())
//	{
//		LogDebug() << "Lso scanner control parameters failed to store!";
//		return false;
//	}
//
//	return true;
//}

bool wso_device::LsoScanner::controlYGalvoMove(int ypos)
{
	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LsoGalvanoMoveY(ypos)) {
		LogD() << "Lso scanner, control y galvo move failed!";
		return false;
	}
	LogD() << "Lso scanner, control y galvo moved to pos: " << ypos;
	return true;
}


bool wso_device::LsoScanner::controlCapture(int nPatternId, int onOff)
{
	if (!impl().initiated) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LsoScannerControl(nPatternId, onOff)) {
		LogD() << "Lso scanner, control capture failed!";
		return false;
	}
	LogD() << "Lso scanner, capture switched: " << onOff;
	return true;
}

bool wso_device::LsoScanner::controlTriggerMode(int onOff)
{
	if (!impl().initiated) {
		return false;
	}

	UsbComm& usbComm = getMainBoard()->getUsbComm();
	if (!usbComm.LsoScannerTriggerControl(onOff)) {
		LogD() << "Lso scanner, control trigger mode failed!";
		return false;
	}
	LogD() << "Lso scanner, control trigger mode : " << onOff;
	return true;
}

bool wso_device::LsoScanner::startGrabbing(int nPatternId)
{
	if (!impl().initiated) {
		return false;
	}

	pauseGrabbing(nPatternId);

	if (!controlCapture(nPatternId, 1)) {
		pauseGrabbing(nPatternId);
		return false;
	}
	return true;
}

bool wso_device::LsoScanner::pauseGrabbing(int nPatternId)
{
	if (!impl().initiated) {
		return false;
	}

	if (!controlCapture(nPatternId, 0)) {
		return false;
	}
	return true;
}


//bool wso_device::LsoScanner::loadConfigFromIniFile()
//{
//	bool bRet = false;
//
//	try
//	{
//		IniFile* ini = getConfigIniFile();
//		if (!ini) {
//			return false;
//		}
//
//		impl().scanProfile[0].exposure_time_us = (float)ini->ReadInt(L"LSO_SCANNER_0", L"ExposureTime");
//		impl().scanProfile[0].TrgSrc = ini->ReadInt(L"LSO_SCANNER_0", L"TrgSrc");
//		impl().scanProfile[0].AcqMode = ini->ReadInt(L"LSO_SCANNER_0", L"AcqMode");
//		impl().scanProfile[0].AcqFrameSize = ini->ReadInt(L"LSO_SCANNER_0", L"AcqFrameSize");
//		impl().scanProfile[0].subframe_size = ini->ReadInt(L"LSO_SCANNER_0", L"SubFrameSize");
//		impl().scanProfile[0].led_on_pos_index = ini->ReadInt(L"LSO_SCANNER_0", L"LedOnPosIndex");
//		impl().scanProfile[0].led_off_pos_index = ini->ReadInt(L"LSO_SCANNER_0", L"LedOffPosIndex");
//		impl().scanProfile[0].prescan_pos_rewind_offset = ini->ReadInt(L"LSO_SCANNER_0", L"GalvanoRewindOffset");
//		impl().scanProfile[0].sample_size = ini->ReadInt(L"LSO_SCANNER_0", L"GalvoPatternSize");
//		impl().scanProfile[0].time_step_us = (float)ini->ReadInt(L"LSO_SCANNER_0", L"TimeStep");
//		impl().scanProfile[0].pos[0] = ini->ReadInt(L"LSO_SCANNER_0", L"GalvoStartPos");
//		impl().scanProfile[0].pos[impl().scanProfile[0].sample_size - 1] = ini->ReadInt(L"LSO_SCANNER_0", L"GalvoEndPos");
//		calcGalvanoPos(0, impl().scanProfile[0].sample_size);
//
//		impl().scanProfile[1].exposure_time_us = (float)ini->ReadInt(L"LSO_SCANNER_1", L"ExposureTime");
//		impl().scanProfile[1].TrgSrc = ini->ReadInt(L"LSO_SCANNER_1", L"TrgSrc");
//		impl().scanProfile[1].AcqMode = ini->ReadInt(L"LSO_SCANNER_1", L"AcqMode");
//		impl().scanProfile[1].AcqFrameSize = ini->ReadInt(L"LSO_SCANNER_1", L"AcqFrameSize");
//		impl().scanProfile[1].subframe_size = ini->ReadInt(L"LSO_SCANNER_1", L"SubFrameSize");
//		impl().scanProfile[1].led_on_pos_index = ini->ReadInt(L"LSO_SCANNER_1", L"LedOnPosIndex");
//		impl().scanProfile[1].led_off_pos_index = ini->ReadInt(L"LSO_SCANNER_1", L"LedOffPosIndex");
//		impl().scanProfile[1].prescan_pos_rewind_offset = ini->ReadInt(L"LSO_SCANNER_1", L"GalvanoRewindOffset");
//		impl().scanProfile[1].sample_size = ini->ReadInt(L"LSO_SCANNER_1", L"GalvoPatternSize");
//		impl().scanProfile[1].time_step_us = (float)ini->ReadInt(L"LSO_SCANNER_1", L"TimeStep");
//		impl().scanProfile[1].pos[0] = ini->ReadInt(L"LSO_SCANNER_1", L"GalvoStartPos");
//		impl().scanProfile[1].pos[impl().scanProfile[1].sample_size - 1] = ini->ReadInt(L"LSO_SCANNER_1", L"GalvoEndPos");
//		calcGalvanoPos(1, impl().scanProfile[1].sample_size);
//
//		bRet = true;
//	}
//	catch (exception ex)
//	{
//		bRet = false;
//	}
//	return bRet;
//}
//
//bool wso_device::LsoScanner::saveConfigToIniFile()
//{
//	bool bRet = false;
//
//	try
//	{
//		IniFile* ini = getConfigIniFile();
//		if (!ini) {
//			return false;
//		}
//
//		ini->WriteInt(L"LSO_SCANNER_0", L"ExposureTime", (int)impl().scanProfile[0].exposure_time_us);
//		ini->WriteInt(L"LSO_SCANNER_0", L"TrgSrc", impl().scanProfile[0].TrgSrc);
//		ini->WriteInt(L"LSO_SCANNER_0", L"AcqMode", impl().scanProfile[0].AcqMode);
//		ini->WriteInt(L"LSO_SCANNER_0", L"AcqFrameSize", impl().scanProfile[0].AcqFrameSize);
//		ini->WriteInt(L"LSO_SCANNER_0", L"SubFrameSize", impl().scanProfile[0].subframe_size);
//		ini->WriteInt(L"LSO_SCANNER_0", L"LedOnPosIndex", impl().scanProfile[0].led_on_pos_index);
//		ini->WriteInt(L"LSO_SCANNER_0", L"LedOffPosIndex", impl().scanProfile[0].led_off_pos_index);
//		ini->WriteInt(L"LSO_SCANNER_0", L"GalvanoRewindOffset", impl().scanProfile[0].prescan_pos_rewind_offset);
//		ini->WriteInt(L"LSO_SCANNER_0", L"GalvoPatternSize", impl().scanProfile[0].sample_size);
//		ini->WriteInt(L"LSO_SCANNER_0", L"TimeStep", (int)impl().scanProfile[0].time_step_us);
//		ini->WriteInt(L"LSO_SCANNER_0", L"GalvoStartPos", impl().scanProfile[0].pos[0]);
//		ini->WriteInt(L"LSO_SCANNER_0", L"GalvoEndPos", impl().scanProfile[0].pos[impl().scanProfile[0].sample_size - 1]);
//
//		ini->WriteInt(L"LSO_SCANNER_1", L"ExposureTime", (int)impl().scanProfile[1].exposure_time_us);
//		ini->WriteInt(L"LSO_SCANNER_1", L"TrgSrc", impl().scanProfile[1].TrgSrc);
//		ini->WriteInt(L"LSO_SCANNER_1", L"AcqMode", impl().scanProfile[1].AcqMode);
//		ini->WriteInt(L"LSO_SCANNER_1", L"AcqFrameSize", impl().scanProfile[1].AcqFrameSize);
//		ini->WriteInt(L"LSO_SCANNER_1", L"SubFrameSize", impl().scanProfile[1].subframe_size);
//		ini->WriteInt(L"LSO_SCANNER_1", L"LedOnPosIndex", impl().scanProfile[1].led_on_pos_index);
//		ini->WriteInt(L"LSO_SCANNER_1", L"LedOffPosIndex", impl().scanProfile[1].led_off_pos_index);
//		ini->WriteInt(L"LSO_SCANNER_1", L"GalvanoRewindOffset", impl().scanProfile[1].prescan_pos_rewind_offset);
//		ini->WriteInt(L"LSO_SCANNER_1", L"GalvoPatternSize", impl().scanProfile[1].sample_size);
//		ini->WriteInt(L"LSO_SCANNER_1", L"TimeStep", (int)impl().scanProfile[1].time_step_us);
//		ini->WriteInt(L"LSO_SCANNER_1", L"GalvoStartPos", impl().scanProfile[1].pos[0]);
//		ini->WriteInt(L"LSO_SCANNER_1", L"GalvoEndPos", impl().scanProfile[1].pos[impl().scanProfile[1].sample_size - 1]);
//
//		bRet = true;
//	}
//	catch (exception ex)
//	{
//		bRet = false;
//	}
//
//	return bRet;
//}


bool wso_device::LsoScanner::generateGalvanoPositions(short startPos, short endPos, int sampleSize, short* coords)
{
	if (sampleSize <= 0 || sampleSize >= LSO_SCANNER_SAMPLE_SIZE_MAX || coords == nullptr) {
		return false;
	}

	// 구간 수 = N-1 (양 끝점을 모두 포함)
	double delta = double(endPos - startPos);
	double step = delta / double(sampleSize - 1);

	std::fill_n(coords, (int)LSO_SCANNER_SAMPLE_SIZE_MAX, 0); // 전체 사이즈를 0으로 초기화

	for (std::size_t i = 0; i < sampleSize; ++i) {
		double value = startPos + step * double(i);
		coords[i] = static_cast<short int>(std::round(value));
	}
	return true;
}


void wso_device::LsoScanner::calcGalvanoPos(int nPatternId, int nGalvanoSampleSize)
{
	size_t count = 0;
	int start = impl().scanProfile[nPatternId].pos[0];
	int end = impl().scanProfile[nPatternId].pos[nGalvanoSampleSize - 1];

	// 구간 수 = N-1 (양 끝점을 모두 포함)
	double delta = double(end - start);
	double step = delta / double(nGalvanoSampleSize - 1);

	std::fill_n(impl().scanProfile[nPatternId].pos, (int)MAX_LSO_DAC_SAMPLE_SIZE, 0); // 전체 사이즈를 0으로 초기화

	for (std::size_t i = 0; i < nGalvanoSampleSize; ++i) {
		double value = start + step * double(i);
		impl().scanProfile[nPatternId].pos[i] = static_cast<short int>(std::round(value));
	}
}


MainBoard* wso_device::LsoScanner::getMainBoard(void) const
{
	return impl().board;
}

LsoScanner::LsoScannerImpl& wso_device::LsoScanner::impl(void) const
{
	return *d_ptr;
}