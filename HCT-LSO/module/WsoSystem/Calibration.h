#pragma once

#include "WsoSystem2.h"

#include <memory>
#include <mutex>


namespace wso_system
{
	class WSOSYSTEM_DLL_API Calibration
	{
	public:
		Calibration();
		virtual ~Calibration();

		Calibration(const Calibration& rhs) = delete;
		Calibration& operator=(const Calibration& rhs) = delete;

		static Calibration* getInstance(void);
		static std::mutex singleMutex_;

	public:
		bool loadSystemCalibration(bool fetch = false, int region = CALIB_ACTIVE_REGION);
		bool saveSystemCalibration(bool write = false, int region = CALIB_ACTIVE_REGION);
		bool applySystemCalibration(void);

		bool loadMotorSetupCalibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);
		bool loadOctParamsCalibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);
		bool loadLedSourceCalibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);
		bool loadOctGalvanoCalibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);
		bool loadDeviceConfigCalibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);
		bool loadStepMotorsCalibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);
		bool loadFactorySetup1Calibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);
		bool loadFactorySetup2Calibration(bool fetch = true, int region = CALIB_ACTIVE_REGION);

		bool saveMotorSetupCalibration(bool write = true, int region = CALIB_ACTIVE_REGION);
		bool saveOctParamsCalibration(bool write = true, int region = CALIB_ACTIVE_REGION);
		bool saveLedSourceCalibration(bool write = true, int region = CALIB_ACTIVE_REGION);
		bool saveOctGalvanoCalibration(bool write = true, int region = CALIB_ACTIVE_REGION);
		bool saveDeviceConfigCalibration(bool write = true, int region = CALIB_ACTIVE_REGION);
		bool saveStepMotorsCalibration(bool write = true, int region = CALIB_ACTIVE_REGION);
		bool saveFactorySetup1Calibration(bool write = true, int region = CALIB_ACTIVE_REGION);
		bool saveFactorySetup2Calibration(bool write = true, int region = CALIB_ACTIVE_REGION);

		bool obtainSystemCalibration(SystemCalibration* sys_calib, bool fetch);
		bool submitSystemCalibration(const SystemCalibration* sys_calib, bool write);

	private:
		struct CalibrationImpl;
		std::unique_ptr<CalibrationImpl> d_ptr;
		CalibrationImpl& impl(void) const;
	};
}

